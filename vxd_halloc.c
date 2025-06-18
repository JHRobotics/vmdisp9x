/*****************************************************************************

Copyright (c) 2025 Jaroslav Hensl <emulator@emulace.cz>

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

#include "winhack.h"
#include "vmm.h"
#include "vxd.h"

#include "vxd_lib.h"
#include "3d_accel.h"
#include "vxd_halloc.h"

#include "code32.h"

extern FBHDA_t *hda;

typedef struct _hblock
{
	BYTE *flat_start;
	BYTE *flat_end; // flat_start + pages*P_SIZE for faster calculations
	DWORD pages;
	DWORD max_alloc;
	BOOL  stats;
	DWORD id;
	DWORD *pageinfo; // first page in block number, e.g. if n == pageinfo[n], this is base block
	DWORD bitmap[1];
} hblock_t;

#define BLOCK_HUGE1 0 /* 256+ pages (1M) */
#define BLOCK_HUGE2 1 /* 256+ pages (1M) */
#define BLOCK_MEDIUM 1 /* 16+ pages (64k) */
#define BLOCK_LOW 1 /* 1-15 pages */

#define BLOCKS 4

static DWORD mem_total = 0;
static DWORD mem_used  = 0;

/*
	allocators
	RAM < 128 = 2D ONLY

	RAM: 128 MB = split 64/64 (SYS/GPU)
	BLOCK: 2 8 54 0
	
	RAM: 256 MB = split 128/128 (SYS/GPU)
	BLOCKS: 4 16 104 0
	
	RAM 512 MB = split 256/256 (SYS/GPU)
	BLOCKS: 4 16 236 0
	
	RAM 1024 MB = split 512/512 (SYS/GPU)
	BLOCKS 4 16 256 232
	
	RAM 2048 MB = split 1256/768 (SYS/GPU)
	BLOCKS 4 32 344 384
*/

static hblock_t *hblocks[BLOCKS] = {NULL, NULL, NULL, NULL};

#define _BP(_v, _pos) (((_v[(_pos) >> 5]) >> ((_pos) & 31)) & 1)

#define _BSET(_v, _pos) _v[(_pos) >> 5] |=   1 << ((_pos) & 31)
#define _BCLR(_v, _pos) _v[(_pos) >> 5] &= ~(1 << ((_pos) & 31))

/* fucking naive */
static DWORD find_hole(DWORD *bitmap, DWORD bitmax, DWORD count)
{
	DWORD i, j;
	DWORD i_max = (bitmax - count) + 1;
	for(i = 0; i < i_max; i++)
	{
		if(_BP(bitmap, i) == 0)
		{
			for(j = 1; j < count; j++)
			{
				if(_BP(bitmap, i+j) != 0)
				{
					break;
				}
			}
			if(j == count)
			{
				return i;
			}
			else
			{
				i += j; /* bit on j + i is non zero, so we can ship it by next iteration (i) */
			}
		}
	}
	return ~0;
}

static void block_set(DWORD *bitmap, DWORD pos, DWORD cnt)
{
	DWORD i;
	for(i = pos; i < pos+cnt; i++)
	{
		_BSET(bitmap, i);
	}
}

static void block_clr(DWORD *bitmap, DWORD pos, DWORD cnt)
{
	DWORD i;
	for(i = pos; i < pos+cnt; i++)
	{
		//dbg_printf("CLR: %d\n", cnt);
		_BCLR(bitmap, i);
	}
}

static void update_stats()
{
	if(hda)
	{
		hda->gpu_mem_total = mem_total;
		hda->gpu_mem_used = mem_used;
	}
	dbg_printf("TOTAL: %ld, USED: %ld\n", mem_total, mem_used);
}

static BOOL vxd_block(hblock_t *blk, DWORD pages_cnt, void **flat)
{
	DWORD i;
	DWORD hole = find_hole(blk->bitmap, blk->pages, pages_cnt);
	//dbg_printf("find_hole: %d\n", hole);

	if(hole == ~0)
	{
		dbg_printf("BLOCK %d: FH failed, PC: %ld\n", blk->id, pages_cnt);
		return FALSE;
	}

	*flat = blk->flat_start + P_SIZE * hole;

	block_set(blk->bitmap, hole, pages_cnt);

	for(i = hole; i < hole+pages_cnt; i++)
	{
		blk->pageinfo[i] = hole;
	}

	dbg_printf("BLOCK %d: ALLOC start: %ld, pages: %ld\n", blk->id, hole, pages_cnt);

	if(blk->stats)
	{
		mem_used += pages_cnt * P_SIZE;
		update_stats();
	}

	return TRUE;
}

static void vxd_block_free(hblock_t *blk, DWORD start_n)
{
	DWORD i;
	DWORD cnt = 0;
	for(i = start_n; i < blk->pages; i++)
	{
		if(blk->pageinfo[i] != start_n)
			break;

		cnt++;
	}
	
	dbg_printf("BLOCK %d: FREE start: %ld, pages: %ld\n", blk->id, start_n, cnt);

	if(cnt > 0)
	{
		block_clr(blk->bitmap, start_n, cnt);
		
		if(blk->stats)
		{
			mem_used -= cnt * P_SIZE;
			update_stats();
		}
		
		for(i = 0; i < cnt; i++)
		{
			blk->pageinfo[start_n+i] = ~0;
		}
		
	}
}

