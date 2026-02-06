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

#include "wram.h"
#include "code32.h"

extern FBHDA_t *hda;

static void *mouse_buffer_mem = NULL;

#define CUR_MIN_SIZE (32*32*4)

#include "vxd_strings.h"

#include "vxd_mouse_conv.h"

void *mouse_buffer()
{
	if(mouse_buffer_mem == NULL)
	{
		mouse_buffer_mem = (void*)_PageAllocate(RoundToPages(MOUSE_BUFFER_SIZE), PG_SYS, 0, 0, PAGE_ALLOC_MIN, PAGE_ALLOC_MAX, NULL, PAGEFIXED|PAGEUSEALIGN);
	}
	
	return mouse_buffer_mem;
}

static void mouse_notify_accel()
{
	hda->flags |= FB_MOUSE_NO_BLIT;
}

BOOL mouse_load()
{
	DWORD ms = 0;
	void *andmask_ptr;
	void *xormask_ptr;
	CURSORSHAPE *cur;
	DWORD cbw;
	LONG x, y;
	
	//dbg_printf(dbg_mouse_load);
	
	if(!mouse_buffer_mem) return FALSE;
		
#ifdef SVGA
	if(SVGA_mouse_hw())
	{
		BOOL r = SVGA_mouse_load();
		wram->regs.s.cursor_empty = 1;
		wram->regs.s.cursor_visible = 0;
		mouse_notify_accel();
		return r;
	}
#endif

	cur = (CURSORSHAPE*)mouse_buffer_mem;
	
	/* erase cursor if present */
	FBHDA_access_begin(FBHDA_ACCESS_MOUSE_MOVE);

	/* check and alocate/resize buffer */
	ms = (cur->cx * cur->cy * 4);
	if(ms < CUR_MIN_SIZE)
		ms = CUR_MIN_SIZE;


	x = wram->regs.s.cursor_x + wram->regs.s.cursor_spotx;
	y = wram->regs.s.cursor_y + wram->regs.s.cursor_spoty;

	wram->regs.s.cursor_spotx = cur->xHotSpot;
	wram->regs.s.cursor_spoty = cur->yHotSpot;

	wram->regs.s.cursor_x = x - wram->regs.s.cursor_spotx;
	wram->regs.s.cursor_y = y - wram->regs.s.cursor_spoty;
	wram->regs.s.cursor_w = cur->cx;
	wram->regs.s.cursor_h = cur->cy;

	andmask_ptr = (void*)(cur + 1);
	xormask_ptr = (void*)(((BYTE*)andmask_ptr) + cur->cbWidth*cur->cy);

	/* AND mask (always 1bpp) */
	expandmask(cur, cur->cbWidth, andmask_ptr, wram->cursor.andmask, 1);
	
	/* XOR mask (1bpp or screen bpp) */
	if(cur->BitsPixel == 1)
	{
		expandmask(cur, cur->cbWidth, xormask_ptr, wram->cursor.xormask, 1);
	}
	else
	{
		cbw = (cur->BitsPixel * cur->cx + 7) / 8;
		expandmask(cur, cbw, xormask_ptr, wram->cursor.xormask, cur->BitsPixel);
	}
	
	wram->regs.s.cursor_empty = cursor_is_empty();
	wram->regs.s.cursor_visible  = !wram->regs.s.cursor_empty;
	
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
	
	if(wram->regs.s.cursor_visible)
	{
		FBHDA_access_begin(FBHDA_ACCESS_MOUSE_MOVE);
		wram->regs.s.cursor_x = x - wram->regs.s.cursor_spotx;
		wram->regs.s.cursor_y = y - wram->regs.s.cursor_spoty;
		FBHDA_access_end(FBHDA_ACCESS_MOUSE_MOVE);
	}
	else
	{
		wram->regs.s.cursor_x = x - wram->regs.s.cursor_spotx;
		wram->regs.s.cursor_y = y - wram->regs.s.cursor_spoty;
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
	wram->regs.s.cursor_visible = !wram->regs.s.cursor_empty;
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
	wram->regs.s.cursor_visible = 0;
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
	
	wram->regs.s.cursor_empty   = 0;
	wram->regs.s.cursor_visible = 0;
	mouse_notify_accel();
}

BOOL mouse_get_rect(DWORD *ptr_left, DWORD *ptr_top,
	DWORD *ptr_right, DWORD *ptr_bottom)
{
	LONG mx;
	LONG my;
	LONG mw;
	LONG mh;
	
	if(wram->regs.s.cursor_visible)
	{
		mw = wram->regs.s.cursor_w;
		mx = wram->regs.s.cursor_x;
		
		if(mx < 0)
		{
			mw += mx;
			mx = 0;
		}
		
		if(mx + mw > wram->regs.s.width)
		{
			mw = wram->regs.s.width - mx;
		}
		
		mh = wram->regs.s.cursor_h;
		my = wram->regs.s.cursor_y;
		
		if(my < 0)
		{
			mh += my;
			my = 0;
		}
		
		if(my + mh > wram->regs.s.height)
		{
			mh = wram->regs.s.height - my;
		}
		
		if(mw > 0 && mh > 0)
		{
			*ptr_left    = (DWORD)mx;
			*ptr_top     = (DWORD)my;
			*ptr_right   = (DWORD)(mx + mw);
			*ptr_bottom  = (DWORD)(my + mh);
			
			return TRUE;
		}
	}

	return FALSE;
}
