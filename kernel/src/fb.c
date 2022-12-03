#define KERNEL

#include <sys/errno.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/sysmacros.h>
#include <sys/uio.h>
#include <sys/user.h>
#include <sys/mouse.h>
#include <sys/user.h>
#include <sys/key.h>

#include <mac/quickdraw.h>
#include <mac/events.h>

#include "fb.h"

/* utility functions */

struct FB_VDEEntRec {
	char* csTable;
	unsigned short start;
	unsigned short count;
};

char fb_c[2048];
struct FB_VDEEntRec fb_vde;

int fb_getCLUTFor(v) struct video* v; {
	int ret;
	int csCode;
	
	ret = 0;
	
	csCode = 3;
	fb_vde.csTable = fb_c;
	fb_vde.start = 0;
	fb_vde.count = 255;

	// kallDriver calls the Mac video driver with the given
	// csCode and csParam.
	// 3 -> GetEntries (gets CLUT entries)
	ret = kallDriver(v, csCode, &fb_vde, 2);
	
	return ret;
}

/* mouse utilities */
extern int ui_devices;
typedef int (*mousecall_t)();
extern char* ui_lowaddr;
extern int ui_active;

// some low memory globals
#define MBSTATE 0x172
#define MTEMP 0x828
#define RAWMOUSE 0x82C
#define CRSRNEW 0x8CE
#define CRSRCOUPLE 0x8CF


void fb_get_mouse_mode_and_callback(id, mode, call)
	int id; int* mode; mousecall_t* call; {
	
	int s;
	int m;
	mousecall_t c;

	/* this is an awful hack.  These are hidden in mouse.o's static
	   scope, and we only have 'swap' calls available externally.  So
	   we swap dummy data in, make a note of what we get back, then swap
	   the originals back, all while keeping interrupts disabled. */
	   
	s = splhi();
	
	// Get the mouse mode
	m = mouse_op(id, MOUSE_OP_MODE, 0);
	mouse_op(id, MOUSE_OP_MODE, m);
	
	// And the callback
	c = (mousecall_t)mouse_op(id, MOUSE_OP_INTR, (void*)0);
	mouse_op(id, MOUSE_OP_INTR, c);
		
	splx(s);
	
	*mode = m;
	*call = c;
}


void fb_ui_mouse(x, y, button) short x; short y; short button; {
	Point p;
	int active;
	char oldBtn;
	char newBtn;
	
	p.h = x;
	p.v = y;
	
	// Poke the mouse position directly into "low" memory
	*(Point*)(&ui_lowaddr[MTEMP]) = p;
	*(Point*)(&ui_lowaddr[RAWMOUSE]) = p;
	ui_lowaddr[CRSRNEW] = ui_lowaddr[CRSRCOUPLE];
	
	// Now do something sensible (?) with the button
	oldBtn = ui_lowaddr[MBSTATE];

	if (button) {
		newBtn = 0x00;
	} else {
		newBtn = 0x80;
	}
	
	if (oldBtn != newBtn) {
		ui_lowaddr[MBSTATE] = newBtn;
		active = ui_active;
		
		if (button) {
			UI_postevent(0, active, mouseDown, 0);
		} else {
			UI_postevent(0, active, mouseUp, 0);
		}
	}
	
}

void move_mouse_to(id, x, y, button) int id; short x; short y; short button; {
	int mode;
	mousecall_t call;
	
	// if the decices are attached to a uinter layer, poke the mac
	// environment directly.
	if (ui_devices) {
		fb_ui_mouse(x, y, button);
		return;
	}	
	
	fb_get_mouse_mode_and_callback(id, &mode, &call);
	
	mouse_x[id] = x;
	mouse_y[id] = y;
	mouse_button[id] = button;
	
	
	// Now we do the magic call that the higher layers expect
	if (mode) {
		(*call)(id,MOUSE_CHANGE,0,3);
	}
}

/* keyboard gubbins */
typedef int (*keycall_t)();

int fb_kb_mode(id) int id; {
	int x;
	int mode;
	x = splhi();
	
	mode = key_op(id, KEY_OP_MODE, 0);
	key_op(id, KEY_OP_MODE, mode);
	
	splx(x);
	
	return mode;
}

keycall_t fb_kb_call(id) int id; {
	int x;
	keycall_t c;
	x = splhi();
	
	c = (keycall_t)key_op(id, KEY_OP_INTR, 0);
	key_op(id, KEY_OP_INTR, c);
	
	splx(x);
	
	return c;
}


