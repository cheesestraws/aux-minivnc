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


/* VIDEO UTILITY FUNCTIONS
   ======================= */

/* FB_VDEEntRec is a type for the parameter block of the "get CLUT"
   call into the video driver. */
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

// Macros for Slot Manager calls
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
	int i;
	
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
	
	for (i = 0; i < sizeof(VPBlock); i++) {
		((char*)vpb)[i] = ((char*)vp)[i];
	}
	//memcpy(vpb, vp, sizeof(VPBlock));
	
	cDisposPtr((char*)sb.spResult);
	
	return noErr;
}

/* fb_update_modelist updates the cached list of modes for the
   given screen device. */
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

/* fb_current_mode gets the current mode number.  To turn it into
   an index into fb_vp[dev], subtract 0x80. */
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

// fb_GetCLUTFor returns the CLUT for the video device with the provided
// index.  It dumps it into fb_c.
int fb_getCLUTFor(idx) int idx; {
	int ret;
	int mode;
	int colours;
	int csCode;
	struct video* v;
	struct FB_VDEEntRec fb_vde;
	
	v = video_desc[idx];
	
	// First, check how many colours are *in* the clut.  If we request
	// too many from the driver it will refuse to talk to us.
	mode = fb_current_mode(v) - 0x80;
	colours = 1 << fb_vp[idx][mode].vpPixelSize;
	
	ret = 0;
	
	csCode = 3;
	fb_vde.csTable = fb_c;
	fb_vde.start = 0;
	fb_vde.count = colours - 1;

	// kallDriver calls the Mac video driver with the given
	// csCode and csParam.
	// 3 -> GetEntries (gets CLUT entries)
	ret = kallDriver(v, csCode, &fb_vde, 2);
	
	if (ret != 0) {
		printf("fb_getCLUTFor: return %d\n", ret);
	}
	
	return ret;
}




/* MOUSE UTILITY FUNCTIONS
   ======================= */

/* mousecall_t is a convenience type for mouse callbacks.  When a "real"
   mouse mvoes, the kernel calls a callback.  We're going to emulate this
   behaviour by calling that callback ourselves and hoping it works, 
   unless we're in the mac environment. */
typedef int (*mousecall_t)();

/* ui_devices is a variable defined by the user interface driver.  It's 
   true if the input devices have been claimed by the user interface 
   driver. */
extern int ui_devices;

/* ui_lowaddr is a variable defined by the user interface driver.  It is
   the home of the low memory globals of the mac environment. */
extern char* ui_lowaddr;

/* ui_active is a variable defined by the user interface driver.  It is
   the number of the currently active uinter layer. */
extern char ui_active;

/* Some low memory globals we're going to need. */
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

/* fb_ui_mouse moves the mouse if the mac environment is active.  It does
   this by totally bypassing the kernel gubbins and just poking the mac
   environment directly.  This means we get proper absolute mouse 
   positioning in the mac environment. */
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

/* fb_move_mouse_to attempts to move the mouse to {x, y}, calling the
   appropriate callbacks and special-casing for the mac environment. 
   It also simulates mouse clicks.  Look, if you press the button, some
   bit of the mouse is still moving, ok?  This is totally a philosophical
   choice and not just that I named the function badly. */
void fb_move_mouse_to(id, x, y, button) int id; short x; short y; short button; {
	int mode;
	int dx;
	int dy;
	int rebuff; // reconstructed ADB from mouse
	mousecall_t call;
	
	// if the devices are attached to a uinter layer, poke the mac
	// environment directly.
	if (ui_devices) {
		fb_ui_mouse(x, y, button);
		return;
	}	
	
	fb_get_mouse_mode_and_callback(id, &mode, &call);
	
	dx = x - mouse_x[id];
	dy = y - mouse_y[id];
	
	mouse_x[id] = x;
	mouse_y[id] = y;
	mouse_button[id] = button;
		
	rebuff = (dx & 0x3f) | ((dy & 0x3f)<<8);
	if (button) {
		rebuff = rebuff | (1 << 15);
	}
	
	// Now we do the magic call that the higher layers expect
	if (mode) {
		(*call)(id,MOUSE_CHANGE,rebuff,3);
	}
}




