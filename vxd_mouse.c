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

#include "winhack.h"
#include "vmm.h"
#include "vxd.h"

#include "vxd_lib.h"
#include "3d_accel.h"

#include "cursor.h"

#ifdef SVGA
#include "vxd_svga.h"
#endif

#include "code32.h"

extern FBHDA_t *hda;

static void *mouse_buffer_mem = NULL;
static void *mouse_andmask_data = NULL;
static void *mouse_xormask_data = NULL;
static void *mouse_swap_data = NULL;
static DWORD mouse_mem_size = 0;

static int mouse_x = 0;
static int mouse_y = 0;

static int mouse_w = 0;
static int mouse_h = 0;
static int mouse_pointx = 0;
static int mouse_pointy = 0;
static int mouse_swap_x = 0;
static int mouse_swap_y = 0;
static int mouse_swap_w = 0;
static int mouse_swap_h = 0;
static int mouse_swap_valid = FALSE;
static int mouse_ps = 0;

static BOOL  mouse_valid = FALSE;
static BOOL  mouse_empty = FALSE;
static BOOL  mouse_visible  = FALSE;

#define CUR_MIN_SIZE (32*32*4)

#include "vxd_strings.h"

#include "vxd_mouse_conv.h"

void *mouse_buffer()
{
	if(mouse_buffer_mem == NULL)
	{
		mouse_buffer_mem = (void*)_PageAllocate(RoundToPages(MOUSE_BUFFER_SIZE), PG_SYS, 0, 0, 0x0, 0x100000, NULL, PAGEFIXED);
	}
	
	return mouse_buffer_mem;
}

static void mouse_notify_accel()
{
	if(mouse_valid && mouse_visible && !mouse_empty)
	{
		hda->flags &= ~((DWORD)FB_MOUSE_NO_BLIT);
	}
	else
	{
		hda->flags |= FB_MOUSE_NO_BLIT;
	}
}

BOOL mouse_load()
{
	DWORD ms = 0;
	void *andmask_ptr;
	void *xormask_ptr;
	CURSORSHAPE *cur;
	DWORD cbw;
	
	//dbg_printf(dbg_mouse_load);
	
	if(!mouse_buffer_mem) return FALSE;
		
#ifdef SVGA
	if(SVGA_mouse_hw())
	{
		BOOL r = SVGA_mouse_load();
		mouse_valid = FALSE;
		mouse_notify_accel();
		return r;
	}
#endif

	cur = (CURSORSHAPE*)mouse_buffer_mem;
	
	/* erase cursor if present */
	FBHDA_access_begin(FBHDA_ACCESS_MOUSE_MOVE);
	
	mouse_valid = FALSE;
	
	/* check and alocate/resize buffer */
	ms = (cur->cx * cur->cy * 4);
	if(ms < CUR_MIN_SIZE)
		ms = CUR_MIN_SIZE;
	
	if(ms > mouse_mem_size)
	{
		if(mouse_andmask_data)
			_PageFree(mouse_andmask_data, 0);
		
		if(mouse_xormask_data)
			_PageFree(mouse_xormask_data, 0);
			
		if(mouse_swap_data)
			_PageFree(mouse_swap_data, 0);
			
		mouse_andmask_data = 
			(void*)_PageAllocate(RoundToPages(ms), PG_SYS, 0, 0, 0x0, 0x100000, NULL, PAGEFIXED);
		
		mouse_xormask_data = 
			(void*)_PageAllocate(RoundToPages(ms), PG_SYS, 0, 0, 0x0, 0x100000, NULL, PAGEFIXED);
		
		mouse_swap_data = 
			(void*)_PageAllocate(RoundToPages(ms), PG_SYS, 0, 0, 0x0, 0x100000, NULL, PAGEFIXED);
			
		mouse_mem_size = ms;
	}
	
	/* can't allocate memory */
	if(mouse_andmask_data == NULL ||
		mouse_xormask_data == NULL ||
		mouse_swap_data == NULL)
	{
		dbg_printf(dbg_mouse_no_mem);
		FBHDA_access_end(FBHDA_ACCESS_MOUSE_MOVE);
		return FALSE;
	}
	
	mouse_w = cur->cx;
	mouse_h = cur->cy;
	mouse_pointx = cur->xHotSpot;
	mouse_pointy = cur->yHotSpot;
	
	andmask_ptr = (void*)(cur + 1);
	xormask_ptr = (void*)(((BYTE*)andmask_ptr) + cur->cbWidth*cur->cy);
	
	mouse_ps = (hda->bpp + 7) / 8;
	
	cbw = (mouse_w+7)/8;
	
	/* AND mask (always 1bpp) */
	convmask(cur, cbw, andmask_ptr, mouse_andmask_data);
	
	/* XOR mask (1bpp or screen bpp) */
	if(cur->BitsPixel == 1)
	{
		convmask(cur, cbw, xormask_ptr, mouse_xormask_data);
	}
	else
	{
		memcpy(mouse_xormask_data, xormask_ptr, 
			((cur->BitsPixel + 7)/8) * cur->cx * cur->cy
		);
	}
	
	mouse_valid = TRUE;
	mouse_visible = TRUE;
	mouse_empty = cursor_is_empty();
	
	//dbg_printf(dbg_mouse_status, mouse_valid, mouse_visible, mouse_empty);
	
	//dbg_printf(dbg_cursor_empty, mouse_empty);

	/* blit new cursor */
	FBHDA_access_end(FBHDA_ACCESS_MOUSE_MOVE);
	
	mouse_notify_accel();
	
	return TRUE;
}

