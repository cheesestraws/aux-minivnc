#ifndef FRAME_BUFFER_H
#define FRAME_BUFFER_H

#include "../kernel/src/fb.h"
#include <mac/quickdraw.h>

#include "vnc_types.h"

typedef struct fb_rect {
	int x1, x2, y1, y2;
} fb_rect;

typedef struct  {
	VPBlock vp;
	struct video vi;

	char* screen_mirror;
	int mirror_size;
	char* frame_to_send;
	int frame_size;
	
	int last_changed;
	fb_rect last_dirty;
	
	int clut_changed;
	unsigned int clut_hash;
	ColorSpec clut[256];
	VNCColour vnc_clut[256];
} frame_buffer;

#endif