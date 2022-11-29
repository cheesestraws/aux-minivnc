#ifndef FRAME_BUFFER_H
#define FRAME_BUFFER_H

#include "../kernel/src/fb.h"
#include <mac/quickdraw.h>

#include "vnc_types.h"

typedef struct  {
	VPBlock vp;
	struct video vi;

	char* screen_mirror;
	int mirror_size;
	char* frame_to_send;
	int frame_size;
	
	ColorSpec clut[256];
	VNCColour vnc_clut[256];
} frame_buffer;

#endif