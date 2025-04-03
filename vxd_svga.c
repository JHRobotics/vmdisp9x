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

#include "vxd_color.h"

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
SVGA_DB_t *svga_db = NULL;

extern LONG fb_lock_cnt;

       BOOL gb_support = FALSE;
       BOOL cb_support = FALSE;
       BOOL cb_context0 = FALSE;

/* for GPU9 is FIFO more stable (VMWARE) or faster (VBOX) */
static DWORD prefer_fifo = 1;

static BOOL SVGA_is_valid = FALSE;

BOOL surface_dirty = FALSE;

static DWORD fence_next_id = 1;
void *cmdbuf = NULL;
void *ctlbuf = NULL;

DWORD async_mobs = 1;
DWORD hw_cursor  = 0;

ULONG cb_sem = 0;
ULONG mem_sem = 0;

/* vxd_mouse.vxd */
BOOL mouse_get_rect(DWORD *ptr_left, DWORD *ptr_top,
	DWORD *ptr_right, DWORD *ptr_bottom);


//DWORD present_fence = 0;
/*
 * strings
 */
static char SVGA_conf_path[] = "Software\\vmdisp9x\\svga";
static char SVGA_conf_hw_cursor[]  = "HWCursor";
/*	^ recovered */
static char SVGA_conf_vram_limit[] = "VRAMLimit";
static char SVGA_conf_rgb565bug[]  = "RGB565bug";
static char SVGA_conf_cb[]         = "CommandBuffers";
static char SVGA_conf_pref_fifo[]  = "PreferFIFO";
static char SVGA_conf_hw_version[] = "HWVersion";
static char SVGA_conf_disable_multisample[] = "NoMultisample";
static char SVGA_conf_reg_multisample[] = "RegMultisample";
static char SVGA_conf_async_mobs[] = "AsyncMOBs";

static char SVGA_vxd_name[]        = "vmwsmini.vxd";

svga_saved_state_t svga_saved_state = {FALSE};

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
	
	dbg_printf("SVGA_DB alloc, mem_usage: %ld\n", svga_db->stat_regions_usage);
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

