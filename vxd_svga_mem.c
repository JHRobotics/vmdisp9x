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

#include "svga_ver.h"

/*
 * consts
 */
#define PTONPAGE (P_SIZE/sizeof(DWORD))
#define MDONPAGE (P_SIZE/sizeof(SVGAGuestMemDescriptor))

#define SPARE_REGIONS_LARGE 2
#define SPARE_REGION_LARGE_SIZE (32*1024*1024)

#define SPARE_REGIONS_MEDIUM 16
#define SPARE_REGION_MEDIUM_SIZE (1024*1024)

#define SPARE_REGIONS_SMALL 128
//#define SPARE_REGION_SMALL_SIZE 4096

#define SPARE_REGIONS_CNT (SPARE_REGIONS_LARGE+SPARE_REGIONS_MEDIUM+SPARE_REGIONS_SMALL)

#define CACHE_THRESHOLD 128

/**
 * types
 */

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


/* object table for GPU10 */
static SVGA_OT_info_entry_t otable_setup[SVGA_OTABLE_DX_MAX] = {
	{0, NULL, RoundTo4k(VMWGFX_NUM_MOB*sizeof(SVGAOTableMobEntry)),                 0, 0}, /* SVGA_OTABLE_MOB */
	{0, NULL, RoundTo4k(VMWGFX_NUM_GB_SURFACE*sizeof(SVGAOTableSurfaceEntry)),      0, 0}, /* SVGA_OTABLE_SURFACE */
	{0, NULL, RoundTo4k(VMWGFX_NUM_GB_CONTEXT*sizeof(SVGAOTableContextEntry)),      0, 0}, /* SVGA_OTABLE_CONTEXT */
	{0, NULL, RoundTo4k(VMWGFX_NUM_GB_SHADER *sizeof(SVGAOTableShaderEntry)),       0, 0}, /* SVGA_OTABLE_SHADER - not used */
	{0, NULL, RoundTo4k(VMWGFX_NUM_GB_SCREEN_TARGET*sizeof(SVGAOTableScreenTargetEntry)), 0, 0}, /* SVGA_OTABLE_SCREENTARGET (VBOX_VIDEO_MAX_SCREENS) */
	{0, NULL, RoundTo4k(VMWGFX_NUM_DXCONTEXT*sizeof(SVGAOTableDXContextEntry)),    0, 0}, /* SVGA_OTABLE_DXCONTEXT */
};

static SVGA_OT_info_entry_t *otable = NULL;

static svga_cache_state_t cache_state = {0, 0};

static BOOL cache_enabled = FALSE;


/*
 * globals
 */
extern FBHDA_t *hda;
extern ULONG hda_sem;
static SVGA_DB_t *svga_db = NULL;

extern LONG fb_lock_cnt;

extern BOOL gb_support;


/**
 * Return PPN (physical page number) to virtual address
 *
 **/
static DWORD getPPN(DWORD virtualaddr)
{
	DWORD phy = 0;
	_CopyPageTable(virtualaddr/P_SIZE, 1, &phy, 0);
	return phy/P_SIZE;
}

/**
 * Return PPN for pointer
 *
 **/
static DWORD getPtrPPN(void *ptr)
{
	return getPPN((DWORD)ptr);
}

/**
 * Return page table pages need for buffer of specific size
 *
 **/
static DWORD PT_count(DWORD size)
{
	DWORD pt1_entries = 0;
	DWORD pt2_entries = 0;

	pt1_entries = RoundToPages(size);
	pt2_entries = ((pt1_entries + PTONPAGE - 1)/PTONPAGE);
	
//	dbg_printf(dbg_pt_build_2, size, pt1_entries, pt2_entries);
	
	if(pt2_entries > 1)
	{
		return pt2_entries + 1;
	}
	
	if(pt1_entries > 1)
	{
		return 1;
	}
	
	return 0;
}

/**
 * Build paget table for specific buffer
 *
 **/
