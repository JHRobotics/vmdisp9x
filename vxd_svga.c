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
#include "vpicd.h"
#include "vxd_lib.h"

#include "svga_all.h"

#include "3d_accel.h"

#include "code32.h"

#include "vxd_svga.h"

#include "vxd_strings.h"

#include "svga_ver.h"

/*
 * consts
 */
#define SVGA3D_MAX_MOBS (SVGA3D_MAX_CONTEXT_IDS + SVGA3D_MAX_CONTEXT_IDS + SVGA3D_MAX_SURFACE_IDS)

#define SVGA_MIN_VRAM 8

#define SVGA_TIMEOUT 0x1000000UL

BOOL CB_queue_check(SVGACBHeader *tracked);

/*
 * globals
 */
extern FBHDA_t *hda;
extern ULONG hda_sem;
static SVGA_DB_t *svga_db = NULL;

extern LONG fb_lock_cnt;

       BOOL gb_support = FALSE;
       BOOL cb_support = FALSE;
       BOOL cb_context0 = FALSE;

/* for GPU9 is FIFO more stable (VMWARE) or faster (VBOX) */
static DWORD prefer_fifo = 1;

static BOOL SVGA_is_valid = FALSE;

static DWORD fence_next_id = 1;
void *cmdbuf = NULL;
void *ctlbuf = NULL;

/* guest frame buffer is dirty */
BOOL ST_FB_invalid = FALSE;

DWORD present_fence = 0;
/*
 * strings
 */
static char SVGA_conf_path[] = "Software\\VMWSVGA";
/*
static char SVGA_conf_hw_cursor[]  = "HWCursor";
	^ removed, use screen target + ST_CURSOR
*/
static char SVGA_conf_vram_limit[] = "VRAMLimit";
static char SVGA_conf_rgb565bug[]  = "RGB565bug";
static char SVGA_conf_cb[]         = "CommandBuffers";
static char SVGA_conf_pref_fifo[]  = "PreferFIFO";
static char SVGA_conf_hw_version[] = "HWVersion";
static char SVGA_conf_st_size[]    = "STSize";
static char SVGA_conf_st_flags[]   = "STOptions";
static char SVGA_vxd_name[]        = "vmwsmini.vxd";

static char SVGA_conf_disable_multisample[] = "NoMultisample";

/**
 * Notify virtual HW that is some work to do
 **/
void SVGA_Sync()
{
	SVGA_WriteReg(SVGA_REG_SYNC, 1);
}

/* map physical addresses to system linear space */
void SVGA_MapIO()
{
	if(SVGA_IsSVGA3())
	{
		gSVGA.rmmio_linear = _MapPhysToLinear(gSVGA.fifoPhy, gSVGA.fifoSize, 0);
		gSVGA.rmmio = (uint32 FARP*)gSVGA.rmmio_linear;
	}
}

static char db_mutexname[] = "svga_db_mux";

static void SVGA_DB_alloc()
{
	DWORD size;
	BYTE *mem = NULL;
	
	DWORD max_regions = SVGA3D_MAX_MOBS; ///SVGA_ReadReg(SVGA_REG_GMR_MAX_IDS);
	
	DWORD regions_map_size = ((max_regions + 31) >> 3) & 0xFFFFFFFCUL;
	DWORD contexts_map_size = ((SVGA3D_MAX_CONTEXT_IDS + 31) >> 3) & 0xFFFFFFFCUL;
	DWORD surfaces_map_size = ((SVGA3D_MAX_SURFACE_IDS + 31) >> 3) & 0xFFFFFFFCUL;
	
	size = max_regions * sizeof(SVGA_DB_region_t) +
	  SVGA3D_MAX_CONTEXT_IDS * sizeof(SVGA_DB_context_t) +
	  SVGA3D_MAX_SURFACE_IDS * sizeof(SVGA_DB_surface_t) +
	  sizeof(SVGA_DB_t) +
	  regions_map_size + contexts_map_size + surfaces_map_size;
	  
	svga_db = (SVGA_DB_t*)_PageAllocate(RoundToPages(size), PG_VM, ThisVM, 0, 0x0, 0x100000, NULL, PAGEFIXED);
	if(svga_db)
	{
		memset(svga_db, 0, size);
		
		mem = (BYTE*)(svga_db+1);
		
		svga_db->regions = (SVGA_DB_region_t*)mem;
		svga_db->regions_cnt = max_regions;
		mem += svga_db->regions_cnt * sizeof(SVGA_DB_region_t);
		
		svga_db->contexts = (SVGA_DB_context_t*)mem;
		svga_db->contexts_cnt =  SVGA3D_MAX_CONTEXT_IDS;
		mem += svga_db->contexts_cnt * sizeof(SVGA_DB_context_t);
		
		svga_db->surfaces = (SVGA_DB_surface_t*)mem;
		svga_db->surfaces_cnt = SVGA3D_MAX_SURFACE_IDS;
		mem += svga_db->surfaces_cnt * sizeof(SVGA_DB_surface_t);
		
		svga_db->regions_map = (DWORD*)mem;
		mem += regions_map_size;
		
		svga_db->contexts_map = (DWORD*)mem;
		mem += contexts_map_size;
		
		svga_db->surfaces_map = (DWORD*)mem;
	  mem += surfaces_map_size;
		
		memcpy(svga_db->mutexname, &(db_mutexname[0]), sizeof(db_mutexname));
		
		memset(svga_db->regions_map,  0xFF, regions_map_size);
		memset(svga_db->contexts_map, 0xFF, contexts_map_size);
		memset(svga_db->surfaces_map, 0xFF, surfaces_map_size);
			
		svga_db->stat_regions_usage = 0;
	}
}

