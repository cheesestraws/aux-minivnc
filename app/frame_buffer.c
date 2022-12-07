#include <sys/ioctl.h>

#include "session.h"
#include "timer.h"
#include "luts.h"
#include "../kernel/src/fb.h"

void fbTODO(char* str) {
	printf("(fb/unimplemented) %s\n", str);
}

void fb_get_video_info(session *sess, VPBlock* vp, struct video* vi) {
	int ret;
	
	ret = ioctl(sess->fb_fd, FB_METADATA, vi);
	if (ret < 0) {	
	    session_err(sess, "FB_METADATA failed");
	}
	
	ret = ioctl(sess->fb_fd, FB_MODE, vp);
	if (ret < 0) {	
	    session_err(sess, "FB_MODE failed");
	}
}

void fb_init(session *sess, frame_buffer *fb) {
	// set all fields to 0
	memset((char*)fb, '\0', sizeof(frame_buffer));
	
	// get the video mode, we need to know how much memory to allocate
	fb_get_video_info(sess, &fb->vp, &fb->vi);
	
	if (fb->vp.vpPixelSize != 8) {
		printf("vpPixelSize => %d\n", fb->vp.vpPixelSize); 
		fbTODO("pixel depths != 8 are not yet supported");
	}
	
	// Ideally, this would be the screen memory mapped into our process space
	// (we'd need to phys(...) in the kernel module).  But I don't know how to
	// yet.  So we're going to have to copy the whole screen memory into here.
	fb->mirror_size = fb->vp.vpRowBytes * fb->vi.video_mem_y;
	fb->screen_mirror = (char*)malloc(fb->mirror_size);
	if (!fb->screen_mirror) {
		session_err(sess, "could not allocate screen_mirror");
	}
	
	fb->frame_size = fb->vi.video_scr_x * fb->vi.video_scr_y;
	fb->frame_to_send = (char*)malloc(fb->frame_size);
	if (!fb->frame_to_send) {
		session_err(sess, "could not allocate frame_to_send");
	}
}

void fb_free(frame_buffer *fb) {
	free(fb->screen_mirror);
	free(fb->frame_to_send);
}

void fb_update_clut(session *sess, frame_buffer *fb) {
	int i;
	int ret;
	struct fb_clut_chunk c;

	// See if the CLUT has changed
	ret = ioctl(sess->fb_fd, FB_CLUT_HASH, &i);
	if (ret < 0) {
		session_err(sess, "FB_CLUT_HASH failed");
	}
	if (fb->clut_hash != i) {
		fb->clut_hash = i;
		fb->clut_changed = 1;
	}

	if (!fb->clut_changed) {
		return;
	}	

	for (i = 0; i < 32; i++) {
		c.chunk = i;
		ret = ioctl(sess->fb_fd, FB_CLUT_CHUNK, &c);
		if (ret < 0) {
			session_err(sess, "FB_CLUT_CHUNK failed");
		}
		memcpy(&fb->clut[i * 8], c.clut, 64);
	}
	
	// And fill the VNC clut from the QuickDraw one
	for (i = 0; i < 256; i++) {
		fb->vnc_clut[i].red = fb->clut[i].rgb.red;
		fb->vnc_clut[i].green = fb->clut[i].rgb.green;
		fb->vnc_clut[i].blue = fb->clut[i].rgb.blue;
	}
}

void fb_update(session *sess, frame_buffer *fb) {
	int ret;
	int i;
	char* src;
	char* dst;
	long us;
	int changed = 0;
	int lineChanged = 0;
	unsigned int lowYChange = 0xFFFFFFFF;
	unsigned int highYChange = 0;
	unsigned int lowXChange = 0xFFFFFFFF;
	unsigned int highXChange = 0;
	int xstart;
	int xend;
	
	fb_get_video_info(sess, &fb->vp, &fb->vi);
	
	// First, update the CLUT
	fb_update_clut(sess, fb);
	
	// Then, copy screen memory out.
	us = start_us();
	ret = read(sess->fb_fd, fb->screen_mirror, fb->mirror_size);
	print_time_since("fb_update/read", us);
	
	// Let's pretend that nothing other than 8 bit depth exists for the
	// moment.  Copy the screen mirror into the bitmap to send.

	us = start_us();
	src = fb->screen_mirror;
	dst = fb->frame_to_send;
	for (i = 0; i < fb->vi.video_scr_y; i++) {
		xstart = xend = 0;
		if (fb->vp.vpPixelSize == 1) {
			lineChanged = fb_copy_1_row(dst, src, fb->vi.video_scr_x, &xstart, &xend);
		} else if (fb->vp.vpPixelSize == 2) {
			lineChanged = fb_copy_2_row(dst, src, fb->vi.video_scr_x, &xstart, &xend);
		} else if (fb->vp.vpPixelSize == 4) {
			lineChanged = fb_copy_4_row(dst, src, fb->vi.video_scr_x, &xstart, &xend);
		} else {
			lineChanged = fb_copy_8_row(dst, src, fb->vi.video_scr_x, &xstart, &xend);
		}

		changed = changed || lineChanged;
		if (lineChanged && (i < lowYChange)) {
			lowYChange = i;
		}
		if (lineChanged && (i > highYChange)) {
			highYChange = i;
		}
		
		if (lineChanged && xstart < lowXChange) {
			lowXChange = xstart;
		}
		
		if (lineChanged && xend > highXChange) {
			highXChange = xend;
		}
		
		dst += fb->vi.video_scr_x;
		src += fb->vp.vpRowBytes;
	}
	print_time_since("fb_update/copy", us);
	
	if (changed) {
		//printf("change: %d %d %d %d\n", lowXChange, lowYChange, highXChange, highYChange);
		fb->last_changed = changed;
		fb->last_dirty.y1 = lowYChange;
		fb->last_dirty.y2 = highYChange;
		fb->last_dirty.x1 = lowXChange;
		fb->last_dirty.x2 = highXChange;
	} else {
		fb->last_changed = changed;
		fb->last_dirty.y1 = 0;
		fb->last_dirty.y2 = 0;
		fb->last_dirty.x1 = 0;
		fb->last_dirty.x2 = 0;
	}
}