#ifndef DBGPRINT
void SVGA_fence_wait(DWORD fence_id)
#else
void SVGA_fence_wait_dbg(DWORD fence_id, int line)
#endif
{
//	dbg_printf(dbg_fence_wait, fence_id, line);
	
	for(;;)
	{
		if(SVGA_fence_is_passed(fence_id))
		{
			break;
		}
		
#if 1
		if(cb_support && cb_context0)
		{
			if(CB_queue_check(NULL))
			{
				/* waiting for fence but command queue is empty */
				SVGA_Flush();
				return;
			}
		}
#endif
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

BOOL SVGA_vxdcmd(DWORD cmd, DWORD arg)
{
	switch(cmd)
	{
		case SVGA_CMD_INVALIDATE_FB:
			return TRUE;
		case SVGA_CMD_CLEANUP:
			SVGA_ProcessCleanup(arg);
			return TRUE;
	}
	
	return FALSE;
}

static DWORD fb_pm16 = 0;
//static DWORD st_pm16 = 0;
//static DWORD st_address = 0;
static DWORD st_surface_mb = 0;
static DWORD disable_multisample = 0;
static DWORD reg_multisample = 0;

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

	/* some semaphores */
	mem_sem = Create_Semaphore(1);
	cb_sem = Create_Semaphore(1);

	/* configs in registry */
 	RegReadConf(HKEY_LOCAL_MACHINE, SVGA_conf_path, SVGA_conf_vram_limit, &conf_vram_limit);
 	RegReadConf(HKEY_LOCAL_MACHINE, SVGA_conf_path, SVGA_conf_rgb565bug,  &conf_rgb565bug);
 	RegReadConf(HKEY_LOCAL_MACHINE, SVGA_conf_path, SVGA_conf_cb,         &conf_cb); 
 	RegReadConf(HKEY_LOCAL_MACHINE, SVGA_conf_path, SVGA_conf_hw_version, &conf_hw_version);
 	RegReadConf(HKEY_LOCAL_MACHINE, SVGA_conf_path, SVGA_conf_pref_fifo,  &prefer_fifo);

 	RegReadConf(HKEY_LOCAL_MACHINE, SVGA_conf_path, SVGA_conf_disable_multisample, &disable_multisample);
 	RegReadConf(HKEY_LOCAL_MACHINE, SVGA_conf_path, SVGA_conf_reg_multisample, &reg_multisample);

 	RegReadConf(HKEY_LOCAL_MACHINE, SVGA_conf_path, SVGA_conf_async_mobs, &async_mobs);
 	RegReadConf(HKEY_LOCAL_MACHINE, SVGA_conf_path, SVGA_conf_hw_cursor,  &hw_cursor);
 	
 	if(async_mobs < 1)
 		async_mobs = 1;
 	
 	if(async_mobs >= SVGA_CB_MAX_QUEUED_PER_CONTEXT)
 		async_mobs = SVGA_CB_MAX_QUEUED_PER_CONTEXT-1;
 	
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
		if(reg_multisample)
		{
			SVGA_WriteReg(SVGA_REG_MSHINT, reg_multisample);
			dbg_printf("Value of SVGA_REG_MSHINT: %lX!\n", SVGA_ReadReg(SVGA_REG_MSHINT));
		}
		
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
		
		/* special set for faster MOB define */
		mob_cb_alloc();
	
		/* vGPU10 */
		if(gb_support)
		{
			SVGA_CB_start();
			
			if(cb_support) /* SVGA_CB_start may fail */
			{
				SVGA_OTable_load();
			}
		}
		
		/* allocate 16bit selector for FB */
		fb_pm16 = map_pm16(1, gSVGA.fbLinear, gSVGA.vramSize);
		
		/* fill address in FBHDA */
		hda->vram_pm32 = (void*)gSVGA.fbLinear;
		hda->vram_size = gSVGA.vramSize;
		hda->vram_pm16 = fb_pm16;
		
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
		
		/* switch back to VGA mode, SVGA mode will be request by 16 bit driver later */
		SVGA_HW_disable();
		
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
	if(hwVersion < SVGA3D_HWVERSION_WS65_B1)
	{
		//dbg_printf("Host SVGA3D protocol is too old, not binary compatible.");
		return FALSE;
	}
	
	return TRUE;
}

DWORD SVGA_pitch(DWORD width, DWORD bpp)
{
	DWORD bp = (bpp + 7) / 8;
	return (bp * width + (FBHDA_ROW_ALIGN-1)) & (~((DWORD)FBHDA_ROW_ALIGN-1));
}

static DWORD SVGA_DT_stride(DWORD w, DWORD h)
{
	DWORD stride = SVGA_pitch(w, 32) * h;
	
	return (stride + 65535) & 0xFFFF0000UL;
}

/**
 *
 * @return: screen offset to VRAM
 **/
static void SVGA_defineScreen(DWORD w, DWORD h, DWORD bpp, DWORD offset)
{
  SVGAFifoCmdDefineScreen *screen;
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

	screen->screen.backingStore.pitch = SVGA_pitch(w, bpp);
	screen->screen.backingStore.ptr.offset = 0; /* vmware, always 0 for primary surface  */
	screen->screen.backingStore.ptr.gmrId = SVGA_GMR_FRAMEBUFFER; /* must be framebuffer */
	
	submit_cmdbuf(cmdoff, SVGA_CB_SYNC|SVGA_CB_FORCE_FIFO, 0);
}

static void SVGA_FillGMRFB(SVGAFifoCmdDefineGMRFB *fbgmr, 
	DWORD offset, DWORD pitch, DWORD bpp)
{
	fbgmr->ptr.gmrId = SVGA_GMR_FRAMEBUFFER;
	fbgmr->ptr.offset = offset;
	fbgmr->bytesPerLine = pitch;
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

static void SVGA_DefineGMRFB()
{
	SVGAFifoCmdDefineGMRFB *gmrfb;
	DWORD cmd_offset = 0;

	wait_for_cmdbuf();
	  	
	gmrfb = SVGA_cmd_ptr(cmdbuf, &cmd_offset, SVGA_CMD_DEFINE_GMRFB, sizeof(SVGAFifoCmdDefineGMRFB));
	SVGA_FillGMRFB(gmrfb, hda->surface, hda->pitch, hda->bpp);
	submit_cmdbuf(cmd_offset, 0, 0);
	
	dbg_printf("SVGA_DefineGMRFB: %ld\n", hda->surface);
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

static void SVGA_setmode_phy(DWORD w, DWORD h, DWORD bpp)
{
	//SVGA_OTable_unload(); // unload otables
	
	/* Make sure, that we drain full FIFO */
	SVGA_Sync();
	SVGA_Flush_CB(); 
	
#if 0
	/* delete old screen at its objects */
	if(SVGA_hasAccelScreen())
	{
		if(st_used)
		{
			st_destroyScreen();
			SVGA_Sync();
			SVGA_Flush_CB(); 
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
  if(SVGA_hasAccelScreen())
  {
		SVGA_SetModeLegacy(w, h, 32);
	}
	else
	{
		SVGA_SetModeLegacy(w, h, bpp);
	}
	
	/* VMware, vGPU10: OK, when screen has change, whoale GPU is reset including FIFO */
	SVGA_Enable();
	
	SVGA_Flush(); /* make sure, that is really set */

	/* setting screen by fifo, this method is required in VB 6.1 */
	if(SVGA_hasAccelScreen())
	{
		SVGA_defineScreen(w, h, bpp, 0);
			
		/* reenable fifo */
		SVGA_Enable();
		
		SVGA_Flush();
		
		/* TODO, not do this on vGPU10 */
	}
	SVGA_Sync();
	
	/* start command buffer context 0 */
	SVGA_CB_start();
	
	SVGA_Sync();
	SVGA_Flush_CB();
}

/* clear both physical screen and system surface */
void SVGA_clear()
{
	if(hda->system_surface)
	{
		memset((BYTE*)hda->vram_pm32 + hda->system_surface, 0, hda->height*hda->pitch);
		memset(hda->vram_pm32, 0, SVGA_pitch(hda->width, 32)*hda->height);
	}
	else
	{
		memset(hda->vram_pm32, 0, hda->height*hda->pitch);
	}
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
	
	svga_saved_state.width = w;
	svga_saved_state.height = h;
	svga_saved_state.bpp = bpp;
	svga_saved_state.enabled = TRUE;

	/* when on overlay silently eat change mode requests */
	if(hda->overlay > 0)
	{
		return TRUE;
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
	
	SVGA_setmode_phy(w, h, bpp);
		
	has3D = SVGA3D_Init();
	
	hda->flags &= ~((DWORD)FB_SUPPORT_FLIPING);
	
	hda->flags &= ~((DWORD)FB_ACCEL_VMSVGA10_ST);

	hda->vram_pm32 = (void*)gSVGA.fbLinear;
	hda->vram_size = gSVGA.vramSize;
	hda->vram_pm16 = fb_pm16;

	hda->width   = w;//SVGA_ReadReg(SVGA_REG_WIDTH);
	hda->height  = h;//SVGA_ReadReg(SVGA_REG_HEIGHT);
	
	if(SVGA_hasAccelScreen())
	{
		if(bpp >= 8)
		{
			DWORD offset = SVGA_DT_stride(hda->width, hda->height);
			
			hda->bpp     = bpp;
			hda->pitch   = SVGA_pitch(hda->width, bpp);
			hda->system_surface = offset;
			hda->surface = offset;
		}
		else
		{
			hda->bpp     = SVGA_ReadReg(SVGA_REG_BITS_PER_PIXEL);
			hda->pitch   = SVGA_ReadReg(SVGA_REG_BYTES_PER_LINE);
			hda->surface = 0;
			hda->system_surface = 0;
		}
	}
	else
	{
		hda->bpp     = SVGA_ReadReg(SVGA_REG_BITS_PER_PIXEL);
		hda->pitch   = SVGA_ReadReg(SVGA_REG_BYTES_PER_LINE);
		hda->surface = 0;
		hda->system_surface = 0;
	}
	hda->stride  = hda->height * hda->pitch;
	
	if(has3D && SVGA_GetDevCap(SVGA3D_DEVCAP_3D) > 0)
	{
		hda->flags |= FB_ACCEL_VMSVGA3D;
	}
	else
	{
		hda->flags &= ~((DWORD)FB_ACCEL_VMSVGA3D);
	}
	
	if(hda->system_surface > 0)
	{
		hda->flags |= FB_SUPPORT_FLIPING;
		SVGA_DefineGMRFB();
	}
	
	SVGA_clear();
	
	mouse_invalidate();
	FBHDA_access_end(0);

	fb_lock_cnt = 0; // reset lock counters

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
	dbg_printf("SVGA_HW_enable()\n");
	
	if(hda->width > 0 && hda->height > 0)
	{
		SVGA_WriteReg(SVGA_REG_ENABLE, TRUE);
		
		SVGA_CB_start();
		
		svga_saved_state.enabled = TRUE;
	}
}

void SVGA_HW_disable()
{
	dbg_printf("SVGA_HW_disable()\n");
	
	SVGA_CB_stop();
	
	SVGA_Disable();
	
	svga_saved_state.enabled = FALSE;
}

BOOL SVGA_valid()
{
	return SVGA_is_valid;
}

BOOL FBHDA_swap(DWORD offset)
{
	BOOL rc = FALSE;

	if(hda->overlay > 0)
	{
		/* on overlay emulate behaviour */
		if(offset >= hda->system_surface && hda->bpp > 8)
		{
			hda->surface = offset;
			return TRUE;
		}
		return FALSE;
	}
	
	if(offset >= hda->system_surface) /* DON'T touch surface 0 */
	{
	 	FBHDA_access_begin(0);
	 	if(hda->bpp > 8)
	 	{
	  	hda->surface = offset;
	 		SVGA_DefineGMRFB();
	  	rc = TRUE;
		}
	  FBHDA_access_end(0);
	}

	return rc;
}

static DWORD rect_left;
static DWORD rect_top;
static DWORD rect_right;
static DWORD rect_bottom;

static inline void update_rect(DWORD left, DWORD top, DWORD right, DWORD bottom)
{
	if(left < rect_left)
		rect_left = left;

	if(top < rect_top)
		rect_top = top;

	if(right > rect_right)
	{
		rect_right = right;
		if(rect_right > hda->width)
		{
			rect_right = hda->width;
		}
	}

	if(bottom > rect_bottom)
	{
		rect_bottom = bottom;
		if(rect_bottom > hda->height)
		{
			rect_bottom = hda->height;
		}
	}
}

static inline void check_dirty()
{
	if(surface_dirty)
	{
		switch(hda->bpp)
		{
			case 32:
			{
			 	SVGAFifoCmdBlitScreenToGMRFB *gmrblit;
			 	DWORD cmd_offset = 0;
		
				wait_for_cmdbuf();
						
				gmrblit = SVGA_cmd_ptr(cmdbuf, &cmd_offset, SVGA_CMD_BLIT_SCREEN_TO_GMRFB, sizeof(SVGAFifoCmdBlitScreenToGMRFB));
	
				gmrblit->destOrigin.x    = 0;
				gmrblit->destOrigin.y    = 0;
				gmrblit->srcRect.left    = 0;
				gmrblit->srcRect.top     = 0;
				gmrblit->srcRect.right   = hda->width;
				gmrblit->srcRect.bottom  = hda->height;
				gmrblit->srcScreenId = 0;
				  	
				submit_cmdbuf(cmd_offset, SVGA_CB_UPDATE, 0);
				break;
			}
			case 16:
			{
				readback16(
					hda->vram_pm32, SVGA_pitch(hda->width, 32),
					((BYTE*)hda->vram_pm32)+hda->surface, hda->pitch,
					0, 0, hda->width, hda->height
				);
				break;
			}
		} // switch
		
		surface_dirty = FALSE;
	}
}

void FBHDA_access_rect(DWORD left, DWORD top, DWORD right, DWORD bottom)
{
	if(hda->overlay > 0)
	{
		return;
	}
	
	Wait_Semaphore(hda_sem, 0);
	
	if(left > hda->width)
	{
		Signal_Semaphore(hda_sem);
		return;
	}
	
	if(top > hda->height)
	{
		Signal_Semaphore(hda_sem);
		return;
	}
	
	if(right > hda->width)
		right = hda->width;
	
	if(bottom > hda->height)
		bottom = hda->height;

	if(fb_lock_cnt++ == 0)
	{
		SVGA_CMB_wait_update();
		check_dirty();
		
		rect_left   = left;
		rect_top    = top;
		rect_right  = right;
		rect_bottom = bottom;

		mouse_erase();

		if(mouse_get_rect(&left, &top, &right, &bottom))
		{
			update_rect(left, top, right, bottom);
		}
	}
	else
	{
		update_rect(left, top, right, bottom);
	}

	Signal_Semaphore(hda_sem);
}

void FBHDA_access_begin(DWORD flags)
{
	if(hda->overlay > 0)
	{
		return;
	}
	
	if(flags & (FBHDA_ACCESS_RAW_BUFFERING | FBHDA_ACCESS_MOUSE_MOVE))
	{
		Wait_Semaphore(hda_sem, 0);
		
//		dbg_printf("FBHDA_access_begin(%ld)\n", flags);
		
		if(fb_lock_cnt++ == 0)
		{
			SVGA_CMB_wait_update();
			mouse_erase();
			check_dirty();
			
			if(!mouse_get_rect(&rect_left, &rect_top, &rect_right, &rect_bottom))
			{
				rect_left   = 0;
				rect_top    = 0;
				rect_right  = 0;
				rect_bottom = 0;
			}
		}
		else
		{
			DWORD l, t, r, b;
			
			if(mouse_get_rect(&l, &t, &r, &b))
			{
				update_rect(l, t, r, b);
			}
		}
		
		Signal_Semaphore(hda_sem);
	}
	else
	{
		FBHDA_access_rect(0, 0, hda->width, hda->height);
	}
}

void FBHDA_access_end(DWORD flags)
{
	//dbg_printf("+++ FBHDA_access_end: %ld\n", fb_lock_cnt);
	
	if(hda->overlay > 0)
	{
		return;
	}

	Wait_Semaphore(hda_sem, 0);

	if(flags & FBHDA_ACCESS_SURFACE_DIRTY)
	{
		surface_dirty = TRUE;
	}

	if(flags & FBHDA_ACCESS_MOUSE_MOVE)
	{
		DWORD l, t, r, b;
		
		if(mouse_get_rect(&l, &t, &r, &b))
		{
			update_rect(l, t, r, b);
		}
	}

	if(--fb_lock_cnt <= 0)
	{
		DWORD w, h;
		BOOL need_refresh = ((hda->bpp == 32) && (hda->system_surface == 0));
		
		fb_lock_cnt = 0;
		
		w = rect_right - rect_left;
		h = rect_bottom - rect_top;
		
/*		dbg_printf("FBHDA_access_end(%ld %ld %ld %ld)\n", 
			rect_left, rect_top, rect_right, rect_bottom);*/

		if(w > 0 && h > 0)
		{
			check_dirty();
			mouse_blit();
			if(hda->surface > 0)
			{
				switch(hda->bpp)
				{
					case 32:
					{
				  	SVGAFifoCmdBlitGMRFBToScreen *gmrblit;
				  	DWORD cmd_offset = 0;
		
						wait_for_cmdbuf();
						
				  	gmrblit = SVGA_cmd_ptr(cmdbuf, &cmd_offset, SVGA_CMD_BLIT_GMRFB_TO_SCREEN, sizeof(SVGAFifoCmdBlitGMRFBToScreen));
	
				  	gmrblit->srcOrigin.x      = rect_left;
				  	gmrblit->srcOrigin.y      = rect_top;
				  	gmrblit->destRect.left    = rect_left;
				  	gmrblit->destRect.top     = rect_top;
				  	gmrblit->destRect.right   = rect_right;
				  	gmrblit->destRect.bottom  = rect_bottom;
	
				  	gmrblit->destScreenId = 0;
	
						submit_cmdbuf(cmd_offset, SVGA_CB_UPDATE, 0);
						break;
					}
					case 16:
						blit16(
							((BYTE*)hda->vram_pm32)+hda->surface, hda->pitch,
							hda->vram_pm32,  SVGA_pitch(hda->width, 32),
							rect_left, rect_top,
							rect_right - rect_left, rect_bottom - rect_top
						);
						need_refresh = TRUE;
						break;
					case 8:
						blit8(
							((BYTE*)hda->vram_pm32)+hda->surface, hda->pitch,
							hda->vram_pm32,  SVGA_pitch(hda->width, 32),
							rect_left, rect_top,
							rect_right - rect_left, rect_bottom - rect_top
						);
						need_refresh = TRUE;
						break;
				} // switch
			}
	
		  if(need_refresh)
		  {
		  	SVGAFifoCmdUpdate *cmd_update;
		  	DWORD cmd_offset = 0;
	
		  	wait_for_cmdbuf();
	
		  	cmd_update = SVGA_cmd_ptr(cmdbuf, &cmd_offset, SVGA_CMD_UPDATE, sizeof(SVGAFifoCmdUpdate));
		  	cmd_update->x = rect_left;
		  	cmd_update->y = rect_top;
		  	cmd_update->width  = rect_right - rect_left;
		  	cmd_update->height = rect_bottom - rect_top;
	
				submit_cmdbuf(cmd_offset, SVGA_CB_UPDATE, 0);
		  }
		}
		else
		{
			mouse_blit(); /* in this case is mouse unvisible, but we need still switch visibility state */
		} // w == 0 && h == 0
	} // fb_lock_cnt == 0
	
	Signal_Semaphore(hda_sem);
}

void FBHDA_palette_set(unsigned char index, DWORD rgb)
{
	if(hda->system_surface > 0)
	{
		palette_emulation[index] = rgb;
	}
	else
	{
		UINT sIndex = SVGA_PALETTE_BASE + index*3;
		
  	SVGA_WriteReg(sIndex+0, (rgb >> 16) & 0xFF);
  	SVGA_WriteReg(sIndex+1, (rgb >>  8) & 0xFF);
  	SVGA_WriteReg(sIndex+2,  rgb        & 0xFF);
  }
  hda->palette_update++;
}

DWORD FBHDA_palette_get(unsigned char index)
{
	if(hda->system_surface > 0)
	{
		return palette_emulation[index];
	}
	else
	{
		UINT sIndex = SVGA_PALETTE_BASE + index*3;
	
		return ((SVGA_ReadReg(sIndex+0) & 0xFF) << 16) |
			((SVGA_ReadReg(sIndex+1) & 0xFF) << 8) |
		 	(SVGA_ReadReg(sIndex+2) & 0xFF);
	}
}

static DWORD ov_width = 0;
static DWORD ov_height = 0;
static DWORD ov_pitch = 0;
static DWORD ov_bpp = 0;

DWORD FBHDA_overlay_setup(DWORD overlay, DWORD width, DWORD height, DWORD bpp)
{
	dbg_printf("FBHDA_overlay_setup: %ld\n", overlay);

	if(overlay >= FBHDA_OVERLAYS_MAX)
		return 0;

	if(overlay == 0)
	{
		/* restore */
		SVGA_setmode_phy(svga_saved_state.width, svga_saved_state.height, svga_saved_state.bpp);
		SVGA_DefineGMRFB();
		
		hda->overlay = 0;

		if(!svga_saved_state.enabled)
		{
			SVGA_Disable();
		}
		else
		{
			/* redraw framebuffer */
			FBHDA_access_begin(0);
			FBHDA_access_end(0);
		}

		return hda->pitch;
	}
	else
	{
		if(bpp == 32)
		{
			SVGAFifoCmdDefineGMRFB *gmrfb;
			DWORD cmd_offset = 0;

			DWORD pitch  = SVGA_pitch(width, bpp);
			DWORD offset = ((BYTE*)hda->overlays[overlay].ptr) - ((BYTE*)hda->vram_pm32);
			DWORD stride = pitch*height;

			if(hda->overlays[overlay].size > stride)
			{
				hda->overlay = overlay;
				SVGA_setmode_phy(width, height, bpp);

				wait_for_cmdbuf();
				gmrfb = SVGA_cmd_ptr(cmdbuf, &cmd_offset, SVGA_CMD_DEFINE_GMRFB, sizeof(SVGAFifoCmdDefineGMRFB));
				SVGA_FillGMRFB(gmrfb, offset, pitch, bpp);
				submit_cmdbuf(cmd_offset, SVGA_CB_SYNC, 0);

				ov_width  = width;
				ov_height = height;
				ov_pitch  = pitch;
				ov_bpp    = bpp;

				/* clear screen */
				FBHDA_overlay_lock(0, 0, width, height);
				memset(hda->overlays[overlay].ptr, 0, stride);
				FBHDA_overlay_unlock(0);

				return pitch;
			}
		} // bpp == 32
	} // overlay > 0
	
	return 0;
}

static int overlay_lock_cnt = 0;
static DWORD ov_rect_left   = 0;
static DWORD ov_rect_top    = 0;
static DWORD ov_rect_right  = 0;
static DWORD ov_rect_bottom = 0;

void FBHDA_overlay_lock(DWORD left, DWORD top, DWORD right, DWORD bottom)
{	
	if(hda->overlay == 0) return;
	
	if(overlay_lock_cnt++ == 0)
	{
		ov_rect_left   = left;
		ov_rect_top    = top;
		ov_rect_right  = right;
		ov_rect_bottom = bottom;
	}
	else
	{
		if(left < ov_rect_left) ov_rect_left = left;

		if(top < ov_rect_top) ov_rect_top = top;

		if(right > ov_rect_right) ov_rect_right = right;

		if(bottom > ov_rect_bottom) ov_rect_bottom = bottom;

	}
	
	if(ov_rect_right > ov_width) ov_rect_right = ov_width;

	if(ov_rect_bottom > ov_height) ov_rect_right = ov_height;
}

void  FBHDA_overlay_unlock(DWORD flags)
{
	if(hda->overlay == 0) return;
	
	if(--overlay_lock_cnt <= 0)
	{
		if(ov_rect_left < ov_rect_right && ov_rect_top < ov_rect_bottom)
		{
			SVGAFifoCmdBlitGMRFBToScreen *gmrblit;
			DWORD cmd_offset = 0;
			
			wait_for_cmdbuf();
			gmrblit = SVGA_cmd_ptr(cmdbuf, &cmd_offset, SVGA_CMD_BLIT_GMRFB_TO_SCREEN, sizeof(SVGAFifoCmdBlitGMRFBToScreen));

	  	gmrblit->srcOrigin.x      = ov_rect_left;
	  	gmrblit->srcOrigin.y      = ov_rect_top;
	  	gmrblit->destRect.left    = ov_rect_left;
	  	gmrblit->destRect.top     = ov_rect_top;
	  	gmrblit->destRect.right   = ov_rect_right;
	  	gmrblit->destRect.bottom  = ov_rect_bottom;
	
	  	gmrblit->destScreenId = 0;
				  	
			submit_cmdbuf(cmd_offset, SVGA_CB_UPDATE, 0);
		}

		if(overlay_lock_cnt < 0)
		{
			overlay_lock_cnt = 0;
		}
	}
}

#define BSTEP (sizeof(DWORD)*8)

static inline void map_reset(DWORD *bitmap, DWORD id)
{
	DWORD i = id / BSTEP;
	DWORD ii = id % BSTEP;
	
	bitmap += i;
	*bitmap |= ((DWORD)1 << ii);
}

void SVGA_ProcessCleanup(DWORD pid)
{
	DWORD id;
	/* just for safety */
	if(pid == 0)
		return;

	/* some process are terminated when SVGA is disabled, clean not possible */
	if(!svga_saved_state.enabled)
		return;

	Begin_Critical_Section(0);
	if(svga_db != NULL)
	{
		/* clean surfaces */
		for(id = 0; id < svga_db->surfaces_cnt; id++)
		{
			SVGA_DB_surface_t *sinfo = &svga_db->surfaces[id];
			if(sinfo->pid == pid)
			{
				dbg_printf("Cleaning surface: %d\n", id);
				if(sinfo->gmrId) // GB surface
				{
					DWORD cmd_offset =  0;
					SVGA3dCmdBindGBSurface *unbind;
					SVGA3dCmdDestroySurface *destgb;

					wait_for_cmdbuf();
					unbind = SVGA_cmd3d_ptr(cmdbuf, &cmd_offset, SVGA_3D_CMD_BIND_GB_SURFACE, sizeof(SVGA3dCmdBindGBSurface));
					unbind->sid   = id+1;
					unbind->mobid = SVGA3D_INVALID_ID;

					destgb = SVGA_cmd3d_ptr(cmdbuf, &cmd_offset, SVGA_3D_CMD_DESTROY_GB_SURFACE, sizeof(SVGA3dCmdDestroySurface));
					destgb->sid = id+1;

					submit_cmdbuf(cmd_offset, SVGA_CB_SYNC, 0);
				}
				else
				{
					DWORD cmd_offset =  0;
					SVGA3dCmdDestroySurface *dest;
					
					wait_for_cmdbuf();
					dest = SVGA_cmd3d_ptr(cmdbuf, &cmd_offset, SVGA_3D_CMD_SURFACE_DESTROY, sizeof(SVGA3dCmdDestroySurface));
					dest->sid = id+1;
					
					submit_cmdbuf(cmd_offset, SVGA_CB_SYNC, 0);
				}

				sinfo->pid = 0;
				map_reset(svga_db->surfaces_map, id);
			}
		} // for

		/* clean contexts */
		for(id = 0; id < svga_db->contexts_cnt; id++)
		{
			SVGA_DB_context_t *cinfo = &svga_db->contexts[id];
			
			if(cinfo->pid == pid)
			{
				DWORD cmd_offset = 0;
				dbg_printf("Cleaning context: %d\n", id);
				
				wait_for_cmdbuf();
				if(cinfo->gmrId != 0) /* GB Context */
				{
					SVGA3dCmdDXDestroyContext *dest_ctx_gb =
						SVGA_cmd3d_ptr(cmdbuf, &cmd_offset, SVGA_3D_CMD_DX_DESTROY_CONTEXT, sizeof(SVGA3dCmdDXDestroyContext));
					dest_ctx_gb->cid = id+1;
					submit_cmdbuf(cmd_offset, SVGA_CB_SYNC, 0);
				}
				else
				{
					SVGA3dCmdDestroyContext *dest_ctx =
						SVGA_cmd3d_ptr(cmdbuf, &cmd_offset, SVGA_3D_CMD_CONTEXT_DESTROY, sizeof(SVGA3dCmdDestroyContext));
					dest_ctx->cid = id+1;
					submit_cmdbuf(cmd_offset, SVGA_CB_SYNC, 0);
				}
				
				cinfo->pid = 0;
				map_reset(svga_db->contexts_map, id);
			}
		} // for

		/* clean regision */
		for(id = 0; id < svga_db->regions_cnt; id++)
		{
			SVGA_DB_region_t *rinfo = &svga_db->regions[id];
			if(rinfo->pid == pid)
			{
				dbg_printf("Cleaning regions: %d\n", id);

				SVGA_region_free(&rinfo->info);
				rinfo->pid = 0;
				map_reset(svga_db->regions_map, id);
			}
		} // for
		
		dbg_printf("Free - pid: %ld, used memory: %ld\n", pid, svga_db->stat_regions_usage);
	} // db != NULL

	End_Critical_Section();
}

void SVGA_AllProcessCleanup()
{
	DWORD id;

	/* some process are terminated when SVGA is disabled, clean not possible */
	if(!svga_saved_state.enabled)
		return;

	Begin_Critical_Section(0);
	if(svga_db != NULL)
	{
		/* clean surfaces */
		for(id = 0; id < svga_db->surfaces_cnt; id++)
		{
			SVGA_DB_surface_t *sinfo = &svga_db->surfaces[id];
			if(sinfo->pid != 0)
			{
				dbg_printf("Cleaning surface: %d\n", id);
				if(sinfo->gmrId) // GB surface
				{
					DWORD cmd_offset =  0;
					SVGA3dCmdBindGBSurface *unbind;
					SVGA3dCmdDestroySurface *destgb;

					wait_for_cmdbuf();
					unbind = SVGA_cmd3d_ptr(cmdbuf, &cmd_offset, SVGA_3D_CMD_BIND_GB_SURFACE, sizeof(SVGA3dCmdBindGBSurface));
					unbind->sid   = id+1;
					unbind->mobid = SVGA3D_INVALID_ID;

					destgb = SVGA_cmd3d_ptr(cmdbuf, &cmd_offset, SVGA_3D_CMD_DESTROY_GB_SURFACE, sizeof(SVGA3dCmdDestroySurface));
					destgb->sid = id+1;

					submit_cmdbuf(cmd_offset, SVGA_CB_SYNC, 0);
				}
				else
				{
					DWORD cmd_offset =  0;
					SVGA3dCmdDestroySurface *dest;
					
					wait_for_cmdbuf();
					dest = SVGA_cmd3d_ptr(cmdbuf, &cmd_offset, SVGA_3D_CMD_SURFACE_DESTROY, sizeof(SVGA3dCmdDestroySurface));
					dest->sid = id+1;
					
					submit_cmdbuf(cmd_offset, SVGA_CB_SYNC, 0);
				}

				sinfo->pid = 0;
				map_reset(svga_db->surfaces_map, id);
			}
		} // for

		/* clean contexts */
		for(id = 0; id < svga_db->contexts_cnt; id++)
		{
			SVGA_DB_context_t *cinfo = &svga_db->contexts[id];
			
			if(cinfo->pid != 0)
			{
				DWORD cmd_offset = 0;
				dbg_printf("Cleaning context: %d\n", id);
				
				wait_for_cmdbuf();
				if(cinfo->gmrId != 0) /* GB Context */
				{
					SVGA3dCmdDXDestroyContext *dest_ctx_gb =
						SVGA_cmd3d_ptr(cmdbuf, &cmd_offset, SVGA_3D_CMD_DX_DESTROY_CONTEXT, sizeof(SVGA3dCmdDXDestroyContext));
					dest_ctx_gb->cid = id+1;
					submit_cmdbuf(cmd_offset, SVGA_CB_SYNC, 0);
				}
				else
				{
					SVGA3dCmdDestroyContext *dest_ctx =
						SVGA_cmd3d_ptr(cmdbuf, &cmd_offset, SVGA_3D_CMD_CONTEXT_DESTROY, sizeof(SVGA3dCmdDestroyContext));
					dest_ctx->cid = id+1;
					submit_cmdbuf(cmd_offset, SVGA_CB_SYNC, 0);
				}
				
				cinfo->pid = 0;
				map_reset(svga_db->contexts_map, id);
			}
		} // for

		/* clean region */
		for(id = 0; id < svga_db->regions_cnt; id++)
		{
			SVGA_DB_region_t *rinfo = &svga_db->regions[id];
			if(rinfo->pid != 0)
			{
				dbg_printf("Cleaning regions: %d\n", id);

				SVGA_region_free(&rinfo->info);
				rinfo->pid = 0;
				map_reset(svga_db->regions_map, id);
			}
		} // for
		
		dbg_printf("Cleanup: used memory: %ld\n", svga_db->stat_regions_usage);
	} // db != NULL

	End_Critical_Section();
}