void SVGA_region_usage_reset()
{
	svga_db->stat_regions_usage = 0;
}

SVGA_DB_surface_t *SVGA_GetSurfaceInfo(DWORD sid)
{
	if(sid > 0 && sid < SVGA3D_MAX_SURFACE_IDS)	
		return &(svga_db->surfaces[sid-1]);
	
	return NULL;
}

SVGA_DB_t *SVGA_DB_setup()
{
	return svga_db;
}

DWORD SVGA_fence_passed()
{
	if(SVGA_IsSVGA3())
	{
		return SVGA_ReadReg(SVGA_REG_FENCE);
	}
	
	return gSVGA.fifoMem[SVGA_FIFO_FENCE];
}

DWORD SVGA_fence_get()
{
	if(fence_next_id == 0)
	{
		/* fence overflow drain all FIFO */  
		SVGA_Flush();
		
		fence_next_id = 1;
		dbg_printf(dbg_fence_overflow);
	}
	
	return fence_next_id++;
}

void SVGA_fence_query(DWORD FBPTR ptr_fence_passed, DWORD FBPTR ptr_fence_last)
{
	if(ptr_fence_passed)
	{
		*ptr_fence_passed = SVGA_fence_passed();
	}
	
	if(ptr_fence_last)
	{
		*ptr_fence_last = fence_next_id-1;
	
		if(*ptr_fence_last == 0)
		{
			*ptr_fence_last = ULONG_MAX;
		}
	}
}

BOOL SVGA_fence_is_passed(DWORD fence_id)
{
	DWORD last_pased;
	DWORD last_fence;
	
	SVGA_fence_query(&last_pased, &last_fence);
	if(fence_id > last_fence)
	{
		return TRUE;
	}
		
	if(fence_id <= last_pased)
	{
		return TRUE;
	}
	
	return FALSE;
}

void SVGA_fence_wait(DWORD fence_id)
{
	for(;;)
	{
		if(SVGA_fence_is_passed(fence_id))
		{
			break;
		}
		
		if(cb_support && cb_context0)
		{
			if(CB_queue_check(NULL))
			{
				/* waiting for fence but command queue is empty */
				SVGA_Flush();
				return;
			}
		}
		SVGA_Sync();
	}
}

void *SVGA_cmd_ptr(DWORD *buf, DWORD *pOffset, DWORD cmd, DWORD cmdsize)
{
	DWORD pp = (*pOffset)/sizeof(DWORD);
	buf[pp] = cmd;
	*pOffset = (*pOffset) + sizeof(DWORD) + cmdsize;
	
	return (void*)(buf + pp + 1);
}

void *SVGA_cmd3d_ptr(DWORD *buf, DWORD *pOffset, DWORD cmd, DWORD cmdsize)
{
	DWORD pp = (*pOffset)/sizeof(DWORD);
	buf[pp] = cmd;
	buf[pp+1] = cmdsize;
	*pOffset = (*pOffset) + 2*sizeof(DWORD) + cmdsize;
	
	return (void*)(buf + pp + 2);
}

#if 0
void SVGA_IRQ_proc()
{
	dbg_printf(dbg_irq);
}

void __declspec(naked) SVGA_IRQ_entry()
{
	_asm
	{
		pushad
		call SVGA_IRQ_proc
		popad
	}
	
	VxDJmp(VPICD, Set_Int_Request);
}
#endif

BOOL SVGA_vxdcmd(DWORD cmd)
{
	switch(cmd)
	{
		case SVGA_CMD_INVALIDATE_FB:
			ST_FB_invalid = TRUE;
			return TRUE;
	}
	
	return FALSE;
}

static DWORD fb_pm16 = 0;
static DWORD st_pm16 = 0;
static DWORD st_address = 0;
static DWORD st_surface_mb = 0;
static DWORD disable_multisample = 0;

BOOL st_useable(DWORD bpp)
{
	if(st_used)
	{
		if(bpp == 32) return TRUE;
			
		if(bpp == 16 && (st_flags & ST_16BPP))
		{
			return TRUE;
		}
	}
	
	return FALSE;
}