static void PT_build(DWORD size, void *buf, DWORD *outBase, DWORD *outType, void **outUserPtr)
{
	DWORD pt1_entries = 0;
	DWORD pt2_entries = 0;
	DWORD i;
	
	pt1_entries = RoundToPages(size);
	pt2_entries = ((pt1_entries + PTONPAGE - 1)/PTONPAGE);

	if(pt2_entries > 1)
	{
		DWORD *ptbuf = buf;
		BYTE *ptr = ((BYTE*)buf)+((pt2_entries+1)*P_SIZE);
		
		memset(buf, 0, (pt2_entries+1)*P_SIZE);
		
		/* build PT2 */
		for(i = 0; i < pt2_entries; i++)
		{
			ptbuf[i] = getPtrPPN(ptbuf + ((i+1)*PTONPAGE));
		}
		
		ptbuf += PTONPAGE;
		
		/* build PT2 */
		for(i = 0; i < pt1_entries; i++)
		{
			ptbuf[i] = getPtrPPN(ptr + i*P_SIZE);
		}
		
		*outBase = getPtrPPN(buf);
		*outType = SVGA3D_MOBFMT_PTDEPTH_2;
		if(outUserPtr)
		{
			*outUserPtr = (void*)ptr;
		}
	}
	else if(pt1_entries > 1)
	{
		DWORD *ptbuf = buf;
		BYTE *ptr = ((BYTE*)buf)+P_SIZE;
		
		memset(buf, 0, P_SIZE);
		
		for(i = 0; i < pt1_entries; i++)
		{
			ptbuf[i] = getPtrPPN(ptr + i*P_SIZE);
		}
		
		*outBase = getPtrPPN(buf);
		*outType = SVGA3D_MOBFMT_PTDEPTH_1;
		if(outUserPtr)
		{
			*outUserPtr = (void*)ptr;
		}
	}
	else
	{
		*outBase = getPtrPPN(buf);
		*outType = SVGA3D_MOBFMT_PTDEPTH_0;
		if(outUserPtr)
		{
			*outUserPtr = (void*)buf;
		}
	}
	
//	dbg_printf(dbg_pt_build, size, *outBase, *outType, *outUserPtr);
}

/**
 * Allocate OTable for GB objects
 **/
void SVGA_OTable_alloc(BOOL screentargets)
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
	
	/* not allocate DX when there is no DX support */
	if(!SVGA_GetDevCap(SVGA3D_DEVCAP_DXCONTEXT))
	{
		otable[SVGA_OTABLE_DXCONTEXT].size = 0;
	}
	
	if(!screentargets)
	{
		otable[SVGA_OTABLE_SCREENTARGET].size = 0;
	}

	if(otable)
	{
		for(i = 0; i < SVGA_OTABLE_DX_MAX; i++)
		{
			SVGA_OT_info_entry_t *entry = &otable[i];
			if(entry->size != 0 && (entry->flags & SVGA_OT_FLAG_ALLOCATED) == 0)
			{
				void *ptr = (void*)_PageAllocate(RoundToPages(entry->size)+PT_count(entry->size), PG_VM, ThisVM, 0, 0x0, 0x100000, NULL, PAGEFIXED);
				
				if(ptr)
				{
//					dbg_printf(dbg_mob_allocate, i);
					PT_build(entry->size, ptr, &entry->ppn, &entry->pt_depth, &entry->lin);
					
					entry->flags |= SVGA_OT_FLAG_ALLOCATED;
					entry->flags |= SVGA_OT_FLAG_DIRTY;
				}
			}
		}
	}
}

SVGA_OT_info_entry_t *SVGA_OT_setup()
{
	return otable;
}

/**
 * Apply tables to VGPU
 **/
void SVGA_OTable_load()
{
	DWORD i;
	DWORD cmd_offset = 0;
	SVGA3dCmdSetOTableBase *cmd;

	SVGA_OT_info_entry_t *ot = SVGA_OT_setup();
	if(ot == NULL)
	{
		return;
	}
	
	wait_for_cmdbuf();

	for(i = SVGA_OTABLE_MOB; i < SVGA_OTABLE_DX_MAX; i++)
	{
		SVGA_OT_info_entry_t *entry = &(ot[i]);
		
		if((entry->flags & SVGA_OT_FLAG_ACTIVE) == 0)
		{
			if(entry->size > 0)
			{
				cmd = SVGA_cmd3d_ptr(cmdbuf, &cmd_offset, SVGA_3D_CMD_SET_OTABLE_BASE, sizeof(SVGA3dCmdSetOTableBase));
				cmd->type = i;
				cmd->baseAddress = entry->ppn;
				cmd->sizeInBytes = entry->size;
				if((entry->flags & SVGA_OT_FLAG_DIRTY) == 0)
				{
			  	cmd->validSizeInBytes = entry->size;
			  }
			  else
			  {
			  	cmd->validSizeInBytes = 0;			  	
			  }
			  cmd->ptDepth = entry->pt_depth;

			  entry->flags |= SVGA_OT_FLAG_ACTIVE;
			}
		}
	}

	submit_cmdbuf(cmd_offset, SVGA_CB_SYNC, 0);
}