/* KEYBOARD UTILITY FUNCTIONS
   ======================= */

/* keycall_t is a convenience call for keyboard callback routines.  When
   the kernel hands off a key event to another subsystem, it calls a
   keyboard callback provided by that subsystem.  We emulate keypresses
   by calling those callbacks ourselves. */
typedef int (*keycall_t)();

/* transData is a variable defined somewhere else in the kernel (?!).  It
   contains the contents of the KCHR resource that the mac environment is
   currently using.  The KCHR maps between virtual keycodes and what 
   characters get generated by those keys. */
extern char transData; 

/* fb_kb_mode returns the current keyboard mode.  See sys/key.h for
   the valid modes. */
int fb_kb_mode(id) int id; {
	int x;
	int mode;
	x = splhi();
	
	mode = key_op(id, KEY_OP_MODE, 0);
	key_op(id, KEY_OP_MODE, mode);
	
	splx(x);
	
	return mode;
}

/* fb_kb_call returns the current keyboard callback. */
keycall_t fb_kb_call(id) int id; {
	int x;
	keycall_t c;
	x = splhi();
	
	c = (keycall_t)key_op(id, KEY_OP_INTR, 0);
	key_op(id, KEY_OP_INTR, c);
	
	splx(x);
	
	return c;
}

/* fb_kb_kchr injects a keypress using the KC_CHAR action.  This is
   suitable for ARAW, MAC and ASCII modes.  I've not run into anything that
   uses raw mode yet. */
void fb_kb_kchr(id, kchr, flags) int id; int kchr; int flags; {
	keycall_t keycall;
	
	keycall = fb_kb_call(id);
	(*keycall)(id, KC_CHAR, kchr, flags);
}



/* PHYS UTILITIES
   ============== */
   

/* fb_fb_size returns the size of the framebuffer (up to 8bpp) */
int fb_fb_size(video_id) {
	int mode8;
	int i;
	
	// work out the size.
	// hack for now: look at the 8-bit mode.
	mode8 = -1;
	for (i = 0; i < QD_MAX_MODES; i++) {
		if (fb_vp[video_id][i].vpPixelSize >= 8) {
			mode8 = i;
			break;
		}
	}
	if (mode8 == -1) {
		printf("fb: couldn't find 8-bit mode\n");
		return -1;
	}
	return fb_vp[video_id][mode8].vpRowBytes * video_desc[video_id]->video_mem_y;

}

int fb_phys(video_id, phys) int video_id; struct fb_phys* phys; {
	int size;
	int phys_id;
	
	struct user* up;
	up = &u;
	
	size = fb_fb_size(video_id);
	
	up->u_error = 0;
	dophys(1, phys->addr, size, video_desc[video_id]->video_addr);
	phys_id = 1;

	if (up->u_error == 0) {
		phys->ok = 1;
		phys->phys_id = phys_id;
		return 0;
	} else {
		phys->ok = 0;
		return up->u_error;
	}
}




/* THE ACTUAL DRIVER
   ================= */

/* fb_init is called when the driver is initialised. */
int fb_init() {
	return 0;
}

/* fb_open is called when our device node is opened.  We check whether
   this is a request for a video device that actually exists or not here,
   and update the cached video mode list for the device. */
int fb_open(dev) dev_t dev; {
  if(minor(dev) >= NSCREENS) return EINVAL;
  
  fb_update_modelist(minor(dev));
  
  return 0;
}

/* fb_close is called when our device is closed.  We don't care. */
int fb_close(dev) dev_t dev; {
  return 0;
}

/* fb_read is called when someone reads from our device. Any attempt 
   to read just returns the contents of the current  framebuffer for 
   this video device. */