static void SVGA_write_driver_id()
{
	if(gSVGA.deviceVersionId >= SVGA_ID_2)
	{
		uint32 caps2 = SVGA_ReadReg(SVGA_REG_CAP2);
		
		if((caps2 & SVGA_CAP2_DX2) != 0)
		{
			SVGA_WriteReg(SVGA_REG_GUEST_DRIVER_ID, SVGA_REG_GUEST_DRIVER_ID_LINUX);

			SVGA_WriteReg(SVGA_REG_GUEST_DRIVER_VERSION1,
				LINUX_VERSION_MAJOR << 24 |
				LINUX_VERSION_PATCHLEVEL << 16 |
				LINUX_VERSION_SUBLEVEL);

			SVGA_WriteReg(SVGA_REG_GUEST_DRIVER_VERSION2,
			  VMWGFX_DRIVER_MAJOR << 24 |
			  VMWGFX_DRIVER_MINOR << 16 |
			  VMWGFX_DRIVER_PATCHLEVEL);

			SVGA_WriteReg(SVGA_REG_GUEST_DRIVER_VERSION3, 0);

			SVGA_WriteReg(SVGA_REG_GUEST_DRIVER_ID, SVGA_REG_GUEST_DRIVER_ID_SUBMIT);
		}
	}
}

/**
 * Init SVGA-II hardware
 * return TRUE on success
 * return FALSE on failure (unsupported HW version or invalid HW)
 **/
