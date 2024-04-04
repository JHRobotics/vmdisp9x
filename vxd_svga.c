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

#include "vxd_strings.h"

/*
 * consts
 */
//#define SVGA3D_MAX_MOBS 33280UL /* SVGA3D_MAX_CONTEXT_IDS + SVGA3D_MAX_CONTEXT_IDS + SVGA3D_MAX_SURFACE_IDS */
#define SVGA3D_MAX_MOBS (SVGA3D_MAX_CONTEXT_IDS + SVGA3D_MAX_CONTEXT_IDS + SVGA3D_MAX_SURFACE_IDS)
#define VBOX_VIDEO_MAX_SCREENS 64

#define PTONPAGE (P_SIZE/sizeof(DWORD))

#define SPARE_REGIONS_LARGE 2
#define SPARE_REGION_LARGE_SIZE (32*1024*1024)

#define SPARE_REGIONS_MEDIUM 16
#define SPARE_REGION_MEDIUM_SIZE (1024*1024)

#define SPARE_REGIONS_SMALL 128
//#define SPARE_REGION_SMALL_SIZE 4096

#define SPARE_REGIONS_CNT (SPARE_REGIONS_LARGE+SPARE_REGIONS_MEDIUM+SPARE_REGIONS_SMALL)

#define CACHE_THRESHOLD 128

#define SVGA_MIN_VRAM 8

#define SVGA_TIMEOUT 0x1000000UL

#define WAIT_FOR_CB(_cb, _forcesync) \
	do{ \
		DWORD wcnt; \
		for(wcnt = 0; (_cb)->status == SVGA_CB_STATUS_NONE; wcnt++){ \
			/*if(wcnt >= SVGA_TIMEOUT) break;*/ \
			WAIT_FOR_CB_SYNC_ ## _forcesync \
		} \
	}while(0)

/* expansions of WAIT_FOR_CB */
#define WAIT_FOR_CB_SYNC_0
#define WAIT_FOR_CB_SYNC_1 SVGA_Sync();

/*
 * types
 */

#pragma pack(push)
#pragma pack(1)
typedef struct _cb_enable_t
{
	uint32             cmd;
	SVGADCCmdStartStop cbstart;
} cb_enable_t;
#pragma pack(pop)

typedef struct spare_region
{
	BOOL      used;
	uint32    missed;
	SVGA_region_info_t region;
} spare_region_t;

typedef struct svga_cache_state
{
	int free_index_min;
	int free_index_max;
	int cnt_large;
	int cnt_medium;
	int cnt_small;
	spare_region_t spare_region[SPARE_REGIONS_CNT];
} svga_cache_state_t;

/*
 * globals
 */
extern FBHDA_t *hda;
extern ULONG hda_sem;
static SVGA_DB_t *svga_db = NULL;

extern LONG fb_lock_cnt;

static BOOL gb_support = FALSE;
static BOOL cb_support = FALSE;
static BOOL cb_context0 = FALSE;

/* for GPU9 is FIFO more stable (VMWARE) or faster (VBOX) */
static DWORD prefer_fifo = 1; 

static BOOL SVGA_is_valid = FALSE;

static volatile SVGACBHeader *last_cb = NULL;

static uint64 cb_next_id = {0, 0};
static DWORD fence_next_id = 1;
static void *cmdbuf = NULL;

static svga_cache_state_t cache_state = {0, 0};

static BOOL cache_enabled = FALSE;

/* object table for GPU10 */
static SVGA_OT_info_entry_t otable_setup[SVGA_OTABLE_DX_MAX] = {
	{0, NULL, RoundToPages(SVGA3D_MAX_MOBS*sizeof(SVGAOTableMobEntry))*P_SIZE,                 0}, /* SVGA_OTABLE_MOB */
	{0, NULL, RoundToPages(SVGA3D_MAX_SURFACE_IDS*sizeof(SVGAOTableSurfaceEntry))*P_SIZE,      0}, /* SVGA_OTABLE_SURFACE */
	{0, NULL, RoundToPages(SVGA3D_MAX_CONTEXT_IDS*sizeof(SVGAOTableContextEntry))*P_SIZE,      0}, /* SVGA_OTABLE_CONTEXT */
	{0, NULL, 0,                                                                               0}, /* SVGA_OTABLE_SHADER - not used */
	{0, NULL, RoundToPages(VBOX_VIDEO_MAX_SCREENS*sizeof(SVGAOTableScreenTargetEntry))*P_SIZE, 0}, /* SVGA_OTABLE_SCREENTARGET (VBOX_VIDEO_MAX_SCREENS) */
	{0, NULL, RoundToPages(SVGA3D_MAX_CONTEXT_IDS*sizeof(SVGAOTableDXContextEntry))*P_SIZE,    0}, /* SVGA_OTABLE_DXCONTEXT */
};

static SVGA_OT_info_entry_t *otable = NULL;

/*
 * strings
 */
static char SVGA_conf_path[] = "Software\\VMWSVGA";
static char SVGA_conf_hw_cursor[]  = "HWCursor";
static char SVGA_conf_vram_limit[] = "VRAMLimit";
static char SVGA_conf_rgb565bug[]  = "RGB565bug";
static char SVGA_conf_cb[]         = "CommandBuffers";
static char SVGA_conf_pref_fifo[]  = "PreferFIFO";
static char SVGA_conf_hw_version[] = "HWVersion";
static char SVGA_vxd_name[] = "vmwsmini.vxd";

/* VM handle */
extern DWORD ThisVM;

/* prototypes */
DWORD SVGA_GetDevCap(DWORD search_id);

