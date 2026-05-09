/*****************************************************************************

Copyright (c) 2026 Jaroslav Hensl <emulator@emulace.cz>

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

#include "wram.h"
#include "async.h"

#include "vxd_lib.h"
#include "3d_accel.h"

#include "code32.h"

extern FBHDA_t *hda;
extern DWORD ThisVM;

static DWORD uid_seq = 1;

typedef struct _surface_t
{
	DWORD off_start; /* inclusive */
	DWORD off_end; /* exclusive */
	DWORD uid;
	DWORD four_cc;
	DWORD width;
	DWORD height;
	DWORD pitch;
	void *watchman_ctx;
	FBHDA_DD_watch_callback_t watchman_proc;
	FBHDA_DD_surface_attrs_t attrs;
} surface_t;

#define DEFAULT_FIFO 16384
#define DEFAULT_SURFACES 1024

static surface_t *surfaces_tlb = NULL;
static DWORD surfaces_tlb_size = 0;
static DWORD surfaces_tlb_length = 0;

#define SURF_MATCH(_i, _off) \
	(surfaces_tlb[(_i)].off_start >= (_off) && \
	surfaces_tlb[(_i)].off_end < (_off))

#define SURF_MATCH_RANGE(_i, _off_start, _off_end) \
	(surfaces_tlb[(_i)].off_start >= (_off_end) && \
	surfaces_tlb[(_i)].off_end < (_off_start))

static BOOL surf_off_valid(void *flat)
{
	DWORD dw_flat = (DWORD)flat;
	DWORD dw_vram_start = (DWORD)hda->vram_pm32;
	DWORD dw_vram_end = dw_vram_start + hda->vram_size_virt;
	
	if(dw_flat >= dw_vram_start && dw_flat < dw_vram_end)
	{
		return TRUE;
	}
	return FALSE;
}

static DWORD surf_off(void *flat)
{
	return ((BYTE*)flat) - ((BYTE*)hda->vram_pm32);
}

static void *surf_flat(surface_t *s)
{
	return (void*)(((BYTE*)hda->vram_pm32) + s->off_start);
}

static DWORD surf_index(surface_t *surf)
{
	return ((BYTE*)surf - (BYTE*)surfaces_tlb)/sizeof(surface_t); // watcom...
}

static void surf_notify(DWORD msg, void *flat, DWORD uid)
{
	DWORD x = (hda->dd_fifo_top + 1) % hda->dd_fifo_length;
	hda->dd_fifo[x].action = msg;
	hda->dd_fifo[x].uid    = uid;
	hda->dd_fifo[x].surface_flat = flat;

	hda->dd_fifo_top = x;
}

static void surf_move(DWORD start_index, DWORD new_cnt, DWORD old_cnt)
{
	DWORD move_cnt = surfaces_tlb_length - start_index - old_cnt;
	DWORD move_src = start_index + old_cnt;
	DWORD move_dst = start_index + new_cnt;
	DWORD resize = new_cnt-old_cnt; /* may carry */

	dbg_printf("surf_move: %ld %ld %ld\n", start_index, new_cnt, old_cnt);
	
	if((surfaces_tlb_length+resize) > surfaces_tlb_size)
	{
		DWORD new_size = surfaces_tlb_size+DEFAULT_SURFACES;
		surface_t *new_buffer = (surface_t*)_PageAllocate(RoundToPages(new_size*sizeof(surface_t)), PG_VM, ThisVM, 0, PAGE_ALLOC_MIN, PAGE_ALLOC_MAX, NULL, 0);
		if(new_buffer)
		{
			surface_t *old_buffer = surfaces_tlb;
			memcpy(new_buffer, old_buffer, surfaces_tlb_size*sizeof(surface_t));
			surfaces_tlb = new_buffer;
			surfaces_tlb_size = new_size;
			_PageFree((void*)old_buffer, 0);
		}
		else
		{
			return;
		}
	}

	if(move_cnt)
	{
		memmove(surfaces_tlb+move_dst, surfaces_tlb+move_src, move_cnt*sizeof(surface_t));
	}

	surfaces_tlb_length += resize;
}

static BOOL surf_cmp(surface_t *s, void *flat, FBHDA_DD_surface_t *i)
{
	DWORD size = s->off_end - s->off_start;
	void *flat2 = surf_flat(s);
	
	if(flat == flat2)
	{
		if(size == i->size)
		{
			if(s->four_cc == i->four_cc)
			{
				if(s->height == i->height)
				{
					if(s->pitch == i->pitch)
					{
						return TRUE;
					}
				}
			}
		}
	}
	return FALSE;
}