BOOL SVGA_init_hw()
{
	/* defaults */
	DWORD conf_vram_limit = 0;
	DWORD conf_rgb565bug = 1;
	DWORD conf_cb = 1;
	DWORD conf_hw_version = SVGA_VERSION_2;
#if 0
	uint8 irq = 0;
#endif

	int rc;
	
	/* configs in registry */
 	RegReadConf(HKEY_LOCAL_MACHINE, SVGA_conf_path, SVGA_conf_vram_limit, &conf_vram_limit);
 	RegReadConf(HKEY_LOCAL_MACHINE, SVGA_conf_path, SVGA_conf_rgb565bug,  &conf_rgb565bug);
 	RegReadConf(HKEY_LOCAL_MACHINE, SVGA_conf_path, SVGA_conf_cb,         &conf_cb); 
 	RegReadConf(HKEY_LOCAL_MACHINE, SVGA_conf_path, SVGA_conf_hw_version, &conf_hw_version);
 	RegReadConf(HKEY_LOCAL_MACHINE, SVGA_conf_path, SVGA_conf_pref_fifo,  &prefer_fifo);
 	
 	RegReadConf(HKEY_LOCAL_MACHINE, SVGA_conf_path, SVGA_conf_st_size,    &st_surface_mb);
 	RegReadConf(HKEY_LOCAL_MACHINE, SVGA_conf_path, SVGA_conf_st_flags,   &st_flags);
 	
 	RegReadConf(HKEY_LOCAL_MACHINE, SVGA_conf_path, SVGA_conf_disable_multisample, &disable_multisample);
 	
 	if(!FBHDA_init_hw())
 	{
 		return FALSE;
 	}
 	
 	dbg_printf(dbg_test, 0);
 	
 	rc = SVGA_Init(FALSE, conf_hw_version);
 	dbg_printf(dbg_SVGA_Init, rc);
 	
	if(rc == 0)
	{
		/* default flags */
		gSVGA.userFlags = 0;
		
		if(conf_rgb565bug)
		{
			gSVGA.userFlags |= SVGA_USER_FLAGS_RGB565_BROKEN;
		}
		
		if(conf_vram_limit > SVGA_MIN_VRAM)
		{
			if(gSVGA.vramSize > conf_vram_limit*(1024*1024))
			{
				gSVGA.vramSize = conf_vram_limit*(1024*1024);
			}
		}
		
		dbg_printf(dbg_Device_Init_proc_succ);
		
		dbg_printf(dbg_mapping_map, gSVGA.fbPhy, gSVGA.fifoPhy);
		
		/* map phys FB to linear */
		gSVGA.fbLinear = _MapPhysToLinear(gSVGA.fbPhy, gSVGA.vramSize, 0);
		
		/* map phys FIFO to linear */
		if(!SVGA_IsSVGA3())
			gSVGA.fifoLinear = _MapPhysToLinear(gSVGA.fifoPhy, gSVGA.fifoSize, 0);
		
		/* system linear addres == PM32 address */
		gSVGA.fbMem = (uint8 *)gSVGA.fbLinear;
		if(!SVGA_IsSVGA3())
			gSVGA.fifoMem = (uint32 *)gSVGA.fifoLinear;
		
		dbg_printf(dbg_mapping);
		dbg_printf(dbg_mapping_map, gSVGA.fbPhy, gSVGA.fbMem);
		dbg_printf(dbg_mapping_map, gSVGA.fifoPhy, gSVGA.fifoMem);
		//dbg_printf(dbg_mapping_map, gSVGA.fifo.bouncePhy, gSVGA.fifo.bounceMem);
		dbg_printf(dbg_siz, sizeof(gSVGA), sizeof(uint8 FARP *));
		
		SVGA_write_driver_id();

#if 0
		irq = SVGA_Install_IRQ();
		if(irq > 0)
		{
			VPICD_IRQ_Descriptor vid;
			memset(&vid, 0, sizeof(vid));
			vid.IRQ_Number      = irq;
			vid.Options         = VPICD_OPT_CAN_SHARE;
			vid.Hw_Int_Proc     = (DWORD)SVGA_IRQ_entry;
			vid.IRET_Time_Out   = 500;
			
			/* JH: OK this is NOT way how catch IRQ, because all PCI video IRQ
			       are catched by fat vdd driver.
			 */
			if(VPICD_Virtualize_IRQ(&vid))
			{
				dbg_printf(dbg_irq_install, irq);
			}
			else
			{
				dbg_printf(dbg_irq_install_fail, irq);
			}
			
			SVGA_WriteReg(SVGA_REG_IRQMASK, SVGA_IRQFLAG_COMMAND_BUFFER);
		}
		else
		{
			dbg_printf(dbg_no_irq);
		}
#endif
		
		/* enable FIFO */
		SVGA_Enable();
		
		/* allocate GB tables, if supported */
		if(SVGA_ReadReg(SVGA_REG_CAPABILITIES) & SVGA_CAP_GBOBJECTS)
		{
			SVGA_OTable_alloc(st_surface_mb > 0);
			gb_support = TRUE;
			dbg_printf(dbg_gb_on);
		}
		
		/* enable command buffers if supported and enabled */
		if(SVGA_ReadReg(SVGA_REG_CAPABILITIES) & (SVGA_CAP_COMMAND_BUFFERS | SVGA_CAP_CMD_BUFFERS_2))
		{
			if(prefer_fifo && !gb_support)
			{
				conf_cb = FALSE;
			}
			
			if(conf_cb)
			{
				cb_support = TRUE;
				dbg_printf(dbg_cb_on);
			}
		}
		
		set_fragmantation_limit();
		
		SVGA_DB_alloc();
		
		/* allocate buffer for enable and disable CB */
		ctlbuf = SVGA_CMB_alloc_size(64);
		
		/* allocate CB for this driver */
		cmdbuf = SVGA_CMB_alloc();
	
		/* vGPU10 */
		if(gb_support)
		{
			SVGA_CB_start();
			
			if(cb_support) /* SVGA_CB_start may fail */
			{
				SVGA_OTable_load();
				
				if(st_surface_mb >= 8)
				{
					if(st_memory_allocate(st_surface_mb*1024*1024, &st_address))
					{
						st_used = TRUE;
					}
				}
			}
		}
		
		/* allocate 16bit selector for FB */
		fb_pm16 = map_pm16(1, gSVGA.fbLinear, gSVGA.vramSize);
		
		/* fill address in FBHDA */
		if(st_used)
		{
			st_pm16 = map_pm16(1, st_address, st_surface_mb*1024*1024);
			
			hda->vram_pm32 = (void*)st_address;
			hda->vram_size = st_surface_mb*1024*1024;
			hda->vram_pm16 = st_pm16;
		}
		else
		{
			hda->vram_pm32 = (void*)gSVGA.fbLinear;
			hda->vram_size = gSVGA.vramSize;
			hda->vram_pm16 = fb_pm16;
		}
		
 		memcpy(hda->vxdname, SVGA_vxd_name, sizeof(SVGA_vxd_name));
		
		hda->flags |= FB_ACCEL_VMSVGA;
		
		if(cb_support && gb_support)
		{
			hda->flags |= FB_ACCEL_VMSVGA10;
		}
			
		cache_init();
				
		SVGA_is_valid = TRUE;
		
		if(_GetFreePageCount(NULL) >= 0x38000UL) /* 896MB / 4096 */
		{
			if(!gb_support) /* mob cache is done by RING-3 DLL */
			{
				cache_enable(TRUE);
			}
		}
				
		return TRUE;
	}
	
	return FALSE;
}

/* Check if screen acceleration is available */
static BOOL SVGA_hasAccelScreen()
{
	if(SVGA_HasFIFOCap(SVGA_FIFO_CAP_SCREEN_OBJECT | SVGA_FIFO_CAP_SCREEN_OBJECT_2))
	{
		return TRUE;
	}
	
	return FALSE;
}