/**
 * Delete item from cache
 **/
static void cache_delete(int index)
{
	if(cache_state.spare_region[index].used != FALSE)
	{
		SVGA_region_info_t *rinfo = &cache_state.spare_region[index].region;
		BYTE *free_ptr = (BYTE*)rinfo->address;
		uint32 region_size = rinfo->size;;
		
		if(rinfo->region_address != NULL)
		{
			dbg_printf(dbg_pagefree, rinfo->region_address);
			_PageFree((PVOID)rinfo->region_address, 0);
		}
		else
		{
			free_ptr -= P_SIZE;
		}
			
		if(rinfo->mob_address != NULL)
		{
			dbg_printf(dbg_pagefree, rinfo->mob_address);
			_PageFree((PVOID)rinfo->mob_address, 0);
		}
			
		dbg_printf(dbg_pagefree, free_ptr);
		_PageFree((PVOID)free_ptr, 0);
			
		cache_state.spare_region[index].used = FALSE;
		
		if((index+1) == cache_state.free_index_max)
		{
			cache_state.free_index_max--;
		}
		
		if(index < cache_state.free_index_min)
		{
			cache_state.free_index_min = index;
		}
		
		if(region_size >= SPARE_REGION_LARGE_SIZE)
			cache_state.cnt_large--;
		else if(region_size >= SPARE_REGION_MEDIUM_SIZE)
			cache_state.cnt_medium--;
		else
			cache_state.cnt_small--;
	}
}

/**
 * Insert region to cache
 **/
static BOOL cache_insert(SVGA_region_info_t *region)
{
	int i;
	BOOL rc = FALSE;
	
	if(!cache_enabled)
	{
		return FALSE;
	}
	
	if(region->size >= SPARE_REGION_LARGE_SIZE)
	{
		if(cache_state.cnt_large > SPARE_REGIONS_LARGE) return FALSE;
	}
	else if(region->size >= SPARE_REGION_MEDIUM_SIZE)
	{
		if(cache_state.cnt_medium > SPARE_REGIONS_MEDIUM) return FALSE;
	}
	else
	{
		if(cache_state.cnt_small > SPARE_REGIONS_SMALL) return FALSE;
	}
	
	for(i = cache_state.free_index_min; i < SPARE_REGIONS_CNT; i++)
	{		
		if(!cache_state.spare_region[i].used)
		{
			memcpy(&cache_state.spare_region[i].region, region, sizeof(SVGA_region_info_t));
			cache_state.spare_region[i].used = TRUE;
			cache_state.spare_region[i].missed = 0;
			rc = TRUE;
			break;
		}
	}
	
	for(; i < SPARE_REGIONS_CNT; i++)
	{
		if(!cache_state.spare_region[i].used)
		{
			cache_state.free_index_min = i;
			break;
		}
	}
	
	if(i == SPARE_REGIONS_CNT)
	{
		cache_state.free_index_min = i;
		cache_state.free_index_max = SPARE_REGIONS_CNT;
	}
	else if(i >= cache_state.free_index_max)
	{
		cache_state.free_index_max = i+1;
	}

	if(rc)
	{
		if(region->size >= SPARE_REGION_LARGE_SIZE)
			cache_state.cnt_large++;
		else if(region->size >= SPARE_REGION_MEDIUM_SIZE)
			cache_state.cnt_medium++;
		else
			cache_state.cnt_small++;

		dbg_printf(dbg_cache_insert, region->region_id, region->size);
	}

	return rc;
}

/**
 * Use region from cache
 **/
static BOOL cache_use(SVGA_region_info_t *region)
{
	int i;
	BOOL rc = FALSE;
	
	if(!cache_enabled)
	{
		return FALSE;
	}
	
	dbg_printf(dbg_cache_search, region->size);
	
	for(i = cache_state.free_index_max-1; i >= 0; i--)
	{
		if(cache_state.spare_region[i].used != FALSE)
		{
			if(cache_state.spare_region[i].region.size == region->size)
			{
				SVGA_region_info_t *ptr = &cache_state.spare_region[i].region;
				
				region->address        = ptr->address;
				region->region_address = ptr->region_address;
				region->region_ppn     = ptr->region_ppn;
				region->mob_address    = ptr->mob_address;
				region->mob_ppn        = ptr->mob_ppn;
				
				cache_state.spare_region[i].used = FALSE;
				rc = TRUE;
				break;
			}
			else
			{
				if(++cache_state.spare_region[i].missed >= CACHE_THRESHOLD)
				{
					dbg_printf(dbg_cache_delete, cache_state.spare_region[i].region.size);
					cache_delete(i);
				}
			}
		}
	}
	
	if(rc)
	{
		if((i+1) == cache_state.free_index_max)
		{
			cache_state.free_index_max--;
		}
		
		if(i < cache_state.free_index_min)
		{
			cache_state.free_index_min = i;
		}
		
		if(region->size >= SPARE_REGION_LARGE_SIZE)
			cache_state.cnt_large--;
		else if(region->size >= SPARE_REGION_MEDIUM_SIZE)
			cache_state.cnt_medium--;
		else
			cache_state.cnt_small--;
	
		dbg_printf(dbg_cache_used, region->region_id, region->size);
	}
		
	return rc;
}

/**
 * Notify virtual HW that is some work to do
 **/
static void SVGA_Sync()
{
	SVGA_WriteReg(SVGA_REG_SYNC, 1);
}

static void SVGA_Flush_CB_critical()
{
	/* wait for actual CB */
	if(last_cb != NULL)
	{
		WAIT_FOR_CB(last_cb, 1);
	}
	
	/* drain FIFO */
	SVGA_Flush();
}