int fb_read(dev, uio) dev_t dev; struct uio* uio; {
	int dev_index;
	struct video* v;
	int size;
	int mem_x;
	int ret;
	int mode;
	
	dev_index = minor(dev);
	if (dev_index >= video_count) {
		return EINVAL;
	}
			
	v = video_desc[dev_index];
	
	mode = fb_current_mode(v) - 0x80;
	mem_x = fb_vp[dev_index][mode].vpRowBytes;
	
	size = mem_x * v->video_mem_y;

	ret = uiomove(v->video_addr, size, UIO_READ, uio);
	return ret;
}

/* fb_write is called when someone writes to our device.  We don't allow 
   writing to the framebuffer, so just error. */
int fb_write(dev, uio) dev_t dev; struct uio* uio; {
	return EINVAL;
}

/* fb_ioctl is called when someone ioctls us.  Surprising.  This is a big
   switch statement, really: see the definitions of the ioctls in fb.h
   for what ioctls do what. */
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
	struct fb_kchr_chunk* kdst;
	struct fb_keypress* kpsrc;
	char* csrc;
	int mouse_id;
	struct fb_mouse* meese;
	struct fb_mouse_state* ms;
	
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
		ret = fb_getCLUTFor(dev_index);

		// copy the clut */
		for (i = 0; i < 64; i++) {
			cdst->clut[i] = fb_c[base + i];
		}
		
		return 0;
		
	case FB_MOVE_MOUSE:
		vsrc = video_desc[dev_index];
		mouse_id = vsrc->video_mouse_ind;
		meese = (struct fb_mouse*)addr;
		
		fb_move_mouse_to(mouse_id, meese->x, meese->y, meese->button);
		return 0;
		
	case FB_KB_MODE:
		vsrc = video_desc[dev_index];
		i = fb_kb_mode(vsrc->video_key_ind);
		*((int*)addr) = i;
		
		return 0;
		
	case FB_KB_KCHR:
		vsrc = video_desc[dev_index];
		i = *((int*)addr);
		fb_kb_kchr(vsrc->video_key_ind, i, 0);
		
		return 0;
	
	case FB_CLUT_HASH:
		vsrc = video_desc[dev_index];
		fb_getCLUTFor(dev_index);
		i = fb_crc32buf(fb_c, 2048);
		*((int*)addr) = i;
		
		return 0;
		
	case FB_MOUSE_STATE:
		ms = (struct fb_mouse_state*)addr;
		vsrc = video_desc[dev_index];
		mouse_id = vsrc->video_mouse_ind;

		fb_get_mouse_mode_and_callback(mouse_id, &(ms->mode), (mousecall_t*)&(ms->call));
		return 0;
		
	case FB_UI_DEVICES:
		*((int*)addr) = ui_devices;
		return 0;
		
	case FB_KB_KCHR_CHUNK:
		/* there's only one kchr for the whole kernel, so we
		   don't care about what device we are */
		kdst = (struct fb_kchr_chunk*)addr;

		   
		if (kdst->chunk >= 32) {
			// There's only a 3k buffer; 32 * 96.
			return EINVAL;
		}
		
		base = kdst->chunk * 96;
		csrc = &transData + base;
		for (i = 0; i < 96; i++) {
			kdst->data[i] = csrc[i];
		}
		return 0;
		
	case FB_KB_KEYPRESS:
		vsrc = video_desc[dev_index];
		kpsrc = (struct fb_keypress*)addr;
		fb_kb_kchr(vsrc->video_key_ind, kpsrc->key, kpsrc->flags);
		
		return 0;
		
	case FB_PHYS:
		return fb_phys(dev_index, (struct fb_phys*)addr);

	}

	return EINVAL;
}

/* fb_select is called when someone select()s on us.  We don't care.  We
   don't understand non-blocking I/O. */
int fb_select(dev, rw) dev_t dev; int rw; {
	return 1;
}

