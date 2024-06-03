/*****************************************************************************

Copyright (c) 2024 Jaroslav Hensl <emulator@emulace.cz>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.

*****************************************************************************/

/* 32 bit RING-0 code for SVGA-II 3D acceleration */
#define SVGA

#include <limits.h>

#include "winhack.h"
#include "vmm.h"
#include "vxd.h"
#include "vxd_lib.h"

#include "svga_all.h"

#include "3d_accel.h"

#include "code32.h"

#include "vxd_svga.h"

#include "vxd_strings.h"

/* values from newer headers */
#define SVGA3D_SURFACE_SCREENTARGET (1 << 16)
#define SVGA3D_SURFACE_BIND_RENDER_TARGET     (1 << 24)

BOOL st_used = FALSE;
static BOOL st_defined = FALSE;

static SVGA_region_info_t st_region;

BOOL st_memory_allocate(DWORD size, DWORD *out)
{
	memset(&st_region, 0, sizeof(SVGA_region_info_t));
	
	st_region.region_id = ST_REGION_ID;
	st_region.size = size;
	st_region.mobonly = FALSE;
	
	if(SVGA_region_create(&st_region))
	{
		*out = (DWORD)st_region.address;
		
		/* don't count this region */
		SVGA_region_usage_reset();
		
		return TRUE;
	}
	
	return FALSE;
}

void st_defineScreen(DWORD w, DWORD h, DWORD bpp)
{
  SVGAFifoCmdDefineScreen *screen;
  SVGAFifoCmdDefineGMRFB  *fbgmr;
  SVGA3dCmdDefineGBScreenTarget *stid;
  SVGA3dCmdDefineGBSurface_v4 *gbsurf;
  SVGA3dCmdBindGBSurface   *gbbind;
  SVGA3dCmdBindGBScreenTarget *stbind;
  SVGA_DB_surface_t *sinfo;
  DWORD cmdoff = 0;
  
  wait_for_cmdbuf();
  
  screen = SVGA_cmd_ptr(cmdbuf, &cmdoff, SVGA_CMD_DEFINE_SCREEN, sizeof(SVGAFifoCmdDefineScreen));

	/* define screen (32 bit) */
	memset(screen, 0, sizeof(SVGAFifoCmdDefineScreen));
	screen->screen.structSize = sizeof(SVGAScreenObject);
	screen->screen.id = 0;
	screen->screen.flags = SVGA_SCREEN_MUST_BE_SET | SVGA_SCREEN_IS_PRIMARY;
	screen->screen.size.width = w;
	screen->screen.size.height = h;
	screen->screen.root.x = 0;
	screen->screen.root.y = 0;
	screen->screen.cloneCount = 0;
	screen->screen.backingStore.ptr.offset = 0;
	screen->screen.backingStore.ptr.gmrId = SVGA_GMR_FRAMEBUFFER;
	
	/* set backing store to framebuffer (for cases where surface_bpp != 32) */
  fbgmr = SVGA_cmd_ptr(cmdbuf, &cmdoff, SVGA_CMD_DEFINE_GMRFB, sizeof(SVGAFifoCmdDefineGMRFB));
	fbgmr->ptr.gmrId = SVGA_GMR_FRAMEBUFFER;
	fbgmr->ptr.offset = 0;
	fbgmr->bytesPerLine = SVGA_pitch(w, 32);
	fbgmr->format.bitsPerPixel = 32;
	fbgmr->format.colorDepth   = 24;
	fbgmr->format.reserved     = 0;
	
	/* create screen target */
	stid = SVGA_cmd3d_ptr(cmdbuf, &cmdoff, SVGA_3D_CMD_DEFINE_GB_SCREENTARGET, sizeof(SVGA3dCmdDefineGBScreenTarget));
	stid->stid = 0;
	stid->flags = SVGA_STFLAG_PRIMARY;
	stid->width = w;
	stid->height = h;
	stid->xRoot = 0;
	stid->yRoot = 0;
	
	submit_cmdbuf(cmdoff, SVGA_CB_SYNC, 0);
	
	wait_for_cmdbuf();
	cmdoff = 0;
	
	/* create gb texture */
	gbsurf = SVGA_cmd3d_ptr(cmdbuf, &cmdoff, SVGA_3D_CMD_DEFINE_GB_SURFACE_V4, sizeof(SVGA3dCmdDefineGBSurface_v4));
	gbsurf->sid                = ST_SURFACE_ID;
	gbsurf->surfaceFlags.low   = SVGA3D_SURFACE_SCREENTARGET | SVGA3D_SURFACE_HINT_RENDERTARGET | SVGA3D_SURFACE_BIND_RENDER_TARGET;
	gbsurf->surfaceFlags.hi    = 0;
	switch(bpp)
	{
		case 16: gbsurf->format = SVGA3D_R5G6B5; break;
		default: gbsurf->format = SVGA3D_X8R8G8B8; /*SVGA3D_R8G8B8A8_UNORM;*/ break;
	}
	gbsurf->numMipLevels       = 1;
	gbsurf->multisampleCount   = 1;
	gbsurf->autogenFilter      = SVGA3D_TEX_FILTER_NONE;
	gbsurf->size.width         = w;
  gbsurf->size.height        = h;
  gbsurf->size.depth         = 1;
  gbsurf->arraySize          = 1;
  gbsurf->bufferByteStride   = SVGA_pitch(w, 32);

	/* bind texture to our GMR/mob */
	gbbind        = SVGA_cmd3d_ptr(cmdbuf, &cmdoff, SVGA_3D_CMD_BIND_GB_SURFACE, sizeof(SVGA3dCmdBindGBSurface));
	gbbind->sid   = ST_SURFACE_ID;
	gbbind->mobid = st_region.region_id;
	
	/* bind screen target to the texture */
	stbind               = SVGA_cmd3d_ptr(cmdbuf, &cmdoff, SVGA_3D_CMD_BIND_GB_SCREENTARGET, sizeof(SVGA3dCmdBindGBScreenTarget));
	stbind->stid         = 0;
	stbind->image.sid    = ST_SURFACE_ID;
	stbind->image.face   = 0;
	stbind->image.mipmap = 0;
	
	sinfo = SVGA_GetSurfaceInfo(ST_SURFACE_ID);
	memset(sinfo, 0, sizeof(SVGA_DB_surface_t));
	sinfo->pid    = ~0UL;
	sinfo->width  = w;
	sinfo->height = h;
	sinfo->format = gbsurf->format;
	sinfo->bpp    = bpp;
	sinfo->gmrId  = ST_REGION_ID;
	sinfo->flags  = 0;
	
	submit_cmdbuf(cmdoff, SVGA_CB_SYNC, 0);

	st_defined = TRUE;
}

void st_destroyScreen()
{
	SVGA3dCmdDestroyGBScreenTarget *stid;
	SVGA3dCmdDestroyGBSurface      *gbsurf;
	DWORD cmdoff = 0;

	if(st_defined)
	{
		wait_for_cmdbuf();

		stid = SVGA_cmd3d_ptr(cmdbuf, &cmdoff, SVGA_3D_CMD_DESTROY_GB_SCREENTARGET, sizeof(SVGA3dCmdDestroyGBScreenTarget));
 		stid->stid = 0;

 		gbsurf = SVGA_cmd3d_ptr(cmdbuf, &cmdoff, SVGA_3D_CMD_DESTROY_GB_SURFACE, sizeof(SVGA3dCmdDestroyGBSurface));
 		gbsurf->sid = ST_SURFACE_ID;

 		submit_cmdbuf(cmdoff, SVGA_CB_SYNC, 0);

 		st_defined = FALSE;
 	}
}