int fb_copy_8_row(char* dst, char* src, int width, int* xstart_, int* xend_) {
	int wi = width / 4;
	int i = 0;
	int changed = 0;
	int xstart = -1;
	int xend;
	
	for (i = 0; i < wi; i++) {
		if (((int*)dst)[i] != ((int*)src)[i]) {
			((int*)dst)[i] = ((int*)src)[i];
			changed = 1;
			if (xstart < 0) {
				xstart = i * 4;
			}
			xend = (i * 4) + 3; // last byte of this int
		}
	}
	
	*xstart_ = xstart;
	*xend_ = xend;
	
	return changed;
}

int fb_copy_1_row(char* dst, char* src, int width, int* xstart_, int* xend_) {
	int wi = width / 8;
	int i = 0;
	int j = 0;
	int changed = 0;
	int xstart = -1;
	int xend;
	unsigned int pixes1;
	unsigned int pixes2;
	int bit;
		
	for (i = 0; i < wi; i++) {
		pixes1 = lut1bit[(unsigned char)src[i] >> 4];
		pixes2 = lut1bit[(unsigned char)src[i] & 0xf];

		if (((unsigned int*)dst)[i * 2] != pixes1) {
			((unsigned int*)dst)[i * 2] = pixes1;
			if (xstart == -1) {
				changed = 1;
				xstart = (i * 2) * 4;
			}
			xend = (i * 2) * 4 + 3;
		}
		if (((unsigned int*)dst)[i * 2 + 1] != pixes2) {
			((unsigned int*)dst)[i * 2 + 1] = pixes2;
			if (xstart == -1) {
				changed = 1;
				xstart = (i * 2 + 1) * 4;
			}
			xend = (i * 2 + 1) * 4 + 3;

		}		
	}
	
	*xstart_ = xstart;
	*xend_ = xend;
	
	return changed;
}

int fb_copy_2_row(char* dst, char* src, int width, int* xstart_, int* xend_) {
	int wi = width / 4;
	int i = 0;
	int j = 0;
	int changed = 0;
	int xstart = -1;
	int xend;
	unsigned int pixes;
	unsigned int pixes2;
	int bit;
		
	for (i = 0; i < wi; i++) {
		pixes = lut2bit[(unsigned char)src[i]];

		if (((unsigned int*)dst)[i] != pixes) {
			((unsigned int*)dst)[i] = pixes;
			if (xstart == -1) {
				changed = 1;
				xstart = i * 4;
			}
			xend = (i * 4) + 3;
		}
	}
	
	*xstart_ = xstart;
	*xend_ = xend;
	
	return changed;
}

int fb_copy_4_row(char* dst, char* src, int width, int* xstart_, int* xend_) {
	int wi = width / 2;
	int i = 0;
	int j = 0;
	int changed = 0;
	int xstart = -1;
	int xend;
	unsigned short pixes;
	int bit;
		
	for (i = 0; i < wi; i++) {
		pixes = lut4bit[(unsigned char)src[i]];

		if (((unsigned short*)dst)[i] != pixes) {
			((unsigned short*)dst)[i] = pixes;
			if (xstart == -1) {
				changed = 1;
				xstart = i * 2;
			}
			xend = (i * 2) + 1;
		}
	}
	
	*xstart_ = xstart;
	*xend_ = xend;
	
	return changed;
}



void fb_reset_dirty_cursor(frame_buffer *fb) {
	fb->dirty_cursor.next_row_offset = fb->vi.video_scr_x;
	fb->dirty_cursor.offset = fb->last_dirty.y1 * fb->vi.video_scr_x;
	fb->dirty_cursor.offset+= fb->last_dirty.x1;
	fb->dirty_cursor.number_of_increments = 0;
}

void fb_advance_dirty_cursor(frame_buffer *fb) {
	fb->dirty_cursor.offset += fb->dirty_cursor.next_row_offset;
	fb->dirty_cursor.number_of_increments++;
}

int fb_size_at_dirty_cursor(frame_buffer *fb) {
	return FB_RECT_WIDTH(fb->last_dirty);
}


int fb_more_dirty(frame_buffer *fb) {
	return fb->dirty_cursor.number_of_increments < FB_RECT_HEIGHT(fb->last_dirty);
}

char* fb_dirty_cursor_ptr(frame_buffer *fb) {
	return fb->frame_to_send + fb->dirty_cursor.offset;
}