static hblock_t *vxd_hinit_block(DWORD pages_cnt, BOOL shared, BOOL stats)
{
	hblock_t *out = NULL;
	DWORD flat;
	DWORD base_size = sizeof(hblock_t) + (((pages_cnt+31))/32)*sizeof(DWORD);
	DWORD info_size = sizeof(DWORD)*pages_cnt;
	DWORD service_size = base_size + info_size;

	DWORD service_pages = (service_size + P_SIZE - 1)/P_SIZE;

	flat = _PageReserve(shared ? PR_SHARED : PR_SYSTEM, service_pages+pages_cnt, PR_FIXED);
	if(flat)
	{
		if(_PageCommit(flat >> 12, service_pages+pages_cnt, PD_FIXED, 0, PC_FIXED | PC_WRITEABLE | PC_USER) == 0)
		{
			dbg_printf("_PageCommit failed\n");
			_PageFree((PVOID)flat, 0);
			return NULL;
		}

		/*
		DWORD flat2 = _PageReAllocate(flat, service_pages+pages_cnt, 0);
		if(flat2 == 0)
		{

		}*/
	}

	if(flat)
	{
		DWORD i;
		out = (hblock_t*)flat;
		memset(out, 0, service_pages * P_SIZE);

		out->pageinfo = (DWORD*)(flat + base_size);
		out->flat_start = ((BYTE*)flat) + service_pages*P_SIZE;
		out->pages = pages_cnt;
		out->flat_end = out->flat_start + out->pages*P_SIZE;
		out->max_alloc = pages_cnt;

		if(stats)
		{
			out->stats = TRUE;
			mem_total += pages_cnt*P_SIZE;
		}

		for(i = 0; i < pages_cnt; i++)
		{
			out->pageinfo[i] = ~0;
		}
	}

	return out;
}

BOOL vxd_halloc(DWORD pages, void **flat/*, DWORD *phy*/)
{
	DWORD i;
	for(i = 0; i < BLOCKS; i++)
	{
		if(hblocks[i] != NULL)
		{
			if(hblocks[i]->max_alloc >= pages)
			{
				if(vxd_block(hblocks[i], pages, flat))
				{
					return TRUE;
				}
			}
		}
	}
	return FALSE;
}

void vxd_hfree(void *flat)
{
	BYTE *bflat = flat;
	DWORD i;
	for(i = 0; i < BLOCKS; i++)
	{
		if(hblocks[i] != NULL)
		{
			if(bflat >= hblocks[i]->flat_start && bflat < hblocks[i]->flat_end)
			{
				DWORD start_n = (bflat - hblocks[i]->flat_start)/P_SIZE;
				vxd_block_free(hblocks[i], start_n);
			}
		}
	}
}

#define RAM_MB128 ((100*1024*1024)/4096)
#define RAM_MB256 ((220*1024*1024)/4096)
#define RAM_MB512 ((470*1024*1024)/4096)
#define RAM_MB768 ((720*1024*1024)/4096)
#define RAM_MB1024 ((960*1024*1024)/4096)

BOOL vxd_hinit()
{
	// test = 4 16 236 0
	DWORD free_pages = _GetFreePageCount(0);
	dbg_printf("freepages: %ld\n", free_pages);

	if(free_pages >= RAM_MB1024)
	{
		hblocks[0] = vxd_hinit_block(2048, FALSE, FALSE); // 8 MB
		hblocks[1] = vxd_hinit_block(8192, FALSE, FALSE); // 32 MB
		hblocks[2] = vxd_hinit_block(32768, FALSE, TRUE); // 128 MB
		hblocks[3] = vxd_hinit_block(65536,  TRUE, TRUE); // 256 MB
	}
	else if(free_pages >= RAM_MB768)
	{
		hblocks[0] = vxd_hinit_block(2048, FALSE, FALSE); // 8 MB
		hblocks[1] = vxd_hinit_block(8192, FALSE, FALSE); // 32 MB
		hblocks[2] = vxd_hinit_block(32768, FALSE, TRUE); // 128 MB
		hblocks[3] = vxd_hinit_block(32768,  TRUE, TRUE); // 128 MB
	}
	else if(free_pages >= RAM_MB512)
	{
		hblocks[0] = vxd_hinit_block(2048, FALSE, FALSE); // 8 MB
		hblocks[1] = vxd_hinit_block(8192, FALSE, FALSE); // 32 MB
		hblocks[2] = vxd_hinit_block(32768, FALSE, TRUE); // 128 MB
	}
	else if(free_pages >= RAM_MB256)
	{
		hblocks[0] = vxd_hinit_block(2048, FALSE, FALSE); // 8 MB
		hblocks[1] = vxd_hinit_block(8192, FALSE, FALSE); // 32 MB
		hblocks[2] = vxd_hinit_block(16384, FALSE, TRUE); // 64 MB
	}
	else if(free_pages >= RAM_MB128)
	{
		hblocks[0] = vxd_hinit_block(1024, FALSE, FALSE); // 4 MB
		hblocks[1] = vxd_hinit_block(4096, FALSE, FALSE); // 16 MB
		hblocks[2] = vxd_hinit_block(8192, FALSE, TRUE);  // 32 MB
	}

	dbg_printf("vxd_hinit status: %lX %lX %lX %lX\n", hblocks[0], hblocks[1], hblocks[2], hblocks[3]);

	if(hblocks[0] && hblocks[1] && hblocks[2])
	{
		hblocks[0]->max_alloc = 15;
		hblocks[1]->max_alloc = 255;
		hblocks[0]->id = 1;
		hblocks[1]->id = 2;
		hblocks[2]->id = 3;
		if(hblocks[3])
		{
			hblocks[2]->id = 4;
		}
		return TRUE;
	}
	return FALSE;
}
