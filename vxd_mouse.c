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
	
	if(!mouse_buffer_mem) return FALSE;

	cur = (CURSORSHAPE*)mouse_buffer_mem;
	
	/* erase cursor if present */
	FBHDA_access_begin(0);
	
	Begin_Critical_Section(0);
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
		End_Critical_Section();
		
		FBHDA_access_end(0);
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
	
	//dbg_printf(dbg_cursor_empty, mouse_empty);
	End_Critical_Section();
	
	/* blit new cursor */
	FBHDA_access_end(0);
	
	mouse_notify_accel();
	
	return TRUE;
}

void mouse_move(int x, int y)
{
	if(mouse_valid && mouse_visible && !mouse_empty)
	{
		FBHDA_access_begin(0);
		mouse_x = x;
		mouse_y = y;
		FBHDA_access_end(0);
	}
	else
	{
		mouse_x = x;
		mouse_y = y;
	}
}

void mouse_show()
{
	mouse_visible = TRUE;
	
	mouse_notify_accel();
}

void mouse_hide()
{
	FBHDA_access_begin(0);
	mouse_visible = FALSE;
	FBHDA_access_end(0);
	
	mouse_notify_accel();
}

void mouse_invalidate()
{
	mouse_valid = FALSE;
	
	mouse_notify_accel();
}

/* called by FBHDA_access_end */
BOOL mouse_blit()
{
	if(mouse_valid && mouse_visible && !mouse_empty)
	{
		Begin_Critical_Section(0);
		draw_save(mouse_x, mouse_y);
		draw_blit(mouse_x, mouse_y);
		End_Critical_Section();
		return TRUE;
	}
	
	return FALSE;
}

/* called by FBHA_access_begin */
void mouse_erase()
{
	if(mouse_valid && mouse_visible && !mouse_empty)
	{
		Begin_Critical_Section(0);
		draw_restore();
		End_Critical_Section();
	}
}