void mouse_move(int x, int y)
{
	//dbg_printf(dbg_mouse_move, x, y);
	
#ifdef SVGA
	if(SVGA_mouse_hw())
	{
		SVGA_mouse_move(x, y);
		return;
	}
#endif
	
	if(mouse_valid && mouse_visible && !mouse_empty)
	{
		FBHDA_access_begin(FBHDA_ACCESS_MOUSE_MOVE);
		mouse_x = x;
		mouse_y = y;
		FBHDA_access_end(FBHDA_ACCESS_MOUSE_MOVE);
	}
	else
	{
		mouse_x = x;
		mouse_y = y;
	}
}

void mouse_show()
{
	//dbg_printf(dbg_mouse_show);
	
#ifdef SVGA
	if(SVGA_mouse_hw())
	{
		SVGA_mouse_show();
		return;
	}
#endif
	FBHDA_access_begin(FBHDA_ACCESS_MOUSE_MOVE);
	mouse_visible = TRUE;
	FBHDA_access_end(FBHDA_ACCESS_MOUSE_MOVE);
	
	mouse_notify_accel();
}

void mouse_hide()
{
	//dbg_printf(dbg_mouse_hide);
	
#ifdef SVGA
	if(SVGA_mouse_hw())
	{
		SVGA_mouse_hide(FALSE);
		return;
	}
#endif
	
	FBHDA_access_begin(FBHDA_ACCESS_MOUSE_MOVE);
	mouse_visible = FALSE;
	FBHDA_access_end(FBHDA_ACCESS_MOUSE_MOVE);
	
	mouse_notify_accel();
}

void mouse_invalidate()
{
	//dbg_printf(dbg_mouse_invalidate);
	
#ifdef SVGA
	if(SVGA_mouse_hw())
	{
		SVGA_mouse_hide(TRUE);
		return;
	}
#endif
	
	mouse_valid = FALSE;
	
	mouse_notify_accel();
}

/* called by FBHDA_access_end */
BOOL mouse_blit()
{
	if(mouse_valid && mouse_visible && !mouse_empty)
	{
		draw_save(mouse_x, mouse_y);
		draw_blit(mouse_x, mouse_y);
		return TRUE;
	}
	
	return FALSE;
}

/* called by FBHDA_access_begin */
void mouse_erase()
{
	if(mouse_valid && mouse_visible && !mouse_empty)
	{
		draw_restore();
	}
}

BOOL mouse_get_rect(DWORD *ptr_left, DWORD *ptr_top,
	DWORD *ptr_right, DWORD *ptr_bottom)
{
	int mx;
	int my;
	int mw;
	int mh;
	
	if(mouse_valid && !mouse_empty)
	{
		mw = mouse_w;
		mx = mouse_x - mouse_pointx;
		
		if(mx < 0)
		{
			mw += mx;
			mx = 0;
		}
		
		if(mx + mw > hda->width)
		{
			mw = hda->width - mx;
		}
		
		mh = mouse_h;
		my = mouse_y - mouse_pointy;
		
		if(my < 0)
		{
			mh += my;
			my = 0;
		}
		
		if(my + mh > hda->height)
		{
			mh = hda->height - my;
		}
		
		if(mw > 0 && mh > 0)
		{
			*ptr_left    = mx;
			*ptr_top     = my;
			*ptr_right   = mx + mw;
			*ptr_bottom  = my + mh;
			
			return TRUE;
		}
		
		return TRUE;
	}
	
	return FALSE;
}