static void wait_for_cmdbuf()
{
	SVGACBHeader *cb = ((SVGACBHeader *)cmdbuf)-1;
	WAIT_FOR_CB(cb, 0);
}

#if 0
static void SVGA_Flush_CB()
{
	Begin_Critical_Section(0);
	SVGA_Flush_CB_critical();
	End_Critical_Section();
}
#endif

/* map physical addresses to system linear space */
void SVGA_MapIO()
{
	if(SVGA_IsSVGA3())
	{
		gSVGA.rmmio_linear = _MapPhysToLinear(gSVGA.fifoPhy, gSVGA.fifoSize, 0);
		gSVGA.rmmio = (uint32 FARP*)gSVGA.rmmio_linear;
	}
}

/**
 * Allocate OTable for GB objects
 **/
static void SVGA_OTable_alloc()
{
	int i;
	
	if(otable == NULL)
	{
		otable = (SVGA_OT_info_entry_t *)_PageAllocate(RoundToPages(sizeof(otable_setup)), PG_VM, ThisVM, 0, 0x0, 0x100000, NULL, PAGEFIXED);
		if(otable)
		{
			memcpy(otable, &(otable_setup[0]), sizeof(otable_setup));
		}
	}

	if(otable)
	{
		for(i = 0; i < SVGA_OTABLE_DX_MAX; i++)
		{
			SVGA_OT_info_entry_t *entry = &otable[i];
			if(entry->size != 0 && (entry->flags & SVGA_OT_FLAG_ALLOCATED) == 0)
			{
				entry->lin = (void*)_PageAllocate(RoundToPages(entry->size), PG_VM, ThisVM, 0, 0x0, 0x100000, &entry->phy, PAGECONTIG | PAGEUSEALIGN | PAGEFIXED);
				if(entry->lin)
				{
					dbg_printf(dbg_mob_allocate, i);
					entry->flags |= SVGA_OT_FLAG_ALLOCATED;
				}
			}
		}
	}
}