#if 0
static BOOL sutf_tlb_lookup(void *flat, DWORD size, surface_t **first, DWORD *count)
{
	DWORD off = surf_off(flat);
	DWORD off_end = off + size;
	if(surfaces_tlb_length > 0)
	{
		DWORD step = surfaces_tlb_length / 2;
		DWORD index = step;
		BOOL match = FALSE;
		
		/* bisection method for find match surface */
		for(;;)
		{
			step /= 2;
			if(step == 0) step = 1;
			
			if(SURF_MATCH(index, off))
			{
				match = TRUE;
				break;
			}
			else if(surfaces_tlb[index].off_start < off)
			{
				if(index == 0) break; /* reach first item */
				index -= step;
			}
			else
			{
				if(index == surfaces_tlb_length-1) break; /* reach last item */
				index += step;
			}
		}
		
		if(match)
		{
			DWORD index_end = index+1;
			
			/* adjust position to the first item */
			/* TODO: we realy need this? */
			while(index > 0)
			{
				if(SURF_MATCH(index-1, off))
				{
					index--;
				}
				else break;
			}
			
			/* scan to last item */
			while(index_end < surfaces_tlb_length)
			{
				if(SURF_MATCH_RANGE(index_end, off, off_end))
				{
					index_end++;
				}
				else break;
			}
			
			*first = &surfaces_tlb[index];
			if(count)
			{
				*count = index_end - index;
			}
			return TRUE;
		}
	}
	
	return FALSE;
}
#else
static BOOL sutf_tlb_lookup(void *flat, DWORD size, surface_t **first, DWORD *count)
{
	DWORD off = surf_off(flat);
	DWORD i;
	DWORD cnt = 0;
	for(i = 0; i < surfaces_tlb_length; i++)
	{
		if(surfaces_tlb[i].off_end > off)
		{
			while(i < surfaces_tlb_length)
			{
				if(off + size > surfaces_tlb[i].off_start)
				{
					if(cnt == 0)
					{
						*first = surfaces_tlb+i;
					}
					cnt++;
				}
				else
				{
					break;
				}
				i++;
			}
			break;
		}
	}

	if(count != NULL)
	{
		*count = cnt;
	}

	return cnt > 0 ? TRUE : FALSE;
}
#endif

static void surf_watch(surface_t *surf, DWORD action)
{
	if(surf->watchman_proc != NULL)
	{
		void *this_ctx = _GetCurrentContext();
		if(this_ctx != surf->watchman_ctx)
		{
			void *new_ctx = _ContextSwitch(surf->watchman_ctx);
			if(new_ctx == surf->watchman_ctx)
			{
				surf->watchman_proc(surf_flat(surf), action);
			}
			_ContextSwitch(this_ctx);
		}
	}
}

void FBHDA_DD_init()
{
	const DWORD size_fifo = DEFAULT_FIFO * sizeof(FBHDA_DD_fifo_item_t);
	const DWORD size_surfs = DEFAULT_SURFACES * sizeof(surface_t);
	surfaces_tlb = (surface_t*)_PageAllocate(RoundToPages(size_surfs), PG_VM, ThisVM, 0, PAGE_ALLOC_MIN, PAGE_ALLOC_MAX, NULL, 0);
	if(surfaces_tlb != NULL)
	{
		surfaces_tlb_size = DEFAULT_SURFACES;
		surfaces_tlb_length = 0;
		
		hda->dd_fifo = (FBHDA_DD_fifo_item_t*)_PageAllocate(RoundToPages(size_fifo), PG_VM, ThisVM, 0, PAGE_ALLOC_MIN, PAGE_ALLOC_MAX, NULL, PAGEZEROINIT);
		if(hda->dd_fifo != NULL)
		{
			hda->dd_fifo_length = DEFAULT_FIFO;
			hda->dd_fifo_top = DEFAULT_FIFO-1;
			surf_notify(FBHDA_DD_NOP, NULL, 0);
		}
	}
}

BOOL FBHDA_DD_surface_get(void *flat, FBHDA_DD_surface_t *info)
{
	surface_t *s;
	//dbg_printf("FBHDA_DD_surface_get(%lX, %lX) ... ", flat, info);
	
	if(!surf_off_valid(flat))
	{
		//dbg_printf("FALSE\n");
		return FALSE;
	}

	if(sutf_tlb_lookup(flat, 1, &s, NULL))
	{
		info->uid     = s->uid;
		info->four_cc = s->four_cc;
		info->size    = s->off_end - s->off_start;
		info->width   = s->width;
		info->height  = s->height;
		info->pitch   = s->pitch;
		info->data    = (void*)(((BYTE*)hda->vram_pm32) + s->off_start);
		memcpy(&(info->attrs), &(s->attrs), sizeof(FBHDA_DD_surface_attrs_t));

		//dbg_printf("TRUE\n");
		return TRUE;
	}
	//dbg_printf("FALSE\n");
	return FALSE;
}

static void print_surfaces()
{
	DWORD i;
	dbg_printf("+-------------------\n");
	for(i = 0; i < surfaces_tlb_length; i++)
	{
		dbg_printf("|%lX:%ld|%lX|%d x %d|\n",
			surfaces_tlb[i].off_start + (DWORD)hda->vram_pm32,
			surfaces_tlb[i].off_end - surfaces_tlb[i].off_start,
			surfaces_tlb[i].four_cc,
			surfaces_tlb[i].width, surfaces_tlb[i].height);
	}
	dbg_printf("+-------------------\n");
}