void fb_kb_kchr(id, kchr) int id; int kchr; {
	keycall_t keycall;
	
	keycall = fb_kb_call(id);
	(*keycall)(id, KC_CHAR, kchr, 0);
}

/* driver functions */

int fb_init() {
	return 0;
}

int fb_open(dev) dev_t dev; {
  if(minor(dev) != 0) return EINVAL;
  return 0;
}

int fb_close(dev) dev_t dev; {
  return 0;
}

int fb_read(dev, uio) dev_t dev; struct uio* uio; {
	int dev_index;
	struct video* v;
	int size;
	int ret;

	
	dev_index = minor(dev);
	if (dev_index >= video_count) {
		return EINVAL;
	}
	
	v = video_desc[dev_index];
	size = v->video_mem_x * v->video_mem_y;

	ret = uiomove(v->video_addr, size, UIO_READ, uio);
	return ret;
}

int fb_write(dev, uio) dev_t dev; struct uio* uio; {
	return EINVAL;
}

int fb_ioctl(dev, cmd, addr, arg)
  dev_t dev; int cmd; caddr_t* addr; int arg;
{
	int ret;
	int chunk;
	int i;
	int base;
	struct video* vsrc;
	struct video* vdst;
	struct VPBlock* psrc;
	struct VPBlock* pdst;
	struct fb_clut_chunk* cdst;
	int mouse_id;
	struct fb_mouse* meese;
	
	int dev_index;
	
	dev_index = minor(dev);
	
	/* metadata */
	switch (cmd) {
	case FB_METADATA:
		vdst = (struct video*)addr;
		vsrc = video_desc[dev_index];
		
		/* copy the fields we can */
		vdst->video_mem_x = vsrc->video_mem_x;
		vdst->video_mem_y = vsrc->video_mem_y;
		vdst->video_scr_x = vsrc->video_scr_x;
		vdst->video_scr_y = vsrc->video_scr_y;
		vdst->video_page = vsrc->video_page;
		vdst->video_def_mode = vsrc->video_def_mode;
		vdst->video_slot = vsrc->video_slot;
		vdst->video_flags = vsrc->video_flags;
		vdst->video_off = vsrc->video_off;
		
		return 0;
	case FB_MODE:
		pdst = (struct VPBlock*)addr;
		psrc = &(video_desc[dev_index]->vpBlock);
		
		pdst->vpBaseOffset = psrc->vpBaseOffset;
		pdst->vpRowBytes = psrc->vpRowBytes;
		pdst->vpVersion = psrc->vpVersion;
		pdst->vpPackType = psrc->vpPackType;
		pdst->vpPackSize = psrc->vpPackSize;
		pdst->vpHRes = psrc->vpHRes;
		pdst->vpVRes = psrc->vpVRes;
		pdst->vpPixelType = psrc->vpPixelType;
		pdst->vpPixelSize = psrc->vpPixelSize;
		pdst->vpCmpCount = psrc->vpCmpCount;
		pdst->vpCmpSize = psrc->vpCmpSize;
		pdst->vpPlaneBytes = psrc->vpPlaneBytes;
				
		return 0;
		
	case FB_CLUT_CHUNK:
		vsrc = video_index[dev_index];
		cdst = (struct fb_clut_chunk*)addr;
		
		cdst->clut[0] = 25;
		cdst->clut[1] = 52;
		chunk = cdst->chunk;
		base = chunk * 64;
		ret = fb_getCLUTFor(vsrc);

		// copy the clut */
		for (i = 0; i < 64; i++) {
			cdst->clut[i] = fb_c[base + i];
		}
		
		return 0;
		
	case FB_MOVE_MOUSE:
		vsrc = video_desc[dev_index];
		mouse_id = vsrc->video_mouse_ind;
		meese = (struct fb_mouse*)addr;
		
		move_mouse_to(mouse_id, meese->x, meese->y, meese->button);
		return 0;
		
	case FB_KB_MODE:
		vsrc = video_desc[dev_index];
		i = fb_kb_mode(vsrc->video_key_ind);
		*((int*)addr) = i;
		
		return 0;
		
	case FB_KB_KCHR:
		vsrc = video_desc[dev_index];
		i = *((int*)addr);
		fb_kb_kchr(vsrc->video_key_ind, i);
		
		return 0;
	}

	return EINVAL;
}

int fb_select(dev, rw) dev_t dev; int rw; {
	return 1;
}