void SVGA_OTable_unload()
{
	DWORD i;
	DWORD cmd_offset = 0;
	SVGA3dCmdSetOTableBase *cmd;
	SVGA3dCmdReadbackOTable *cmd_readback;

	SVGA_OT_info_entry_t *ot = SVGA_OT_setup();
	if(ot == NULL)
	{
		return;
	}
	
	wait_for_cmdbuf();

	for(i = SVGA_OTABLE_MOB; i < SVGA_OTABLE_DX_MAX; i++)
	{
		SVGA_OT_info_entry_t *entry = &(ot[i]);

		if(entry->flags & SVGA_OT_FLAG_ACTIVE)
		{
			if(entry->size > 0)
			{
				cmd_readback = SVGA_cmd3d_ptr(cmdbuf, &cmd_offset, SVGA_3D_CMD_READBACK_OTABLE, sizeof(SVGA3dCmdReadbackOTable));
				cmd_readback->type = i;
				entry->flags &= ~SVGA_OT_FLAG_DIRTY;
				
				cmd = SVGA_cmd3d_ptr(cmdbuf, &cmd_offset, SVGA_3D_CMD_SET_OTABLE_BASE, sizeof(SVGA3dCmdSetOTableBase));
				cmd->type = i;
				cmd->baseAddress = 0;
				cmd->sizeInBytes = 0;
			  cmd->validSizeInBytes = 0;
			  cmd->ptDepth = SVGA3D_MOBFMT_INVALID;

			  entry->flags &= ~SVGA_OT_FLAG_ACTIVE;
			}
		}
	}

	submit_cmdbuf(cmd_offset, SVGA_CB_SYNC, 0);
}

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
//			dbg_printf(dbg_pagefree, rinfo->region_address);
			_PageFree((PVOID)rinfo->region_address, 0);
		}
		else
		{
			free_ptr -= P_SIZE;
		}
			
		if(rinfo->mob_address != NULL)
		{
//			dbg_printf(dbg_pagefree, rinfo->mob_address);
			_PageFree((PVOID)rinfo->mob_address, 0);
		}
		else
		{
//			dbg_printf(dbg_pagefree, free_ptr);
			_PageFree((PVOID)free_ptr, 0);
		}
			
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

//		dbg_printf(dbg_cache_insert, region->region_id, region->size);
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
	