/* from svga3d.c, we need only this function from whole file */
BOOL SVGA3D_Init(void)
{
	SVGA3dHardwareVersion hwVersion;

	if(!(gSVGA.capabilities & SVGA_CAP_EXTENDED_FIFO))
	{
		return FALSE;
	}
	
	if((gSVGA.capabilities & SVGA_CAP_GBOBJECTS))
	{
		if(!SVGA_GetDevCap(SVGA3D_DEVCAP_DXCONTEXT))
		{
			/* don't allow 3D on vGPU without DX context */
			return FALSE;
		}
		hwVersion = SVGA3D_HWVERSION_WS8_B1;
	}
	else if(SVGA_HasFIFOCap(SVGA_FIFO_CAP_3D_HWVERSION_REVISED))
	{
		hwVersion = gSVGA.fifoMem[SVGA_FIFO_3D_HWVERSION_REVISED];
	}
	else
	{
		if(gSVGA.fifoMem[SVGA_FIFO_MIN] <= sizeof(uint32) * SVGA_FIFO_GUEST_3D_HWVERSION)
		{
			return FALSE;
		}
		hwVersion = gSVGA.fifoMem[SVGA_FIFO_3D_HWVERSION];
	}

	/*
	 * Check the host's version, make sure we're binary compatible.
	 */
	if(hwVersion == 0)
	{
		//dbg_printf("3D disabled by host.");
		return FALSE;
	}
	if (hwVersion < SVGA3D_HWVERSION_WS65_B1)
	{
		//dbg_printf("Host SVGA3D protocol is too old, not binary compatible.");
		return FALSE;
	}
	
	return TRUE;
}

DWORD SVGA_pitch(DWORD width, DWORD bpp)
{
	DWORD bp = (bpp + 7) / 8;
	return (bp * width + 3) & 0xFFFFFFFC;
}

static void SVGA_defineScreen(DWORD w, DWORD h, DWORD bpp)
{
  SVGAFifoCmdDefineScreen *screen;
  SVGAFifoCmdDefineGMRFB  *fbgmr;
  DWORD cmdoff = 0;
  
  wait_for_cmdbuf();
  
  screen = SVGA_cmd_ptr(cmdbuf, &cmdoff, SVGA_CMD_DEFINE_SCREEN, sizeof(SVGAFifoCmdDefineScreen));

	memset(screen, 0, sizeof(SVGAFifoCmdDefineScreen));
	screen->screen.structSize = sizeof(SVGAScreenObject);
	screen->screen.id = 0;
	screen->screen.flags = SVGA_SCREEN_MUST_BE_SET | SVGA_SCREEN_IS_PRIMARY;
	screen->screen.size.width = w;
	screen->screen.size.height = h;
	screen->screen.root.x = 0;
	screen->screen.root.y = 0;
	screen->screen.cloneCount = 0;

	if(bpp < 32)
	{
		screen->screen.backingStore.pitch = SVGA_pitch(w, bpp);
	}
	
	screen->screen.backingStore.ptr.offset = 0;
	screen->screen.backingStore.ptr.gmrId = SVGA_GMR_FRAMEBUFFER;

  /* set GMR to same location as screen */
  if(bpp >= 15/* || bpp < 32*/)
  {
  	fbgmr = SVGA_cmd_ptr(cmdbuf, &cmdoff, SVGA_CMD_DEFINE_GMRFB, sizeof(SVGAFifoCmdDefineGMRFB));
	  
	  if(fbgmr)
	  {
	  	fbgmr->ptr.gmrId = SVGA_GMR_FRAMEBUFFER;
	  	fbgmr->ptr.offset = 0;
	  	fbgmr->bytesPerLine = SVGA_pitch(w, bpp);
	  	fbgmr->format.colorDepth = bpp;
	  	if(bpp >= 24)
	  	{
	  		fbgmr->format.bitsPerPixel = 32;
	  		fbgmr->format.colorDepth   = 24;
	  		fbgmr->format.reserved     = 0;
	  	}
	  	else
	  	{
	  		fbgmr->format.bitsPerPixel = 16;
	  		fbgmr->format.colorDepth   = 16;
	  		fbgmr->format.reserved     = 0;
	  	}
	  }
	}
	
	submit_cmdbuf(cmdoff, SVGA_CB_SYNC/*|SVGA_CB_FORCE_FIFO*/, 0);
}


/**
 * Test if display mode is supported
 **/
BOOL SVGA_validmode(DWORD w, DWORD h, DWORD bpp)
{
	switch(bpp)
	{
		case 8:
		case 16:
		case 32:
			break;
		case 24:
			if(!st_useable(24)) return FALSE;
			break;
		default:
			return FALSE;
	}
	
	if(w <= SVGA_ReadReg(SVGA_REG_MAX_WIDTH))
	{
		if(h <= SVGA_ReadReg(SVGA_REG_MAX_HEIGHT))
		{
			DWORD size = SVGA_pitch(w, bpp) * h;
			if(size < hda->vram_size)
			{
				if(bpp != 32 && size > SVGA_FB_MAX_TRACEABLE_SIZE)
				{
					return FALSE;
				}
				
				return TRUE;
			}
		}
	}
	
	return FALSE;
}

/**
 * Set display mode
 *
 **/
