#ifndef FB_H
#define FB_H

#include <mac/events.h>
#include <mac/files.h>
#include <mac/devices.h>
#include <mac/video.h>
#include <sys/ioctl.h>
#include <sys/video.h>

/* according to io.h, we have only 128 bytes to play with, but in practice
   it seems to break *at* 128 bytes. So we return 16 colours at once. */
struct fb_clut_chunk {
	short chunk;
	char clut[64];
};

struct fb_mouse {
	short x;
	short y;
	short button;
};

struct fb_mouse_state {
	int mode;
	unsigned int call;
};

/* See remarks above about the clut: we can't smuggle out a whole kchr
   at once, so we chunk it. */
struct fb_kchr_chunk {
	short chunk;
	char data[96];
};

struct fb_keypress {
	int key;
	int flags;
};

/* ioctls */

/* FB_METADATA returns the useful bits of the 'video' struct for the
   device it's being called on.  See sys/video.h. */
#define FB_METADATA _IOWR('F', 0, struct video)

/* FB_MODE returns the VPBlock for the *current* video mode.  See 
   mac/video.h for more details. */
#define FB_MODE _IOWR('F', 1, struct VPBlock)

/* FB_CLUT_CHUNK returns a 64-byte chunk of the current CLUT. */
#define FB_CLUT_CHUNK _IOWR('F', 2, struct fb_clut_chunk)

/* FB_MOVE_MOUSE attempts to move the mouse pointer to {x,y}. */
#define FB_MOVE_MOUSE _IOW('F', 3, struct fb_mouse)

/* FB_KB_MODE returns the keyboard mode for the device: see
   sys/key.h for definitions. */
#define FB_KB_MODE _IOR('F', 4, int)

/* FB_KB_KCHR injects a keypress using the KCHR mechanism.  This is 
   suitable for ASCII, ARAW and Mac keyboard modes.  Supplying the
   right key code is up to you. */
#define FB_KB_KCHR _IOW('F', 5, int)

/* FB_CLUT_HASH returns the crc32 of the current CLUT, so you can
   see if it's changed. */
#define FB_CLUT_HASH _IOR('F', 6, int)

/* FB_MOUSE_STATE returns the mouse mode.  See sys/mouse.h for more 
   details.  Mostly useful for debugging. */
#define FB_MOUSE_STATE _IOR('F', 7, struct fb_mouse_state)

/* FB_UI_DEVICES returns whether the input devices are claimed by the
   user interface driver, and thus the Mac environment, or not. */
#define FB_UI_DEVICES _IOR('F', 8, int)

/* FB_KB_KCHR_CHUNK copies a chunk of the virtual keycode mapping
   to the caller. */
#define FB_KB_KCHR_CHUNK _IOWR('F', 9, struct fb_kchr_chunk)

/* FB_KB_KEYPRESS simulates a keypress using the KCHR mechanism, but
   allows the caller to specify the flags. */
#define FB_KB_KEYPRESS _IOW('F', 10, struct fb_keypress)



#endif