//	dbg_printf(dbg_cache_search, region->size);
	
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
				region->mob_pt_depth   = ptr->mob_pt_depth;
				
				cache_state.spare_region[i].used = FALSE;
				rc = TRUE;
				break;
			}
			else
			{
				if(++cache_state.spare_region[i].missed >= CACHE_THRESHOLD)
				{
//					dbg_printf(dbg_cache_delete, cache_state.spare_region[i].region.size);
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

void cache_init()
{
	memset(&cache_state, 0, sizeof(svga_cache_state_t));
}

void cache_enable(BOOL enabled)
{
	cache_enabled = enabled;
}

static DWORD pa_flags = PAGEFIXED;
static DWORD pa_align = 0x00000000;

void set_fragmantation_limit()
{
	DWORD max_len = SVGA_ReadReg(SVGA_REG_GMR_MAX_DESCRIPTOR_LENGTH);
	
	if(max_len < 1024)
	{
		pa_flags = PAGEFIXED | PAGEUSEALIGN;
		pa_align = 0x1F; // 128K
	}
	else if(max_len < 2048)
	{
		pa_flags = PAGEFIXED | PAGEUSEALIGN;
		pa_align = 0x0F; //  64K
	}
	else if(max_len < 4096)
	{
		pa_flags = PAGEFIXED | PAGEUSEALIGN;
		pa_align = 0x07; //  32K
	}
	else if(max_len < 8192)
	{
		pa_flags = PAGEFIXED | PAGEUSEALIGN;
		pa_align = 0x03; //  16K
	}
	else if(max_len < 16384)
	{
		pa_flags = PAGEFIXED | PAGEUSEALIGN;
		pa_align = 0x01; //  8K
	}
	else
	{
		pa_flags = PAGEFIXED;
		pa_align = 0x0;
	}
}


//#define GMR_CONTIG
//#define GMR_SYSTEM

//static DWORD pa_flags = PAGEFIXED | PAGEUSEALIGN;
//static DWORD pa_align = 0x00000003; // 64k

/*
0x00 =   4K
0x01 =   8K
0x03 =  16K
0x07 =  32K
0x0F =  64K
0x1F = 128K
*/

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
#ifdef GMR_CONTIG
	ULONG phy = 0;
#endif
	ULONG laddr;
	ULONG maddr = 0;
	ULONG pgblk;
	ULONG new_size = RoundTo4k(rinfo->size);	
	ULONG nPages = RoundToPages(new_size);
	//ULONG nPages = RoundToPages64k(rinfo->size);
	ULONG pa_vm = ThisVM;
	ULONG pa_type = PG_VM;
	SVGAGuestMemDescriptor *desc;
	ULONG size_total = 0;
	
	DWORD pt_pages = PT_count(new_size);

#ifdef GMR_SYSTEM
		pa_vm = 0;
		pa_type = PG_SYS;
#endif
	
	rinfo->size = new_size;
	
	if(cache_use(rinfo))
	{
		goto spare_region_used;
	}
	
	//dbg_printf(dbg_pages, rinfo->size, nPages, P_SIZE);
#ifdef GMR_CONTIG
	/* JH: OK, using PAGECONTIG leads to very fast memory exhaustion, so using only slower way! */
	/* first try to allocate continuous physical memory space, 1st page is used for GMR descriptor */
	laddr = _PageAllocate(nPages+1, pa_type, pa_vm, 0, 0x0, 0x100000, &phy, PAGECONTIG | PAGEUSEALIGN | PAGEFIXED);
	
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
		rinfo->mob_pt_depth   = SVGA3D_MOBFMT_RANGE;
		
		//dbg_printf(dbg_region_simple, laddr, rinfo->address);
		
		size_total = (nPages+1)*P_SIZE;
	}
	else
#endif
	{
		/* memory is too fragmented to create this large continous region */
		ULONG taddr;
		ULONG tppn;
		ULONG pgi;
		ULONG base_ppn;
		ULONG base_cnt;
		ULONG blocks;
		ULONG blk_pages = 0;
		ULONG blk_pages_raw = 0;
		
		/* allocate user block */
		if(!rinfo->mobonly)
		{
			maddr = _PageAllocate(nPages+pt_pages, pa_type, pa_vm, pa_align, 0x0, 0x100000, NULL, pa_flags);
		}
		else
		{
			maddr = _PageAllocate(nPages+pt_pages, pa_type, pa_vm, 0, 0x0, 0x100000, NULL, PAGEFIXED);
		}

		if(!maddr)
		{
			return FALSE;
		}

		laddr = maddr + pt_pages*P_SIZE;
		
		if(!rinfo->mobonly)
		{
			/* determine how many physical continuous blocks we have */
			base_ppn = getPPN(laddr);
			base_cnt = 1;
			blocks   = 1;
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
			blk_pages_raw = (blocks + MDONPAGE - 1)/MDONPAGE;
	
			// number of pages including pages-cross descriptors
			blk_pages = (blocks + blk_pages_raw + MDONPAGE - 1)/MDONPAGE;
			
//			dbg_printf(dbg_region_1, blocks+blk_pages_raw, SVGA_ReadReg(SVGA_REG_GMR_MAX_DESCRIPTOR_LENGTH));
	
			/* allocate memory for GMR descriptor */
			pgblk = _PageAllocate(blk_pages, pa_type, pa_vm, pa_align, 0x0, 0x100000, NULL, pa_flags);
			if(!pgblk)
			{
				_PageFree((PVOID)laddr, 0);
				return FALSE;
			}
	
			desc = (SVGAGuestMemDescriptor*)pgblk;
			memset(desc, 0, blk_pages*P_SIZE);
	
			blocks         = 1;
			desc->ppn      = getPPN(laddr);
			desc->numPages = 1;
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
					/* the next descriptor is on next page page */
					if(((blocks+1) % MDONPAGE) == 0)
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
			
//			dbg_printf(dbg_region_2, blocks);
		}

		rinfo->mob_address    = (void*)maddr;
		rinfo->mob_ppn        = 0;
		rinfo->mob_pt_depth   = 0;

		if(gb_support) /* don't create MOBs for Gen9 */
		{
			PT_build(new_size, (void*)maddr, &rinfo->mob_ppn, &rinfo->mob_pt_depth, NULL);
		}
		
		rinfo->address        = (void*)laddr;
		rinfo->region_address = (void*)pgblk;
		rinfo->region_ppn     = getPPN(pgblk);
				
		size_total = (nPages + blk_pages + pt_pages)*P_SIZE;
		
		//dbg_printf(dbg_region_fragmented);
	}
	
	//dbg_printf(dbg_gmr, rinfo->region_id, rinfo->region_ppn);
	
	spare_region_used:
	//dbg_printf(dbg_mobonly, rinfo->mobonly);
	
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
		void *mobcb;
		
		//wait_for_cmdbuf();
		mobcb = mob_cb_get();
		
  	//mob              = SVGA_cmd3d_ptr(cmdbuf, &cmdoff, SVGA_3D_CMD_DEFINE_GB_MOB, sizeof(SVGA3dCmdDefineGBMob));
  	mob              = SVGA_cmd3d_ptr(mobcb, &cmdoff, SVGA_3D_CMD_DEFINE_GB_MOB, sizeof(SVGA3dCmdDefineGBMob));
  	mob->mobid       = rinfo->region_id;
  	mob->base        = rinfo->mob_ppn;
		mob->ptDepth     = rinfo->mob_pt_depth;
		mob->sizeInBytes = rinfo->size;
		
		if(async_mobs == 1)
		{
			SVGA_CMB_submit(mobcb, cmdoff, NULL, SVGA_CB_SYNC, 0);
			SVGA_Sync();
		}
		else
		{
			SVGA_CMB_submit(mobcb, cmdoff, NULL, 0, 0);
		}
		
  	rinfo->is_mob = 1;
	}
	else
	{
		rinfo->is_mob = 0;
	}
	
	svga_db->stat_regions_usage += rinfo->size;
	