BOOL SVGA_setmode(DWORD w, DWORD h, DWORD bpp)
{
	BOOL has3D;
	if(!SVGA_validmode(w, h, bpp))
	{
		return FALSE;
	}
	
	/* free chached regions */
	/*
	SVGA_flushcache();
	 ^JH: no alloc/free operations here (don't know why,
				but when VXD si called from 16 bits, these
				operations are bit unstable)
	*/
	
	mouse_invalidate();
	FBHDA_access_begin(0);
	
	Begin_Critical_Section(0);
	//SVGA_OTable_unload(); // unload otables
	
	/* Make sure, that we drain full FIFO */
	SVGA_Sync();
	SVGA_Flush_CB_critical(); 
	
#if 0
	/* delete old screen at its objects */
	if(SVGA_hasAccelScreen())
	{
		if(st_used)
		{
			st_destroyScreen();
			SVGA_Sync();
			SVGA_Flush_CB_critical(); 
		}
	}
	/* JH: OK, we should normally clean after ourselves,
	       but VirtualBox GUI can crash if there are
	       no surface to draw. Anyway, when we touch legacy
	       screen registers all screen objects will be
	       reset.
	 */
#endif

	/* stop command buffer context 0 */
	SVGA_CB_stop();
  
   /* setup by legacy registry */
  if(st_useable(bpp))
  {
		SVGA_SetModeLegacy(w, h, 32);
	}
	else
	{
		SVGA_SetModeLegacy(w, h, bpp);
	}
	/* VMware, vGPU10: OK, when screen has change, whoale GPU is reset including FIFO */
	SVGA_Enable();
	
	SVGA_Flush_CB_critical(); /* make sure, that is really set */

	/* setting screen by fifo, this method is required in VB 6.1 */
	if(SVGA_hasAccelScreen())
	{
		if(!st_useable(bpp))
		{
			SVGA_defineScreen(w, h, bpp);
			
			/* reenable fifo */
			SVGA_Enable();
		}
		
		SVGA_Flush_CB_critical();
	}
	
	/* start command buffer context 0 */
	SVGA_CB_start();
	
	has3D = SVGA3D_Init();
	SVGA_Sync();

	SVGA_Flush_CB_critical();
	
	if(st_useable(bpp))
	{
		SVGA_CB_stop();
		st_defineScreen(w, h, bpp);
		hda->flags |= FB_ACCEL_VMSVGA10_ST;
		
		SVGA_CB_start();
	}
	else
	{
		hda->flags &= ~((DWORD)FB_ACCEL_VMSVGA10_ST);
	}
	
	if(st_useable(bpp))
	{
		hda->vram_pm32 = (void*)st_address;
		hda->vram_size = st_surface_mb*1024*1024;
		hda->vram_pm16 = st_pm16;
	}
	else
	{
		hda->vram_pm32 = (void*)gSVGA.fbLinear;
		hda->vram_size = gSVGA.vramSize;
		hda->vram_pm16 = fb_pm16;
	}

	hda->width   = SVGA_ReadReg(SVGA_REG_WIDTH);
	hda->height  = SVGA_ReadReg(SVGA_REG_HEIGHT);
	if(st_useable(bpp))
	{
		/*
		 when screen target is used, screen bpp and pitch may be different from 
		 backscreen surface.
		 */
		hda->bpp     = bpp;
		hda->pitch   = SVGA_pitch(hda->width, bpp);
	}
	else
	{
		hda->bpp     = SVGA_ReadReg(SVGA_REG_BITS_PER_PIXEL);
		hda->pitch   = SVGA_ReadReg(SVGA_REG_BYTES_PER_LINE);
	}
	hda->stride  = hda->height * hda->pitch;
	hda->surface = 0;

	if(has3D && SVGA_GetDevCap(SVGA3D_DEVCAP_3D) > 0)
	{
		hda->flags |= FB_ACCEL_VMSVGA3D;
	}
	else
	{
		hda->flags &= ~((DWORD)FB_ACCEL_VMSVGA3D);
	}
	
	End_Critical_Section();
	
	mouse_invalidate();
	FBHDA_access_end(0);

  return TRUE;
}

/*
 * Fix SVGA caps which are known as bad:
 *  - VirtualBox returning A4R4G4B4 instead of R5G6B5,
 *    try to guess it's value from X8R8G8B8.
 *
 */
