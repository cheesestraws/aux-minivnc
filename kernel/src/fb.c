#define KERNEL

#include <sys/errno.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/sysmacros.h>
#include <sys/uio.h>
#include <sys/user.h>

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
		vsrc = video_index[0];
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
	}

	return EINVAL;
}

int fb_select(dev, rw) dev_t dev; int rw; {
	return 1;
}