//	dbg_printf(dbg_gmr_succ, rinfo->region_id, rinfo->size);
	
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

	svga_db->stat_regions_usage -= rinfo->size;
	
	saved_in_cache = cache_insert(rinfo);
	
	if(gb_support)
	{
		SVGA3dCmdDestroyGBMob *mob;
		DWORD cmdoff = 0;
		void *mobcb = mob_cb_get();
		
  	wait_for_cmdbuf();
  	mob              = SVGA_cmd3d_ptr(mobcb, &cmdoff, SVGA_3D_CMD_DESTROY_GB_MOB, sizeof(SVGA3dCmdDestroyGBMob));
  	mob->mobid       = rinfo->region_id;
  	
  	if(saved_in_cache)
  	{
  		SVGA_CMB_submit(mobcb, cmdoff, NULL, 0, 0);
  	}
  	else
  	{
			SVGA_CMB_submit(mobcb, cmdoff, NULL, SVGA_CB_SYNC, 0);
		}
	}
	
	if(!rinfo->mobonly)
	{
		SVGA_Sync();
		SVGA_Flush_CB();
		
		SVGA_WriteReg(SVGA_REG_GMR_ID, rinfo->region_id);
		SVGA_WriteReg(SVGA_REG_GMR_DESCRIPTOR, 0);
		SVGA_Sync(); // notify register change
	}

	if(!saved_in_cache)
	{
		if(rinfo->region_address != NULL)
		{
			//dbg_printf(dbg_pagefree, rinfo->region_address);
			_PageFree((PVOID)rinfo->region_address, 0);
		}
		else
		{
			free_ptr -= P_SIZE;
		}
			
		if(rinfo->mob_address != NULL)
		{
			_PageFree((PVOID)rinfo->mob_address, 0);
		}
		else
		{
			_PageFree((PVOID)free_ptr, 0);
		}
	}
		
	//dbg_printf(dbg_pagefree_end, rinfo->region_id, rinfo->size, saved_in_cache);
		
	rinfo->address        = NULL;
	rinfo->region_address = NULL;
	rinfo->mob_address    = NULL;
	rinfo->region_ppn     = 0;
	rinfo->mob_ppn        = 0;
	rinfo->mob_pt_depth   = 0;
}

/**
 * Destroy saved regions and free memory
 *
 **/
void SVGA_flushcache()
{
	int i;
	
	for(i = 0; i < cache_state.free_index_max ; i++)
	{
		cache_delete(i);
	}
	
	cache_state.free_index_max = 0;
	cache_state.free_index_min = 0;
	cache_state.cnt_large      = 0;
	cache_state.cnt_medium     = 0;
	cache_state.cnt_small      = 0;
}