DWORD SVGA_FixDevCap(DWORD cap_id, DWORD cap_val)
{
	switch(cap_id)
	{
		case SVGA3D_DEVCAP_SURFACEFMT_R5G6B5:
		{
			if(gSVGA.userFlags & SVGA_USER_FLAGS_RGB565_BROKEN)
			{
				DWORD xrgb8888 = SVGA_GetDevCap(SVGA3D_DEVCAP_SURFACEFMT_X8R8G8B8);
				xrgb8888 &= ~SVGA3DFORMAT_OP_SRGBWRITE; /* nvidia */
				return xrgb8888;
			}
			else
			{
				if(cap_val & SVGA3DFORMAT_OP_SAME_FORMAT_UP_TO_ALPHA_RENDERTARGET) /* VirtualBox BUG */
				{
					DWORD xrgb8888 = SVGA_GetDevCap(SVGA3D_DEVCAP_SURFACEFMT_X8R8G8B8);
					xrgb8888 &= ~SVGA3DFORMAT_OP_SRGBWRITE; /* nvidia */
					return xrgb8888;
				}
				
				if((cap_val & SVGA3DFORMAT_OP_3DACCELERATION) == 0) /* VirtualBox BUG, older drivers */
				{
					DWORD xrgb8888 = SVGA_GetDevCap(SVGA3D_DEVCAP_SURFACEFMT_X8R8G8B8);
					xrgb8888 &= ~SVGA3DFORMAT_OP_SRGBWRITE; /* nvidia */
					if(xrgb8888 & SVGA3DFORMAT_OP_3DACCELERATION)
					{
						return xrgb8888;
					}
				}
			}
			break;
		}
		case SVGA3D_DEVCAP_MULTISAMPLE_2X:
		case SVGA3D_DEVCAP_MULTISAMPLE_4X:
		case SVGA3D_DEVCAP_MULTISAMPLE_8X:
			if(disable_multisample)
			{
				return 0;
			}
			break;
		
	}
	
	return cap_val;
}

/**
 * Read HW cap, supported old way (GPU gen9):
 *   struct SVGA_FIFO_3D_CAPS in FIFO,
 * and new way (GPU gen10):
 *   SVGA_REG_DEV_CAP HW register
 *
 **/
DWORD SVGA_GetDevCap(DWORD search_id)
{
	if (gSVGA.capabilities & SVGA_CAP_GBOBJECTS)
	{
		/* new way to read device CAPS */
		SVGA_WriteReg(SVGA_REG_DEV_CAP, search_id);
		return SVGA_FixDevCap(search_id, SVGA_ReadReg(SVGA_REG_DEV_CAP));
	}
	else
	{
		SVGA3dCapsRecord *pCaps = (SVGA3dCapsRecord *)&(gSVGA.fifoMem[SVGA_FIFO_3D_CAPS]);  
		while(pCaps->header.length != 0)
		{
		  if(pCaps->header.type == SVGA3DCAPS_RECORD_DEVCAPS)
		  {
		   	DWORD datalen = (pCaps->header.length - 2)/2;
	    	SVGA3dCapPair *pData = (SVGA3dCapPair *)(&pCaps->data);
	    	DWORD i;
	    	
		 		for(i = 0; i < datalen; i++)
		 		{
		  		DWORD id  = pData[i][0];
		  		DWORD val = pData[i][1];
		  			
		  		if(id == search_id)
					{
						return SVGA_FixDevCap(search_id, val);
					}
				}
			}
			pCaps = (SVGA3dCapsRecord *)((DWORD *)pCaps + pCaps->header.length);
		}
	}
	
	return 0;
}

DWORD SVGA_query(DWORD type, DWORD index)
{
	switch(type)
	{
		case SVGA_QUERY_REGS:
			if(index >= 256) break;			
			return SVGA_ReadReg(index);
		case SVGA_QUERY_FIFO:
			if(index >= 1024) break;
			return gSVGA.fifoMem[index];
		case SVGA_QUERY_CAPS:
			if(index >= 512) break;
			return SVGA_GetDevCap(index);
	}
	
	return ~0x0;
}

void SVGA_query_vector(DWORD type, DWORD index_start, DWORD count, DWORD *out)
{
	DWORD i;
	for(i = 0; i < count; i++)
	{
		out[i] = SVGA_query(type, index_start+i);
	}
}

void SVGA_HW_enable()
{
	if(hda->width > 0 && hda->height > 0)
	{
		SVGA_WriteReg(SVGA_REG_ENABLE, TRUE);
		
		SVGA_CB_start();
	}
}

void SVGA_HW_disable()
{
	dbg_printf(dbg_disable);
	
	SVGA_CB_stop();
	
	SVGA_Disable();
}

BOOL SVGA_valid()
{
	return SVGA_is_valid;
}

BOOL FBHDA_swap(DWORD offset)
{
	return FALSE;
}

void FBHDA_access_begin(DWORD flags)
{
	FBHDA_access_rect(0, 0, hda->width, hda->height);
}

static DWORD rect_left;
static DWORD rect_top;
static DWORD rect_right;
static DWORD rect_bottom;

