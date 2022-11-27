#include <mac/events.h>
#include <mac/files.h>
#include <mac/devices.h>

#include <sys/ioctl.h>
#include <sys/video.h>
#include <errno.h>

#include <stdio.h>

#include "fb.h"

void print_video(v) struct video* v; {
	printf("video_mem_x: %ld\n", v->video_mem_x);
	printf("video_mem_y: %ld\n", v->video_mem_y);
	printf("video_scr_x: %ld\n", v->video_scr_x);
	printf("video_scr_y: %ld\n", v->video_scr_y);
	printf("video_page: %ld\n", v->video_page);
	printf("video_def_mode: %ld\n", v->video_def_mode);
	printf("video_slot: %ld\n", v->video_slot);
	printf("video_flags: %ld\n", v->video_flags);
	printf("video_off: %ld\n", v->video_off);
}

void print_mode(b) struct VPBlock* b; {
	printf("vpBaseOffset: %ld\n", b->vpBaseOffset);
	printf("vpRowBytes: %hd\n", b->vpRowBytes);
	printf("vpVersion: %hd\n", b->vpVersion);
	printf("vpPackType: %hd\n", b->vpPackType);
	printf("vpPackSize: %ld\n", b->vpPackSize);
	printf("vpHRes: %ld\n", b->vpHRes);
	printf("vpVRes: %ld\n", b->vpVRes);
	printf("vpPixelType: %hd\n", b->vpPixelType);
	printf("vpPixelSize: %hd\n", b->vpPixelSize);
	printf("vpCmpCount: %hd\n", b->vpCmpCount);
	printf("vpCmpSize: %hd\n", b->vpCmpSize);
	printf("vpPlaneBytes: %ld\n", b->vpPlaneBytes);
}

int main() {
	int fb_fd;
	struct video v;
	struct VPBlock b;
	struct fb_clut_chunk c;
	int ret;
	ColorSpec* cs;
	int i, j;
	
	int gm_size;
	unsigned char* greymap;
	ColorSpec lut[256];
		
	fb_fd = open("/dev/fb0");
	if (fb_fd < 0) {
		printf("open: error %d\n", errno);
	}
	
	ret = ioctl(fb_fd, FB_METADATA, &v);
	if (ret < 0) {
		printf("ioctl 1: error %d\n", errno);
	}
	
	//print_video(&v);
	
	ret = ioctl(fb_fd, FB_MODE, &b);
	if (ret < 0) {
		printf("ioctl 1: error %d\n", errno);
	}

	for (i = 0; i < 32; i++) {
		c.chunk = i;
		ret = ioctl(fb_fd, FB_CLUT_CHUNK, &c);
		if (ret < 0) {
			printf("ioctl 1: error %d\n", errno);
		}
		memcpy(&lut[i * 8], c.clut, 64);
	}

	//print_mode(&b);
	
	/*for (i = 0; i < 32; i++) {
		c.chunk = i;
		c.clut[0]=10;
		ret = ioctl(fb_fd, FB_CLUT_CHUNK, &c);
		if (ret < 0) {
			printf("ioctl 1: error %d\n", errno);
		}
	
		for (j = 0; j < 8; j++) {
			cs = (ColorSpec*)&c.clut[j*8];
			printf("%d %hx %hx %hx\n", cs->value, cs->rgb.red, cs->rgb.green, cs->rgb.blue);
		}
	} */
		
	gm_size = v.video_mem_x * v.video_mem_y;
	greymap = (unsigned char*)malloc(gm_size);
	if (!greymap) {
		printf("malloc farted\n");
	}
	
	ret = read(fb_fd, greymap, gm_size);
	if (ret < 0) {
		printf("read farted: %d\n", errno);
	}
	

	// output a pgm file
	printf("P3\n%d %d\n255\n", v.video_mem_x, v.video_mem_y);
	
	for (i = 0; i < gm_size; i++) {
		if (i % 17 == 0) {
			printf("\n");
		}
		cs = &lut[greymap[i]];
		
		printf("%d %d %d ", cs->rgb.red, cs->rgb.green, cs->rgb.blue);
	}
	
	printf("\n");
	
	return 0;
}