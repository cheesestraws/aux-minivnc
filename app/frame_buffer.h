#ifndef FRAME_BUFFER_H
#define FRAME_BUFFER_H

#include "../kernel/src/fb.h"
#include <mac/quickdraw.h>

#include "vnc_types.h"

// An fb_rect is determined by its corners, not by an x/y/width/height.
// Note that it is assumed in multiple places in the code that x2 > x1
// and y2 > y1.
typedef struct fb_rect {
	int x1, x2, y1, y2;
} fb_rect;

#define FB_RECT_WIDTH(r) ( r.x2 + 1 - r.x1 )
#define FB_RECT_HEIGHT(r) ( r.y2 + 1 - r.y1 )


// an fb_cursor is a point in a framebuffer where we want to read from
typedef struct fb_cursor {
	int offset;
	int next_row_offset;
	int number_of_increments;
} fb_cursor;

typedef struct  {
	VPBlock vp;
	struct video vi;

	char* frame_to_send;
	int frame_size;
	
	char* physed_fb;
	
	int last_changed;
	fb_rect last_dirty;
	fb_cursor dirty_cursor;
	
	int clut_changed;
	unsigned int clut_hash;
	ColorSpec clut[256];
	VNCColour vnc_clut[256];
} frame_buffer;

#endif