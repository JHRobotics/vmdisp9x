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

/* 32 bit RING-0 code for SVGA-II HW mouse */
#define SVGA

#include <limits.h>

#include "winhack.h"
#include "vmm.h"
#include "vxd.h"
#include "vxd_lib.h"

#include "svga_all.h"

#include "3d_accel.h"

#include "cursor.h"

#include "code32.h"

#include "vxd_svga.h"

#include "vxd_strings.h"

extern FBHDA_t *hda;

static BOOL hw_cursor_valid = FALSE;
static BOOL hw_cursor_visible = FALSE;
static DWORD mouse_last_x = 0;
static DWORD mouse_last_y = 0;

static DWORD conv_mask16to32(void *in, void *out, int w, int h)
{
	WORD *in16 = in;
	DWORD *out32 = out;
	int x, y;
	
	for(y = 0; y < h; y++)
	{
		for(x = 0; x < w; x++)
		{
			DWORD px16 = *in16;
			DWORD px32 = ((px16 & 0xF800) << 8) | ((px16 & 0x07E0) << 5) | ((px16 & 0x001F) << 3);
			*out32 = px32;
			
			in16++;
			out32++;
		}
	}
	
	return w * h * sizeof(DWORD);
}

static DWORD conv_mask1to32(void *in, void *out, int w, int h, int pitch)
{
	BYTE *in1 = in;
	DWORD *out32 = out;
	int x, y;
	
	for(y = 0; y < h; y++)
	{
		for(x = 0; x < w; x++)
		{
			DWORD b = ((in1[x >> 3] << (x & 0x7)) >> 7) & 0x1;
			out32[x] = b ? 0x00FFFFFF : 0;
		}
		
		out32 += w;
		in1 += pitch;
	}
	
	return w * h * sizeof(DWORD);
}

static DWORD copy_mask1(void *in, void *out, int w, int h, int pitch)
{
	BYTE *in8 = in;
	BYTE *out8 = out;
	DWORD bw = pitch;
	DWORD pad = bw % 4;
	int y;
	
	for(y = 0; y < h; y++)
	{
		memcpy(out8, in8, bw);
		in8 += bw;
		out8 += bw + pad;
	}
	
	return (bw + pad) * h;
}

static DWORD conv_mask(void *in, DWORD in_bpp, void *out, DWORD *out_bpp, int w, int h, int pitch, BOOL force32bpp)
{
	switch(in_bpp)
	{
		case 1:
			if(force32bpp)
			{
				*out_bpp = 32;
				return conv_mask1to32(in, out, w, h, pitch);
			}
			*out_bpp = 1;
			return copy_mask1(in, out, w, h, pitch);
		case 16:
			*out_bpp = 32;
			return conv_mask16to32(in, out, w, h);
		case 32:
		{
			DWORD s = w * h * sizeof(DWORD);
			
			*out_bpp = 32;
			memcpy(out, in, s);
			return s;
		}
	}
	
	return 0;
}

BOOL SVGA_mouse_load()
{
	SVGAFifoCmdDefineCursor *cursor;
	DWORD cmdoff = 0;
	DWORD mask_size;
	void *mb;
	CURSORSHAPE *cur;
	
	SVGA_mouse_hide(TRUE);
	
	mb = mouse_buffer();
	
	if(!mb)
	{
		return FALSE;
	}
	cur = (CURSORSHAPE*)mb;
	
	if(cur->cx == 0 || cur->cy == 0)
	{
		return FALSE;
	}
		
	wait_for_cmdbuf();
	
  cursor = SVGA_cmd_ptr(cmdbuf, &cmdoff, SVGA_CMD_DEFINE_CURSOR, sizeof(SVGAFifoCmdDefineCursor));

  mask_size = conv_mask(cur+1, 1, ((BYTE*)cmdbuf)+cmdoff, &(cursor->andMaskDepth),
  	cur->cx, cur->cy, cur->cbWidth, FALSE);
  cmdoff += mask_size;
  mask_size = conv_mask(((BYTE*)(cur+1)) + cur->cbWidth * cur->cy, cur->BitsPixel, ((BYTE*)cmdbuf)+cmdoff, &(cursor->xorMaskDepth),
  	cur->cx, cur->cy, cur->cbWidth, TRUE);

  cmdoff += mask_size;
	
	cursor->id = 0;
	cursor->hotspotX = cur->xHotSpot;
	cursor->hotspotY = cur->yHotSpot;
	cursor->width    = cur->cx;
	cursor->height   = cur->cx;
	
	submit_cmdbuf(cmdoff, SVGA_CB_SYNC, 0);
	
	hw_cursor_valid = TRUE;
	hw_cursor_visible = TRUE;
	SVGA_mouse_move(mouse_last_x, mouse_last_y);
	
	return TRUE;
}

BOOL SVGA_mouse_hw()
{
	if(st_used)
	{
		if(st_flags & ST_CURSOR)
		{
			if(SVGA_HasFIFOCap(SVGA_FIFO_CAP_CURSOR_BYPASS_3))
			{
				return TRUE;
			}
		}
	}
	
	return FALSE;
}

void SVGA_mouse_move(int x, int y)
{
	if(hw_cursor_visible)
	{
		gSVGA.fifoMem[SVGA_FIFO_CURSOR_SCREEN_ID] = 0;
		gSVGA.fifoMem[SVGA_FIFO_CURSOR_ON] = 1;
		gSVGA.fifoMem[SVGA_FIFO_CURSOR_X] = x;
		gSVGA.fifoMem[SVGA_FIFO_CURSOR_Y] = y;
		gSVGA.fifoMem[SVGA_FIFO_CURSOR_COUNT]++;
	}
	
//	dbg_printf(dbg_hw_mouse_move, x, y, hw_cursor_visible, hw_cursor_valid);
	
	mouse_last_x = x;
	mouse_last_y = y;
}

void SVGA_mouse_show()
{
//	dbg_printf(dbg_hw_mouse_show, hw_cursor_valid);
	if(hw_cursor_valid)
	{
		hw_cursor_visible = TRUE;
		SVGA_mouse_move(mouse_last_x, mouse_last_y);
	}
}

void SVGA_mouse_hide(BOOL invalidate)
{
//	dbg_printf(dbg_hw_mouse_hide);

	gSVGA.fifoMem[SVGA_FIFO_CURSOR_SCREEN_ID] = 0;
	gSVGA.fifoMem[SVGA_FIFO_CURSOR_ON] = 0;
	
	/* vbox bug, move cursor outside screen */
	if((st_flags & ST_CURSOR_HIDEABLE) == 0)
	{
		gSVGA.fifoMem[SVGA_FIFO_CURSOR_X] = hda->width;
		gSVGA.fifoMem[SVGA_FIFO_CURSOR_Y] = hda->height;
	}
		
	gSVGA.fifoMem[SVGA_FIFO_CURSOR_COUNT]++;
	
	if(invalidate)
	{
		hw_cursor_valid = FALSE;
	}
	
	hw_cursor_visible = FALSE;
}