void FBHDA_access_rect(DWORD left, DWORD top, DWORD right, DWORD bottom)
{
	Wait_Semaphore(hda_sem, 0);
	
	if(fb_lock_cnt++ == 0)
	{
		BOOL readback = FALSE;
				
		if(present_fence != 0)
		{
			SVGA_fence_wait(present_fence);
//			readback = TRUE;
		}
		
		if(ST_FB_invalid)
		{
			readback = TRUE;		
			ST_FB_invalid = FALSE;
		}
		
		if(readback)
		{
			if(st_useable(hda->bpp))
			{
				DWORD cmd_offset = 0;
				SVGA3dCmdReadbackGBSurface *gbreadback;
				
				wait_for_cmdbuf();
				
				gbreadback = SVGA_cmd3d_ptr(cmdbuf, &cmd_offset, SVGA_3D_CMD_READBACK_GB_SURFACE, sizeof(SVGA3dCmdReadbackGBSurface));
				gbreadback->sid = ST_SURFACE_ID;
				submit_cmdbuf(cmd_offset, SVGA_CB_SYNC, 0);
			}
		}
		
		mouse_erase();
		
		rect_left   = left;
		rect_top    = top;
		rect_right  = right;
		rect_bottom = bottom;
	}
	else
	{
		if(left < rect_left)
			rect_left = left;
		
		if(top < rect_top)
			rect_top = top;
		
		if(right > rect_right)
			rect_right = right;
		
		if(bottom > rect_bottom)
			rect_bottom = bottom;
	}
	
	Signal_Semaphore(hda_sem);
}

void FBHDA_access_end(DWORD flags)
{
	Wait_Semaphore(hda_sem, 0);

	fb_lock_cnt--;
	if(fb_lock_cnt < 0) fb_lock_cnt = 0;
	
	if(fb_lock_cnt == 0)
	{
		mouse_blit();		
		if(st_useable(hda->bpp))
		{
			SVGA3dCmdUpdateGBSurface *gbupdate;
			SVGA3dCmdUpdateGBScreenTarget *stupdate;
			SVGA3dCmdUpdateGBImage   *gbupdate_rect;
			DWORD cmd_offset = 0;
			DWORD w = rect_right - rect_left;
			DWORD h = rect_bottom - rect_top;

		 	wait_for_cmdbuf();

			if(w == hda->width && h == hda->height)
			{
				/* full screen update */
				gbupdate = SVGA_cmd3d_ptr(cmdbuf, &cmd_offset, SVGA_3D_CMD_UPDATE_GB_SURFACE, sizeof(SVGA3dCmdUpdateGBSurface));
				gbupdate->sid = ST_SURFACE_ID;
			}
			else
			{
				/* partial box */
				gbupdate_rect = SVGA_cmd3d_ptr(cmdbuf, &cmd_offset, SVGA_3D_CMD_UPDATE_GB_IMAGE, sizeof(SVGA3dCmdUpdateGBImage));
				gbupdate_rect->image.sid = ST_SURFACE_ID;
				gbupdate_rect->image.face = 0;
				gbupdate_rect->image.mipmap = 0;
				gbupdate_rect->box.x = rect_left;
				gbupdate_rect->box.y = rect_top;
				gbupdate_rect->box.z = 0;
				gbupdate_rect->box.w = w;
				gbupdate_rect->box.h = h;
				gbupdate_rect->box.d = 1;
			}

			stupdate = SVGA_cmd3d_ptr(cmdbuf, &cmd_offset, SVGA_3D_CMD_UPDATE_GB_SCREENTARGET, sizeof(SVGA3dCmdUpdateGBScreenTarget));
			stupdate->stid = 0;
	  	stupdate->rect.x = rect_left;
	  	stupdate->rect.y = rect_top;
	  	stupdate->rect.w = w;
	  	stupdate->rect.h = h;

			submit_cmdbuf(cmd_offset, SVGA_CB_PRESENT_ASYNC, 0);
		}
	  else if(hda->bpp == 32)
	  {
	  	SVGAFifoCmdUpdate  *cmd_update;
	  	DWORD cmd_offset = 0;

	  	wait_for_cmdbuf();

	  	cmd_update = SVGA_cmd_ptr(cmdbuf, &cmd_offset, SVGA_CMD_UPDATE, sizeof(SVGAFifoCmdUpdate));
	  	cmd_update->x = 0;
	  	cmd_update->y = 0;
	  	cmd_update->width  = hda->width;
	  	cmd_update->height = hda->height;
	  	
			submit_cmdbuf(cmd_offset, SVGA_CB_PRESENT_ASYNC, 0);
	  }
	} // fb_lock_cnt == 0
	
	Signal_Semaphore(hda_sem);
}

void  FBHDA_palette_set(unsigned char index, DWORD rgb)
{
	UINT sIndex = SVGA_PALETTE_BASE + index*3;
	
  SVGA_WriteReg(sIndex+0, (rgb >> 16) & 0xFF);
  SVGA_WriteReg(sIndex+1, (rgb >>  8) & 0xFF);
  SVGA_WriteReg(sIndex+2,  rgb        & 0xFF);
}

DWORD FBHDA_palette_get(unsigned char index)
{
	UINT sIndex = SVGA_PALETTE_BASE + index*3;
	
	return ((SVGA_ReadReg(sIndex+0) & 0xFF) << 16) |
		((SVGA_ReadReg(sIndex+1) & 0xFF) << 8) |
		 (SVGA_ReadReg(sIndex+2) & 0xFF);
}