SVGA_OT_info_entry_t *SVGA_OT_setup()
{
	return otable;
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

SVGA_DB_t *SVGA_DB_setup()
{
	return svga_db;
}

static DWORD SVGA_fence_passed()
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

void SVGA_fence_wait(DWORD fence_id)
{
	DWORD last_pased;
	DWORD last_fence;
	DWORD cnt = 0;
	
	for(cnt = 0; cnt < SVGA_TIMEOUT; cnt++)
	{
		SVGA_fence_query(&last_pased, &last_fence);
		if(fence_id > last_fence)
		{
			break;
		}
		
		if(fence_id <= last_pased)
		{
			break;
		}
		
		SVGA_Sync();
	}
	
	
	if(cnt == SVGA_TIMEOUT)
	{
		SVGA_Flush();
	}
}

/**
 * Allocate memory for command buffer
 **/
DWORD *SVGA_CMB_alloc()
{
	DWORD phy;
	SVGACBHeader *cb;
	
	Begin_Critical_Section(0);
	
	cb = (SVGACBHeader*)_PageAllocate(RoundToPages(SVGA_CB_MAX_SIZE+sizeof(SVGACBHeader)), PG_SYS, 0, 0, 0x0, 0x100000, &phy, PAGECONTIG | PAGEUSEALIGN | PAGEFIXED);
	
	if(cb)
	{
		memset(cb, 0, sizeof(SVGACBHeader));
		cb->status = SVGA_CB_STATUS_COMPLETED; /* important to sync between commands */
		cb->ptr.pa.hi   = 0;
		cb->ptr.pa.low  = phy + sizeof(SVGACBHeader);	
		
		End_Critical_Section();
		return (DWORD*)(cb+1);
	}
	End_Critical_Section();
	
	return NULL;
}

void SVGA_CMB_free(DWORD *cmb)
{
	SVGACBHeader *cb = ((SVGACBHeader *)cmb)-1;
	
	Begin_Critical_Section(0);
	
	WAIT_FOR_CB(cb, 1);
	
	if(cb == last_cb)
	{
		last_cb = NULL;
	}
		
	_PageFree(cb, 0);
	
	End_Critical_Section();
}

static void *SVGA_cmd_ptr(DWORD *buf, DWORD *pOffset, DWORD cmd, DWORD cmdsize)
{
	DWORD pp = (*pOffset)/sizeof(DWORD);
	buf[pp] = cmd;
	*pOffset = (*pOffset) + sizeof(DWORD) + cmdsize;
	
	return (void*)(buf + pp + 1);
}

static void *SVGA_cmd3d_ptr(DWORD *buf, DWORD *pOffset, DWORD cmd, DWORD cmdsize)
{
	DWORD pp = (*pOffset)/sizeof(DWORD);
	buf[pp] = cmd;
	buf[pp+1] = cmdsize;
	*pOffset = (*pOffset) + 2*sizeof(DWORD) + cmdsize;
	
	return (void*)(buf + pp + 2);
}

// FIXME: inline?
static void SVGA_cb_id_inc()
{
	_asm
	{
		inc dword ptr [cb_next_id]
		adc dword ptr [cb_next_id+4], 0
	}
}

void SVGA_CMB_submit(DWORD FBPTR cmb, DWORD cmb_size, SVGA_CMB_status_t FBPTR status, DWORD flags, DWORD DXCtxId)
{
	SVGACBHeader *cb = ((SVGACBHeader *)cmb)-1;
	Begin_Critical_Section(0);
	
	if(  /* CB are supported and enabled */
		cb_support &&
		(cb_context0 || (flags & SVGA_CB_USE_CONTEXT_DEVICE)) &&
		(flags & SVGA_CB_FORCE_FIFO) == 0
	)
	{
		DWORD cbhwctxid = SVGA_CB_CONTEXT_0;
		DWORD fence = 0;
		
		if(flags & SVGA_CB_USE_CONTEXT_DEVICE)
		{
			cbhwctxid = SVGA_CB_CONTEXT_DEVICE;
		}
		
		if(flags & SVGA_CB_FORCE_FENCE)
		{
			DWORD dwords = cmb_size/sizeof(DWORD);
			fence = SVGA_fence_get();
			cmb[dwords]   = SVGA_CMD_FENCE;
			cmb[dwords+1] = fence;
			cmb_size += sizeof(DWORD)*2;
		}
		
		if(cmb_size == 0)
		{
			cb->status = SVGA_PROC_COMPLETED;
			if(status)
			{
				status->sStatus = SVGA_PROC_COMPLETED;
				status->qStatus = NULL;
				status->fifo_fence_used = 0;
			}
		}
		else
		{
			cb->status = SVGA_CB_STATUS_NONE;
			cb->errorOffset = 0;
			cb->offset = 0; /* VMware modified this, needs to be clear */
			cb->flags  = SVGA_CB_FLAG_NO_IRQ;
			
			if(flags & SVGA_CB_FLAG_DX_CONTEXT)
			{
				cb->flags |= SVGA_CB_FLAG_DX_CONTEXT;
				cb->dxContext = DXCtxId;
			}
			else
			{
				cb->dxContext = 0;
			}
			
			cb->id.low = cb_next_id.low;
			cb->id.hi  = cb_next_id.hi;
			cb->length = cmb_size;
			
#if 0
			/* wait for last CB to complete */
			if(last_cb)
			{
				WAIT_FOR_CB(last_cb, 0);
			}
#endif
			SVGA_WriteReg(SVGA_REG_COMMAND_HIGH, 0); // high part of 64-bit memory address...
			SVGA_WriteReg(SVGA_REG_COMMAND_LOW, (cb->ptr.pa.low - sizeof(SVGACBHeader)) | cbhwctxid);
			SVGA_Sync(); /* notify HV to read registers (VMware needs it) */
			
			last_cb = cb;
	
			if(flags & SVGA_CB_SYNC)
			{
				WAIT_FOR_CB(cb, 0);

				if(cb->status != SVGA_CB_STATUS_COMPLETED)
				{
					dbg_printf(dbg_cmd_error, cb->status, cmb[0], cb->errorOffset);
					if(flags & SVGA_CB_FORCE_FENCE)
					{
						/* this may cause freeze, when fence is in buffer and isn't complete. So return some passed fence  */
						fence = SVGA_fence_passed();
					}
				}
				
				if(status)
				{
					status->sStatus = SVGA_PROC_COMPLETED;
					status->qStatus = NULL;
					status->fifo_fence_used = fence;
				}
				
				last_cb = NULL;
			}
			else
			{
				if(status)
				{
					status->sStatus = SVGA_PROC_NONE;
					status->qStatus = (volatile DWORD*)&cb->status;
					status->fifo_fence_used = fence;
				}
			}
		}
		
		SVGA_cb_id_inc();		
	}
	else /* copy to FIFO */
	{
		DWORD *ptr = cmb;
		DWORD dwords = cmb_size/sizeof(DWORD);
		DWORD nextCmd, max, min;
		DWORD fence;
		
		/* insert fence CMD */
		fence = SVGA_fence_get();
		ptr[dwords]   = SVGA_CMD_FENCE;
		ptr[dwords+1] = fence;
		
		dwords += 2;
		
		nextCmd = gSVGA.fifoMem[SVGA_FIFO_NEXT_CMD];
		max     = gSVGA.fifoMem[SVGA_FIFO_MAX];
		min     = gSVGA.fifoMem[SVGA_FIFO_MIN];
		
		/* copy to fifo */
		while(dwords > 0)
		{
			gSVGA.fifoMem[nextCmd/sizeof(DWORD)] = *ptr;
			ptr++;
			
			nextCmd += sizeof(uint32);
			if (nextCmd >= max)
			{
				nextCmd = min;
			}
			gSVGA.fifoMem[SVGA_FIFO_NEXT_CMD] = nextCmd;
			dwords--;
		}
		
		if(flags & SVGA_CB_SYNC)
		{
			SVGA_fence_wait(fence);
			if(status)
			{
				status->sStatus = SVGA_PROC_COMPLETED;
				status->qStatus = NULL;
				status->fifo_fence_used = 0;
			}
		}
		else
		{
			if(status)
			{
				status->sStatus = SVGA_PROC_FENCE;
				status->fifo_fence_used = fence;
				status->qStatus = NULL;
			}
		}
		
		cb->status = SVGA_PROC_COMPLETED;
		last_cb = NULL;
	}
	
	if(status)
	{
		status->fifo_fence_last = SVGA_fence_passed();
	}
	
	End_Critical_Section();
	
	//dbg_printf(dbg_cmd_off, cmb[0]);
}

/**
 * Init SVGA-II hardware
 * return TRUE on success
 * return FALSE on failure (unsupported HW version or invalid HW)
 **/
BOOL SVGA_init_hw()
{
	/* defaults */
	DWORD conf_hw_cursor = 0;
	DWORD conf_vram_limit = 0;
	DWORD conf_rgb565bug = 1;
	DWORD conf_cb = 1;
	DWORD conf_hw_version = SVGA_VERSION_2;
	int rc;
	
	/* configs in registry */
	RegReadConf(HKEY_LOCAL_MACHINE, SVGA_conf_path, SVGA_conf_hw_cursor,  &conf_hw_cursor);
 	RegReadConf(HKEY_LOCAL_MACHINE, SVGA_conf_path, SVGA_conf_vram_limit, &conf_vram_limit);
 	RegReadConf(HKEY_LOCAL_MACHINE, SVGA_conf_path, SVGA_conf_rgb565bug,  &conf_rgb565bug);
 	RegReadConf(HKEY_LOCAL_MACHINE, SVGA_conf_path, SVGA_conf_cb,         &conf_cb); 
 	RegReadConf(HKEY_LOCAL_MACHINE, SVGA_conf_path, SVGA_conf_hw_version, &conf_hw_version);
 	RegReadConf(HKEY_LOCAL_MACHINE, SVGA_conf_path, SVGA_conf_pref_fifo,  &prefer_fifo);
 	
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
		
		if(SVGA_ReadReg(SVGA_REG_CAPABILITIES) & SVGA_CAP_CURSOR)
		{
			if(conf_hw_cursor)
			{
				gSVGA.userFlags |= SVGA_USER_FLAGS_HWCURSOR;
			}
		}
		
		if(SVGA_ReadReg(SVGA_REG_CAPABILITIES) & SVGA_CAP_ALPHA_CURSOR)
		{
			if(conf_hw_cursor)
			{
				gSVGA.userFlags |= SVGA_USER_FLAGS_ALPHA_CUR;
			}
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
		
#if 0
		/* allocate fifo bounce buffer */
		gSVGA.fifo.bounceLinear =
			_PageAllocate(
				(SVGA_BOUNCE_SIZE + PAGE_SIZE)/PAGE_SIZE,
				PG_SYS, 0, 0, 0x0, 0x100000, &(gSVGA.fifo.bouncePhy), PAGECONTIG | PAGEUSEALIGN | PAGEFIXED
			);
#endif
		
		/* system linear addres == PM32 address */
		gSVGA.fbMem = (uint8 *)gSVGA.fbLinear;
		if(!SVGA_IsSVGA3())
			gSVGA.fifoMem = (uint32 *)gSVGA.fifoLinear;
		
#if 0
    gSVGA.fifo.bounceMem = (uint8 *)(gSVGA.fifo.bounceLinear + PAGE_SIZE); /* first page is for CB header (if CB supported) */
#endif
		
		dbg_printf(dbg_mapping);
		dbg_printf(dbg_mapping_map, gSVGA.fbPhy, gSVGA.fbMem);
		dbg_printf(dbg_mapping_map, gSVGA.fifoPhy, gSVGA.fifoMem);
		//dbg_printf(dbg_mapping_map, gSVGA.fifo.bouncePhy, gSVGA.fifo.bounceMem);
		dbg_printf(dbg_siz, sizeof(gSVGA), sizeof(uint8 FARP *));
		
		/* enable FIFO */
		SVGA_Enable();
		
		/* allocate GB tables, if supported */
		if(SVGA_ReadReg(SVGA_REG_CAPABILITIES) & SVGA_CAP_GBOBJECTS)
		{
			SVGA_OTable_alloc();
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
		
		SVGA_DB_alloc();
		
		/* fill address in FBHDA */
		hda->vram_pm32 = (void*)gSVGA.fbLinear;
		hda->vram_size = gSVGA.vramSize;
 		memcpy(hda->vxdname, SVGA_vxd_name, sizeof(SVGA_vxd_name));
		
		/* allocate CB for this driver */
		cmdbuf = SVGA_CMB_alloc();
		
		hda->flags |= FB_ACCEL_VMSVGA;
		
		if(cb_support && gb_support)
		{
			hda->flags |= FB_ACCEL_VMSVGA10;
		}
		
		memset(&cache_state, 0, sizeof(svga_cache_state_t));
		
		SVGA_is_valid = TRUE;
		
		if(_GetFreePageCount(NULL) >= 0x38000UL) /* 896MB / 4096 */
		{
			cache_enabled = TRUE;
		}
		
		dbg_printf(dbg_cache, cache_enabled);
		
		return TRUE;
	}
	
	return FALSE;
}

/**
 * Return PPN (physical page number) from virtual address
 * 
 **/
static DWORD getPPN(DWORD virtualaddr)
{
	DWORD phy = 0;
	_CopyPageTable(virtualaddr/P_SIZE, 1, &phy, 0);
	return phy/P_SIZE;
}


/**
 * Allocate guest memory region (GMR) - HW needs know memory physical
 * addressed of pages in (virtual) memory block.
 * Technically this allocate 2 memory block, 1st for data and 2nd as its
 * physical description.
 *
 * @return: TRUE on success
 *
 **/
BOOL SVGA_region_create(SVGA_region_info_t *rinfo)
{
	ULONG phy = 0;
	ULONG laddr;
	ULONG pgblk;
	ULONG new_size = RoundTo4k(rinfo->size);	
	ULONG nPages = RoundToPages(new_size);
	//ULONG nPages = RoundToPages64k(rinfo->size);

	SVGAGuestMemDescriptor *desc;
	
	rinfo->size = new_size;
	
	Begin_Critical_Section(0);

	if(cache_use(rinfo))
	{
		goto spare_region_used;
	}
	
	dbg_printf(dbg_pages, rinfo->size, nPages, P_SIZE);
	
	/* first try to allocate continuous physical memory space, 1st page is used for GMR descriptor */
	laddr = _PageAllocate(nPages+1, PG_VM, ThisVM, 0, 0x0, 0x100000, &phy, PAGECONTIG | PAGEUSEALIGN | PAGEFIXED);
	
	if(laddr)
	{
		desc = (SVGAGuestMemDescriptor*)laddr;
		desc->ppn = (phy/P_SIZE)+1;
		desc->numPages = nPages;
		desc++;
		desc->ppn = 0;
		desc->numPages = 0;
		
		rinfo->address        = (void*)(laddr+P_SIZE);
		rinfo->region_address = 0;
		rinfo->region_ppn     = (phy/P_SIZE);
		rinfo->mob_address    = 0;	
		rinfo->mob_ppn        = (phy/P_SIZE)+1;
		
		dbg_printf(dbg_region_simple, laddr, rinfo->address);
	}
	else
	{
		/* memory is too fragmented to create this large continous region */
		ULONG taddr;
		ULONG tppn;
		ULONG pgi;
		ULONG base_ppn;
		ULONG base_cnt;
		ULONG blocks = 1;
		ULONG blk_pages = 0;
		
		DWORD *mob = NULL;
		DWORD mobphy = 0;
		
		/* allocate user block */
		laddr = _PageAllocate(nPages, PG_VM, ThisVM, 0, 0x0, 0x100000, NULL, PAGEFIXED);
		
		if(!laddr)
		{
			End_Critical_Section();
			return FALSE;
		}
		
		/* determine how many physical continuous blocks we have */
		base_ppn = getPPN(laddr);
		base_cnt = 1;
		for(pgi = 1; pgi < nPages; pgi++)
		{
			taddr = laddr + pgi*P_SIZE;
			tppn = getPPN(taddr);
			
			if(tppn != base_ppn + base_cnt)
			{
				base_ppn = tppn;
				base_cnt = 1;
				blocks++;
			}
			else
			{
				base_cnt++;
			}
		}
			
		// number of pages to store regions information
		blk_pages = ((blocks+1)/(P_SIZE/sizeof(SVGAGuestMemDescriptor))) + 1;
			
		// add extra descriptors for pages edge
		blk_pages = ((blocks+blk_pages+1)/(P_SIZE/sizeof(SVGAGuestMemDescriptor))) + 1;
		
		/* allocate memory for GMR descriptor */
		pgblk = _PageAllocate(blk_pages, PG_VM, ThisVM, 0, 0x0, 0x100000, NULL, PAGEFIXED);
		if(!pgblk)
		{
			_PageFree((PVOID)laddr, 0);
			End_Critical_Section();
			return FALSE;
		}
		
		desc = (SVGAGuestMemDescriptor*)pgblk;
		desc->ppn = getPPN(laddr);
		desc->numPages = 1;
		blocks = 1;
		
		/* fill GMR physical structure */
		for(pgi = 1; pgi < nPages; pgi++)
		{
			taddr = laddr + pgi*P_SIZE;
			tppn = getPPN(taddr);
					
			if(tppn == desc->ppn + desc->numPages)
			{
				desc->numPages++;
			}
			else
			{
				/* next descriptor is on page edge */
				if((blocks+1) % (P_SIZE/sizeof(SVGAGuestMemDescriptor)) == 0)
				{
					desc++;
					desc->numPages = 0;
					desc->ppn = getPPN((DWORD)(desc+1));
					blocks++;
				}

				desc++;
				desc->ppn = tppn;
				desc->numPages = 1;
				blocks++;
			}
		}

		desc++;
		desc->ppn = 0;
		desc->numPages = 0;

		if(gb_support) /* don't create MOBs for Gen9 */
		{
			int i;
			/* for simplicity we're always creating table of depth 2, first page is page of PPN pages */
			mob = (DWORD *)_PageAllocate(1 + (nPages + PTONPAGE - 1)/PTONPAGE, PG_VM, ThisVM, 0, 0x0, 0x100000, &mobphy, PAGECONTIG | PAGEUSEALIGN | PAGEFIXED);
				
			if(!mob)
			{
				_PageFree((PVOID)laddr, 0);
				_PageFree((PVOID)pgblk, 0);
				End_Critical_Section();
				return FALSE;
			}
			
			/* dim 1 */
			for(i = 0; i < (nPages + PTONPAGE - 1)/PTONPAGE; i++)
			{
				mob[i] = (mobphy/P_SIZE) + i + 1;
			}
			/* dim 2 */
			for(i = 0; i < nPages; i++)
			{
				mob[PTONPAGE + i] = getPPN(laddr + i*P_SIZE);
			}
		}
		
		rinfo->address        = (void*)laddr;
		rinfo->region_address = (void*)pgblk;
		rinfo->region_ppn     = getPPN(pgblk);
		rinfo->mob_address    = (void*)mob;
		rinfo->mob_ppn        = 0;
		if(mob)
		{
			rinfo->mob_ppn        = mobphy/P_SIZE;
		}
		
		dbg_printf(dbg_region_fragmented);
	}
	
	dbg_printf(dbg_gmr, rinfo->region_id, rinfo->region_ppn);
	
	spare_region_used:
	dbg_printf(dbg_mobonly, rinfo->mobonly);
	
	// JH: no need here
	//SVGA_Flush_CB();
	
	if(!rinfo->mobonly)
	{
		if(rinfo->region_id < SVGA_ReadReg(SVGA_REG_GMR_MAX_IDS))
		{
			/* register GMR */
			SVGA_WriteReg(SVGA_REG_GMR_ID, rinfo->region_id);
			SVGA_WriteReg(SVGA_REG_GMR_DESCRIPTOR, rinfo->region_ppn);
			SVGA_Sync(); // notify register change
			
			SVGA_Flush_CB_critical();
		}
		else
		{
			/* GMR is too high, use it as only as MOB (DX) */
			rinfo->region_ppn = 0;
		}
	}

	if(gb_support)
	{
		SVGA3dCmdDefineGBMob *mob;
		DWORD cmdoff = 0;
		
		wait_for_cmdbuf();
  	mob              = SVGA_cmd3d_ptr(cmdbuf, &cmdoff, SVGA_3D_CMD_DEFINE_GB_MOB, sizeof(SVGA3dCmdDefineGBMob));
  	mob->mobid       = rinfo->region_id;
  	mob->base        = rinfo->mob_ppn;
  	mob->sizeInBytes = rinfo->size;
		if(rinfo->mob_address == NULL) /* continual memory space */
		{
			mob->ptDepth     = SVGA3D_MOBFMT_RANGE;
		}
		else
		{
			mob->ptDepth     = SVGA3D_MOBFMT_PTDEPTH_2;
		}
		
		SVGA_CMB_submit(cmdbuf, cmdoff, NULL, SVGA_CB_SYNC, 0);
		
  	rinfo->is_mob = 1;
	}
	else
	{
		rinfo->is_mob = 0;
	}
	
	svga_db->stat_regions_usage += rinfo->size;
	
	End_Critical_Section();
	
	//dbg_printf(dbg_gmr_succ, rinfo->size);
	
	return TRUE;
}

/**
 * Free data allocated by SVGA_region_create
 *
 **/
void SVGA_region_free(SVGA_region_info_t *rinfo)
{
	BOOL saved_in_cache;
	BYTE *free_ptr = (BYTE*)rinfo->address;
	
	Begin_Critical_Section(0);
	svga_db->stat_regions_usage -= rinfo->size;
	
	saved_in_cache = cache_insert(rinfo);
	
	if(gb_support)
	{
		SVGA3dCmdDestroyGBMob *mob;
		DWORD cmdoff = 0;
		
  	wait_for_cmdbuf();
  	mob              = SVGA_cmd3d_ptr(cmdbuf, &cmdoff, SVGA_3D_CMD_DESTROY_GB_MOB, sizeof(SVGA3dCmdDestroyGBMob));
  	mob->mobid       = rinfo->region_id;
  	
  	if(saved_in_cache)
  		SVGA_CMB_submit(cmdbuf, cmdoff, NULL, 0, 0);
  	else
			SVGA_CMB_submit(cmdbuf, cmdoff, NULL, SVGA_CB_SYNC, 0);
	}
	
	if(!rinfo->mobonly)
	{
		SVGA_Sync();
		SVGA_Flush_CB_critical();
		
		SVGA_WriteReg(SVGA_REG_GMR_ID, rinfo->region_id);
		SVGA_WriteReg(SVGA_REG_GMR_DESCRIPTOR, 0);
		SVGA_Sync(); // notify register change
	}

	if(!saved_in_cache)
	{
		if(rinfo->region_address != NULL)
		{
			dbg_printf(dbg_pagefree, rinfo->region_address);
			_PageFree((PVOID)rinfo->region_address, 0);
		}
		else
		{
			free_ptr -= P_SIZE;
		}
			
		if(rinfo->mob_address != NULL)
		{
			dbg_printf(dbg_pagefree, rinfo->mob_address);
			_PageFree((PVOID)rinfo->mob_address, 0);
		}
			
		dbg_printf(dbg_pagefree, free_ptr);
		_PageFree((PVOID)free_ptr, 0);
	}
	End_Critical_Section();
		
	rinfo->address        = NULL;
	rinfo->region_address = NULL;
	rinfo->mob_address    = NULL;
	rinfo->region_ppn     = 0;
	rinfo->mob_ppn        = 0;
	
	dbg_printf(dbg_pagefree_end, rinfo->region_id);
}

/**
 * Destroy saved regions and free memory
 *
 **/
void SVGA_flushcache()
{
	int i;
	
	Begin_Critical_Section(0);
	
	for(i = 0; i < cache_state.free_index_max ; i++)
	{
		cache_delete(i);
/*		if(cache_state.spare_region[i].used != FALSE)
		{
			SVGA_region_info_t *rinfo = &cache_state.spare_region[i].region;
			BYTE *free_ptr = (BYTE*)rinfo->address;
			
			if(rinfo->region_address != NULL)
			{
				dbg_printf(dbg_pagefree, rinfo->region_address);
				_PageFree((PVOID)rinfo->region_address, 0);
			}
			else
			{
				free_ptr -= P_SIZE;
			}
			
			if(rinfo->mob_address != NULL)
			{
				dbg_printf(dbg_pagefree, rinfo->mob_address);
				_PageFree((PVOID)rinfo->mob_address, 0);
			}
			
			dbg_printf(dbg_pagefree, free_ptr);
			_PageFree((PVOID)free_ptr, 0);
			
			cache_state.spare_region[i].used = FALSE;
		}*/
	}
	
	cache_state.free_index_max = 0;
	cache_state.free_index_min = 0;
	cache_state.cnt_large      = 0;
	cache_state.cnt_medium     = 0;
	cache_state.cnt_small      = 0;

	End_Critical_Section();
}


/**
 * GPU10: start context0
 *
 **/
static void SVGA_CB_start()
{
	if(cb_support)
	{
		cb_enable_t *cbe = cmdbuf;
		wait_for_cmdbuf();
		
		memset(cbe, 0, sizeof(cb_enable_t));
		cbe->cmd = SVGA_DC_CMD_START_STOP_CONTEXT;
		cbe->cbstart.enable  = 1;
		cbe->cbstart.context = SVGA_CB_CONTEXT_0;
		
		SVGA_CMB_submit(cmdbuf, sizeof(cb_enable_t), NULL, SVGA_CB_USE_CONTEXT_DEVICE|SVGA_CB_SYNC, 0);
		
		cb_context0 = TRUE;
	}
}

/**
 * GPU10: stop context0
 *
 **/
static void SVGA_CB_stop()
{
	cb_context0 = FALSE;
	
	if(cb_support)
	{
		cb_enable_t *cbe = cmdbuf;
		wait_for_cmdbuf();
		
		memset(cbe, 0, sizeof(cb_enable_t));
		cbe->cmd = SVGA_DC_CMD_START_STOP_CONTEXT;
		cbe->cbstart.enable  = 0;
		cbe->cbstart.context = SVGA_CB_CONTEXT_0;
		
		SVGA_CMB_submit(cmdbuf, sizeof(cb_enable_t), NULL, SVGA_CB_USE_CONTEXT_DEVICE|SVGA_CB_SYNC, 0);
	}
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
//		dbg_printf("3D requires the Extended FIFO capability.\n");
	}

	if(SVGA_HasFIFOCap(SVGA_FIFO_CAP_3D_HWVERSION_REVISED))
	{
		hwVersion = gSVGA.fifoMem[SVGA_FIFO_3D_HWVERSION_REVISED];
	}
	else
	{
		if(gSVGA.fifoMem[SVGA_FIFO_MIN] <= sizeof(uint32) * SVGA_FIFO_GUEST_3D_HWVERSION)
		{
			//dbg_printf("GUEST_3D_HWVERSION register not present.\n");
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

static DWORD SVGA_pitch(DWORD width, DWORD bpp)
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
	
	SVGA_CMB_submit(cmdbuf, cmdoff, NULL, SVGA_CB_SYNC/*|SVGA_CB_FORCE_FIFO*/, 0);
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
	
	/* Make sure, that we drain full FIFO */
	SVGA_Sync();
	SVGA_Flush_CB_critical(); 
      
	/* stop command buffer context 0 */
	SVGA_CB_stop();
            
	SVGA_SetModeLegacy(w, h, bpp); /* setup by legacy registry */
	SVGA_Flush_CB_critical(); /* make sure, that is really set */
	
	has3D = SVGA3D_Init();
      
	/* setting screen by fifo, this method is required in VB 6.1 */
	if(SVGA_hasAccelScreen())
	{
		SVGA_defineScreen(w, h, bpp);
		SVGA_Flush_CB_critical();
	}
	
	/* start command buffer context 0 */
	SVGA_CB_start();
     
	/*
	 * JH: this is a bit stupid = all SVGA command cannot work with non 32 bpp. 
	 * SVGA_CMD_UPDATE included. So if we're working in 32 bpp, we'll disable
	 * traces and updating framebuffer changes with SVGA_CMD_UPDATE.
	 * On non 32 bpp we just enable SVGA_REG_TRACES.
	 *
	 * QEMU hasn't SVGA_REG_TRACES register and framebuffer cannot be se to
	 * 16 or 8 bpp = we supporting only 32 bpp moders if we're running under it.
	 */
	if(bpp == 32)
	{
		SVGA_WriteReg(SVGA_REG_TRACES, FALSE);
	}
	else
	{
		SVGA_WriteReg(SVGA_REG_TRACES, TRUE);
	}
      
	SVGA_WriteReg(SVGA_REG_ENABLE, TRUE);
	SVGA_Sync();
	SVGA_Flush_CB_critical();
	
	hda->width   = SVGA_ReadReg(SVGA_REG_WIDTH);
	hda->height  = SVGA_ReadReg(SVGA_REG_HEIGHT);
	hda->bpp     = SVGA_ReadReg(SVGA_REG_BITS_PER_PIXEL);
	hda->pitch   = SVGA_ReadReg(SVGA_REG_BYTES_PER_LINE);
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

void FBHDA_access_end(DWORD flags)
{
	//dbg_printf(dbg_update, hda->width, hda->height, hda->bpp);
	fb_lock_cnt--;
	if(fb_lock_cnt < 0) fb_lock_cnt = 0;
	
	if(fb_lock_cnt == 0)
	{
		/*BOOL need_sync = */
		mouse_blit();
		
	  if(hda->bpp == 32)
	  {
	  	SVGAFifoCmdUpdate  *cmd_update;
	  	DWORD cmd_offset = 0;
	  	
	  	wait_for_cmdbuf();
	  	
	  	cmd_update = SVGA_cmd_ptr(cmdbuf, &cmd_offset, SVGA_CMD_UPDATE, sizeof(SVGAFifoCmdUpdate));
	  	cmd_update->x = 0;
	  	cmd_update->y = 0;
	  	cmd_update->width  = hda->width;
	  	cmd_update->height = hda->height;
#if 0
	  	if(need_sync)
	  	{
	  		SVGA_CMB_submit(cmdbuf, cmd_offset, NULL, SVGA_CB_SYNC, 0);
	  	}
	  	else
	  	{
	  		SVGA_CMB_submit(cmdbuf, cmd_offset, NULL, 0, 0);
	  	}
#else
			SVGA_CMB_submit(cmdbuf, cmd_offset, NULL, SVGA_CB_SYNC, 0);
#endif
	  }
	}
	
	//Signal_Semaphore(hda_sem);
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
