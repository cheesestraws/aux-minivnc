#define KERNEL

#include <string.h>

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
#include <mac/slots.h>

#include "fb.h"
#include "crc32.h"

/* utility functions */

struct FB_VDEEntRec {
	char* csTable;
	unsigned short start;
	unsigned short count;
};

// fb_c contains a clut that is in the process of being transferred to
// a user process
char fb_c[2048];

// This is an array of VPBlocks for screen modes available.  The first
// subscript is the device number; the second is the mode number - 0x80.
#define QD_MAX_MODES 6
VPBlock fb_vp[NSCREENS][QD_MAX_MODES];

#define SNextTypeSRsrc(x) slotmanager(0x15, (x))
#define SFindStruct(x) slotmanager(0x6, (x))
#define SGetBlock(x) slotmanager(0x5, (x))

/* fb_get_vpblock gets the video mode information for a specific mode
   for a specific slot (per the slot manager) and copies it into the
   block pointed to by vpb. */
int fb_get_vpblock(slot, mode, vpb) int slot; int mode; VPBlock* vpb; {
	SpBlock sb;
	int err;
	VPBlock* vp;
	
	err = noErr;
	
	/* Get the functional sResource for video */
	sb.spSlot = slot;
	sb.spID = 0;
	sb.spCategory = 3;
	sb.spCType = 1;
	sb.spDrvrSW = 1;
	sb.spTBMask = 1;  /* ignore spDrvrHW */
	err = SNextTypeSRsrc(&sb);
	if (err != 0) {
		return err;
	}
	
	/* are we still looking at the right slot? */
	if (sb.spSlot != slot) {
		return -1;
	}
	
	/* Good, now get the struct for the mode.  This is kind of badly 
	   documented in IM.  Modes are elements 0x80 and above in the
	   video functional sResource, and each mode is a struct.  This
	   is actually only explicitly stated in the diagram on p. 154 of
	   Cards & Drivers 3rd ed.  The second field of this struct is the
	   VPBlock. */
	sb.spID = mode;
	err = SFindStruct(&sb);
	if (err != noErr) {
		return err;
	}
	
	sb.spID = 1; /* Field within the mode struct */
	err = SGetBlock(&sb);
	if (err != noErr) {
		return err;
	}
	
	vp = (VPBlock*)sb.spResult;
	memcpy(vpb, vp, sizeof(VPBlock));
	
	cDisposPtr((char*)sb.spResult);
	
	return noErr;
}

void fb_update_modelist(video_id) int video_id; {
	int slot;
	struct video* v;
	int mode;
	int modeindex;
	int err;
	
	v = video_desc[video_id];
	slot = v->video_slot;
	
	mode = 0x80;
	err = 0;
	while (err == 0) {
		/* Have we run out of space for modes? */
		if (mode == 0x80 + QD_MAX_MODES) {
			break;
		} 
		
		modeindex = mode - 0x80;
		err = fb_get_vpblock(slot, mode, &fb_vp[video_id][modeindex]);
		
		mode++;
	}
}


int fb_getCLUTFor(v) struct video* v; {
	int ret;
	int csCode;
	struct FB_VDEEntRec fb_vde;
	
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

int fb_current_mode(v) struct video* v; {
	int ret;
	int csCode;
	struct VDPageInfo pi;
	
	ret = 0;
	
	csCode = 2;
	
	// kallDriver calls the Mac video driver with the given
	// csCode and csParam.
	// 2 -> GetMode (gets mode ID)
	ret = kallDriver(v, csCode, &pi, 2);

	return pi.csMode;
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
  
  fb_update_modelist(minor(dev));
  
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
		i = fb_current_mode(video_desc[dev_index]) - 0x80; 
		if (i >= QD_MAX_MODES) {
			return EINVAL;
		}
		
		psrc = &fb_vp[dev_index][i];
		
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
	
	case FB_CLUT_HASH:
		vsrc = video_desc[dev_index];
		fb_getCLUTFor(vsrc);
		i = fb_crc32buf(fb_c, 2048);
		*((int*)addr) = i;
		
		return 0;
	}

	return EINVAL;
}

int fb_select(dev, rw) dev_t dev; int rw; {
	return 1;
}

