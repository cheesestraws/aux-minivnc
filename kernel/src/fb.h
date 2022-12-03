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


#define FB_METADATA _IOWR('F', 0, struct video)
#define FB_MODE _IOWR('F', 1, struct VPBlock)
#define FB_CLUT_CHUNK _IOWR('F', 2, struct fb_clut_chunk)
#define FB_MOVE_MOUSE _IOW('F', 3, struct fb_mouse)
#define FB_KB_MODE _IOR('F', 4, int)
#define FB_KB_KCHR _IOW('F', 5, int)
#define FB_CLUT_HASH _IOR('F', 6, int)



#endif