BOOL FBHDA_DD_surface_set(void *flat, FBHDA_DD_surface_t *info)
{
	DWORD cnt;
	surface_t *first;

	if(!surf_off_valid(flat))
	{
		return FALSE;
	}

	dbg_printf("FBHDA_DD_surface_set(%lX:%ld)\n", flat, info->size);

	if(sutf_tlb_lookup(flat, info->size, &first, &cnt))
	{
		DWORD i;
		if(cnt == 1)
		{
			if(surf_cmp(first, flat, info))
			{
				surf_notify(FBHDA_DD_MODIFY, flat, first->uid);
				surf_watch(first, FBHDA_DD_MODIFY);
				info->uid = first->uid;
				return TRUE;
			}
		}

		for(i = 0; i < cnt; i++)
		{
			surf_notify(FBHDA_DD_DELETE, surf_flat(first+i), first[i].uid);
			surf_watch(first+i, FBHDA_DD_DELETE);
		}
		
		surf_move(surf_index(first), 1, cnt);
		
		first->off_start = ((BYTE*)flat) - ((BYTE*)hda->vram_pm32);
		first->off_end   = first->off_start + info->size;
		first->uid       = uid_seq++;
		first->four_cc   = info->four_cc;
		first->width     = info->width;
		first->height    = info->height;
		first->pitch     = info->pitch;
		first->watchman_proc = NULL;
		first->watchman_ctx  = NULL;
		memcpy(&(first->attrs), &(info->attrs), sizeof(FBHDA_DD_surface_attrs_t));
		
		print_surfaces();
	}
	else
	{
		DWORD index = 0;
		DWORD off = surf_off(flat);
		for(index = 0; index < surfaces_tlb_length; index++)
		{
			if(surfaces_tlb[index].off_start > off)
			{
				break;
			}
		}
		
		surf_move(index, 1, 0);
		
		surfaces_tlb[index].off_start = off;
		surfaces_tlb[index].off_end   = off + info->size;
		surfaces_tlb[index].uid       = uid_seq++;
		surfaces_tlb[index].four_cc   = info->four_cc;
		surfaces_tlb[index].width     = info->width;
		surfaces_tlb[index].height    = info->height;
		surfaces_tlb[index].pitch     = info->pitch;
		surfaces_tlb[index].watchman_proc = NULL;
		surfaces_tlb[index].watchman_ctx  = NULL;
		memcpy(&(surfaces_tlb[index].attrs), &(info->attrs), sizeof(FBHDA_DD_surface_attrs_t));
		
		print_surfaces();
	}
	
	//surf_notify(FBHDA_DD_CREATE, (DWORD)flat);
	return TRUE;
}

void FBHDA_DD_surface_delete(void *flat)
{
	surface_t *s;
	dbg_printf("FBHDA_DD_surface_delete(%lX)\n", flat);

	if(surf_off_valid(flat))
	{
		if(sutf_tlb_lookup(flat, 1, &s, NULL))
		{
			DWORD index = surf_index(s);
			surf_notify(FBHDA_DD_DELETE, flat, s->uid);
			surf_watch(s, FBHDA_DD_DELETE);
			surf_move(index, 0, 1);
		}
	}
	
	print_surfaces();
}

BOOL FBHDA_DD_surface_modify(void *flat)
{
	surface_t *s;
	dbg_printf("FBHDA_DD_surface_modify(%lX)\n", flat);

	if(surf_off_valid(flat))
	{
		if(sutf_tlb_lookup(flat, 1, &s, NULL))
		{
			surf_notify(FBHDA_DD_MODIFY, flat, s->uid);
			surf_watch(s, FBHDA_DD_MODIFY);
			return TRUE;
		}
	}
	return FALSE;
}

void FBHDA_DD_surface_notify(void *flat)
{
	surface_t *s;
	dbg_printf("FBHDA_DD_surface_notify(%lX)\n", flat);
	
	if(surf_off_valid(flat))
	{
		if(sutf_tlb_lookup(flat, 1, &s, NULL))
		{
			surf_notify(FBHDA_DD_NOTIFY, flat, s->uid);
			surf_watch(s, FBHDA_DD_NOTIFY);
		}
	}
}

BOOL FBHDA_DD_surface_watch(void *flat, FBHDA_DD_watch_callback_t callback)
{
	surface_t *s;
	if(surf_off_valid(flat))
	{
		if(sutf_tlb_lookup(flat, 1, &s, NULL))
		{
			s->watchman_proc = callback;
			if(callback)
			{
				s->watchman_ctx = _GetCurrentContext();
			}
			else
			{
				s->watchman_ctx = NULL;
			}
			return TRUE;
		}
	}
	return FALSE;
}
