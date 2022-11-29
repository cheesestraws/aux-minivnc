#include <sys/ioctl.h>

#include "session.h"
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
	// get the video mode, we need to know how much memory to allocate
	
	fb_get_video_info(sess, &fb->vp, &fb->vi);
	
	if (fb->vp.vpPixelSize != 8) {
		fbTODO("pixel depths != 8 are not yet supported");
	}
	
	// Ideally, this would be the screen memory mapped into our process space
	// (we'd need to phys(...) in the kernel module).  But I don't know how to
	// yet.  So we're going to have to copy the whole screen memory into here.
	fb->mirror_size = fb->vi.video_mem_x * fb->vi.video_mem_y;
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

	// TODO: Optimise this later
	fbTODO("fb_update_clut is still pessimal");
	
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
	
	// First, update the CLUT
	fb_update_clut(sess, fb);
	
	// Then, copy screen memory out.
	ret = read(sess->fb_fd, fb->screen_mirror, fb->mirror_size);
	printf("fb_update: read %d bytes\n", ret);
	
	// Let's pretend that nothing other than 8 bit depth exists for the
	// moment.  Copy the screen mirror into the bitmap to send.

	src = fb->screen_mirror;
	dst = fb->frame_to_send;
	for (i = 0; i < fb->vi.video_scr_y; i++) {
		memcpy(dst, src, fb->vi.video_scr_x);
		dst += fb->vi.video_scr_x;
		src += fb->vi.video_mem_x;
	}
	
	printf("copied\n");
}

