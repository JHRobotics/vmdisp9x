/*****************************************************************************

Copyright (c) 2023  Jaroslav Hensl

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
#include <gdidefs.h>
#include <dibeng.h>
#include <minivdd.h>
#include <string.h>

#include "minidrv.h"
#include "dpmi.h"
#include "drvlib.h"
#include "swcursor.h"

struct _SWCURSOR
{
	long  x; /* pointer X */
	long  y; /* pointer Y */
	long  offx;
	long  offy;
	long  w;
	long  h;
	long  hotx;
	long  hoty;
	long  lastx; /* blitted offx */
	long  lasty; /* blitted offy */
	long  lastw;
	long  lasth;
	DWORD ps;
	DWORD state;
	long  blitlock;
	BYTE data[1];
};

#define STATE_EMPTY  0
#define STATE_BLITED 1
#define STATE_ERASED 2

BOOL sw_cursor_enabled = FALSE;
SWCURSOR __far *sw_cursor = NULL;
static BYTE __far *cursor_andmask  = NULL;
static BYTE __far *cursor_xormask  = NULL;
static BYTE __far *cursor_backdata = NULL;

#define CURSOR_MAX_SIZE 65536

static DWORD sw_cursor_lin = 0;
static  WORD fb_desc = 0;
static DWORD fb_desc_base = 0;

#pragma code_seg( _TEXT )

static void __far *GetLinePtr(long y)
{
	DWORD y_lin = dwScreenFlatAddr + (DWORD)wScreenPitchBytes*y;
	DWORD y_lin_max = y_lin + wScreenPitchBytes;
	
	if(fb_desc_base > y_lin ||
		y_lin-fb_desc_base > 65536 ||
		y_lin_max-fb_desc_base > 65536
		)
	{
		DPMI_SetSegBase(fb_desc, y_lin);
		fb_desc_base = y_lin;
	}
	
	return fb_desc :> (WORD)(y_lin - fb_desc_base);
}

void cursor_merge_rect(longRECT __far *r1, longRECT __far *r2)
{
	if(r1->left == r1->right)
	{
		if(r2->left == r2->right)
		{
			return;
		}
		
		*r1 = *r2;
		return;
	}
	
	if(r2->left == r2->right)
	{
		return;
	}
	
	if(r2->left < r1->left)
		r1->left = r2->left;
	
	if(r2->top < r1->top)
		r1->top = r2->top;
	
	if(r2->right > r1->right)
		r1->right = r2->right;
	
	if(r2->bottom > r1->bottom)
		r1->bottom = r2->bottom;
}

static void cursor_cs_enter()
{
	_asm
	{
		mov ax, 1681h
		int 2Fh
	};
}

static void cursor_cs_leave()
{
	_asm
	{
		mov ax, 1682h
		int 2Fh
	};
}


BOOL cursor_init()
{
	cursor_cs_enter();
	
	fb_desc = DPMI_AllocLDTDesc(1);
	
	if(fb_desc)
	{
		SWCURSOR __far *cur = drv_malloc(CURSOR_MAX_SIZE, &sw_cursor_lin);
		if(cur)
		{
			fb_desc_base = dwScreenFlatAddr;
			DPMI_SetSegBase(fb_desc, fb_desc_base);
			DPMI_SetSegLimit(fb_desc, CURSOR_MAX_SIZE-1);
			_fmemset(cur, 0, sizeof(SWCURSOR));
			cur->state = STATE_EMPTY;
			
			sw_cursor = cur;
			sw_cursor_enabled = TRUE;
			
			cursor_cs_leave();
			return TRUE;
		}
	}
	
	cursor_cs_leave();
	return FALSE;
}

#define EXPAND_BIT(_dsttype, _bit) (~(((_dsttype)(_bit))-1))
//#define EXPAND_BIT(_dsttype, _bit) ((_bit) > 0 ? 0xFFFFFFFF : 0)

/*
 * load cursor from CURSHAPE and convert it to display BPP
 */
BOOL cursor_load(CURSORSHAPE __far *lpCursor, longRECT __far *changes)
{
	//dbg_printf("cursor_load: %d\n", wBpp);
	
	if(sw_cursor)
	{
		unsigned int ps = (wBpp + 7)/8;
		const DWORD data_size = lpCursor->cx *  lpCursor->cy * ps;
		
		BYTE __far *DstAndMask = &sw_cursor->data[0];
		BYTE __far *DstXorMask = &sw_cursor->data[data_size];
		
		BYTE __far *andmask = (BYTE __far *)(lpCursor + 1);
		BYTE __far *xormask = (BYTE __far *)(andmask + lpCursor->cbWidth*lpCursor->cy);
		
		longRECT r1 = {0, 0, 0, 0};
		longRECT r2 = {0, 0, 0, 0};
		BOOL is_blitted = FALSE;
		
		/* maximum cursor size in 32bit mode is 64x64 px */
		if(data_size * 3 + sizeof(SWCURSOR) > CURSOR_MAX_SIZE)
		{
			return FALSE;
		}
				
		if(sw_cursor->state == STATE_BLITED)
		{
			is_blitted = TRUE;
			cursor_erase(&r1);
		}
		
		cursor_cs_enter();
	
		if(lpCursor->BitsPixel == 1)
		{
			int xi, xj, y;
			
			for(y = 0; y < lpCursor->cy; y++)
			{
				for(xj = 0; xj < lpCursor->cbWidth; xj++)
				{
					BYTE a = andmask[lpCursor->cbWidth*y + xj];
					BYTE x = xormask[lpCursor->cbWidth*y + xj];
					
					for(xi = 7; xi >= 0; xi--)
					{
						switch(wBpp)
						{
							case 8:
							{
								DstAndMask[lpCursor->cx * y + xj*8 + (7-xi)] = EXPAND_BIT(BYTE, ((a >> xi) & 0x1));
								DstXorMask[lpCursor->cx * y + xj*8 + (7-xi)] = EXPAND_BIT(BYTE, ((x >> xi) & 0x1));
								break;
							}
							case 16:
							{
								((WORD __far*)DstAndMask)[lpCursor->cx * y + xj*8 + (7-xi)] = EXPAND_BIT(WORD, ((a >> xi) & 0x1));
								((WORD __far*)DstXorMask)[lpCursor->cx * y + xj*8 + (7-xi)] = EXPAND_BIT(WORD, ((x >> xi) & 0x1));
								break;
							}
							case 24:
							{
								DWORD am = EXPAND_BIT(DWORD, ((a >> xi) & 0x1));
								DWORD xm = EXPAND_BIT(DWORD, ((x >> xi) & 0x1));
								WORD pos = (lpCursor->cx * y + xj*8 + (7-xi))*3;
								
								DstAndMask[pos]   =  am & 0xFF;
								DstAndMask[pos+1] = (am >> 8) & 0xFF;
								DstAndMask[pos+2] = (am >> 16) & 0xFF;
								
								DstXorMask[pos]   =  xm & 0xFF;
								DstXorMask[pos+1] = (xm >> 8) & 0xFF;
								DstXorMask[pos+2] = (xm >> 16) & 0xFF;
								
								break;
							}
							case 32:
							{
								((DWORD __far*)DstAndMask)[lpCursor->cx * y + xj*8 + (7-xi)] = EXPAND_BIT(DWORD, ((a >> xi) & 0x1));
								((DWORD __far*)DstXorMask)[lpCursor->cx * y + xj*8 + (7-xi)] = EXPAND_BIT(DWORD, ((x >> xi) & 0x1));
								break;
							}
						}
					}
				}
			}
		}
		else if(lpCursor->BitsPixel == wBpp)
		{
			_fmemcpy(DstAndMask, andmask, ps * lpCursor->cx * lpCursor->cy);
			_fmemcpy(DstXorMask, xormask, ps * lpCursor->cx * lpCursor->cy);
		}
		else
		{
			cursor_cs_leave();
			return FALSE;
		}
				
		sw_cursor->x = cursorX;
		sw_cursor->y = cursorY;
				
		sw_cursor->w = lpCursor->cx;
		sw_cursor->h = lpCursor->cy;
				
		sw_cursor->hotx = lpCursor->xHotSpot;
		sw_cursor->hoty = lpCursor->yHotSpot;
		
		sw_cursor->ps = ps;
		if(sw_cursor->state != STATE_BLITED)
		{
			sw_cursor->state = STATE_ERASED;
		}
		
		cursor_andmask  = DstAndMask;
		cursor_xormask  = DstXorMask;
		cursor_backdata = (BYTE __far *)sw_cursor + (CURSOR_MAX_SIZE - 64*64*4);//DstXorMask + data_size;
		
		cursor_cs_leave();
		
		if(is_blitted)
		{
			cursor_coordinates(cursorX, cursorY);
			cursor_blit(&r2);
			cursor_merge_rect(&r1, &r2);
		}
		
		if(changes)
		{
			*changes = r1;
		}
		
		return TRUE;
	}
	return FALSE;
}

void cursor_unload(longRECT __far *changes)
{
	//dbg_printf("cursor_unload\n");
	
	if(sw_cursor)
	{
		longRECT r1 = {0,0,0,0};
		
		if(sw_cursor->state == STATE_BLITED)
		{
			cursor_erase(&r1);
		}

		sw_cursor->state = STATE_EMPTY;
		
		if(changes)
		{
			*changes = r1;
		}
	}
}

void cursor_coordinates(long curx, long cury)
{
	if(sw_cursor)
	{
		cursor_cs_enter();
		if(sw_cursor->state != STATE_BLITED)
		{
			sw_cursor->x = curx;
			sw_cursor->y = cury;
			sw_cursor->offx = curx - sw_cursor->hotx;
			sw_cursor->offy = cury - sw_cursor->hoty;
		}
		cursor_cs_leave();
	}
}

void cursor_move(longRECT __far *changes)
{
	if(sw_cursor)
	{
		if(sw_cursor->blitlock == 0)
		{
			longRECT r1 = {0, 0, 0, 0}, r2 = {0, 0, 0, 0};
			BOOL blitted = FALSE;
			
			if(sw_cursor->state == STATE_EMPTY)
			{
				return;
			}
			
			if(sw_cursor->state == STATE_BLITED)
			{
				cursor_erase(&r1);
				blitted = TRUE;
			}
			
			cursor_coordinates(cursorX, cursorY);
			
			if(blitted)
			{
				cursor_blit(&r2);
			}
			
			if(changes)
			{
				cursor_merge_rect(&r1, &r2);
				*changes = r1;
			}
		}
	}
}

void cursor_erase(longRECT __far *changes)
{
	//dbg_printf("cursor_erase\n");
	
	if(sw_cursor)
	{
		long x, y;
		
		if(sw_cursor->ps != (wBpp + 7)/8)
		{
			return;
		}
		
		cursor_cs_enter();
		if(sw_cursor->state == STATE_BLITED && sw_cursor->blitlock == 0)
		{
			sw_cursor->blitlock++;
			switch(sw_cursor->ps)
			{
				case 1:
				{
					for(y = sw_cursor->lasty; y < sw_cursor->lasty + sw_cursor->lasth; y++){
						if(y >= 0 && y < wScrY){
							BYTE __far *dst = GetLinePtr(y);
							BYTE __far *src = cursor_backdata + ((y - sw_cursor->lasty)*sw_cursor->w);
							for(x = sw_cursor->lastx; x < sw_cursor->lastx + sw_cursor->lastw; x++){
								if(x >= 0 && x < wScrX){
									dst[x] = *src;
								}
								src++;
							} } }
					break;
				}
				case 2:
				{
					for(y = sw_cursor->lasty; y < sw_cursor->lasty + sw_cursor->lasth; y++){
						if(y >= 0 && y < wScrY){
							WORD __far *dst = GetLinePtr(y);
							WORD __far *src = (WORD __far *)cursor_backdata + ((y - sw_cursor->lasty)*sw_cursor->w);
							for(x = sw_cursor->lastx; x < sw_cursor->lastx + sw_cursor->lastw; x++){
								if(x >= 0 && x < wScrX){
									dst[x] = *src;
								}
								src++;
							} } }
					break;
				}
				case 3:
				{
					for(y = sw_cursor->lasty; y < sw_cursor->lasty + sw_cursor->lasth; y++){
						if(y >= 0 && y < wScrY){
							BYTE __far *dst = GetLinePtr(y);
							BYTE __far *src = cursor_backdata + ((y - sw_cursor->lasty)*sw_cursor->w)*3;
							for(x = sw_cursor->lastx; x < sw_cursor->lastx + sw_cursor->lastw; x++){
								if(x >= 0 && x < wScrX){
									dst[x*3  ] = *src;
									dst[x*3+1] = *(src+1);
									dst[x*3+2] = *(src+2);
								}
								src += 3;
							} } }
					break;
				}
				case 4:
				{
					for(y = sw_cursor->lasty; y < sw_cursor->lasty + sw_cursor->lasth; y++){
						if(y >= 0 && y < wScrY){
							DWORD __far *dst = GetLinePtr(y);
							DWORD __far *src = (DWORD __far *)cursor_backdata + ((y - sw_cursor->lasty)*sw_cursor->w);
							for(x = sw_cursor->lastx; x < sw_cursor->lastx + sw_cursor->lastw; x++){
								if(x >= 0 && x < wScrX){
									dst[x] = *src;
								}
								src++;
							} } }
					break;
				}
			} // switch bpp
			
			sw_cursor->state = STATE_ERASED;
			
			if(changes)
			{
				changes->left  = sw_cursor->lastx;
				changes->right = sw_cursor->lastx + sw_cursor->lastw;
				
				changes->top  = sw_cursor->lasty;
				changes->bottom = sw_cursor->lasty + sw_cursor->lasth;
			}
			
			sw_cursor->blitlock--;
		}
		else
		{
			if(changes)
			{
				changes->left = 0;
				changes->right = 0;
				changes->top = 0;
				changes->bottom = 0;
			}
		}
		cursor_cs_leave();
	}
}

void cursor_lock()
{
	if(sw_cursor)
	{
		cursor_cs_enter();
		sw_cursor->blitlock++;
		cursor_cs_leave();
	}
}

void cursor_unlock()
{
	if(sw_cursor)
	{
		cursor_cs_enter();
		sw_cursor->blitlock--;
		if(sw_cursor->blitlock < 0)
		{
			sw_cursor->blitlock = 0;
		}
		cursor_cs_leave();
	}
}

void cursor_blit(longRECT __far *changes)
{
	//dbg_printf("cursor_blit\n");
	
	if(sw_cursor)
	{
		long x, y;
		
		if(sw_cursor->ps != (wBpp + 7)/8)
		{
			return;
		}
		
		cursor_cs_enter();
		if(sw_cursor->state == STATE_ERASED && sw_cursor->blitlock == 0)
		{
			//dbg_printf("blit: state - %ld\n", sw_cursor->state);
			sw_cursor->blitlock++;
			
			switch(sw_cursor->ps)
			{
				case 1:
				{
					for(y = sw_cursor->offy; y < sw_cursor->offy + sw_cursor->h; y++){
						if(y >= 0 && y < wScrY){
							const long ptx = (y - sw_cursor->offy)*sw_cursor->w;
							BYTE __far *dst = GetLinePtr(y);
							BYTE __far *bd = cursor_backdata + ptx;
							BYTE __far *am = cursor_andmask  + ptx;
							BYTE __far *xm = cursor_xormask  + ptx;			
							for(x = sw_cursor->offx; x < sw_cursor->offx + sw_cursor->w; x++){
								if(x >= 0 && x < wScrX){
									*bd = dst[x];
									dst[x] &= *am;
									dst[x] ^= *xm;
								}
								bd++; am++; xm++;
							} } }
					break;
				}
				case 2:
				{
					for(y = sw_cursor->offy; y < sw_cursor->offy + sw_cursor->h; y++){
						if(y >= 0 && y < wScrY){
							const long ptx = (y - sw_cursor->offy)*sw_cursor->w;
							WORD __far *dst = GetLinePtr(y);
							WORD __far *bd = (WORD __far *)(cursor_backdata) + ptx;
							WORD __far *am = (WORD __far *)(cursor_andmask)  + ptx;
							WORD __far *xm = (WORD __far *)(cursor_xormask)  + ptx;
							for(x = sw_cursor->offx; x < sw_cursor->offx + sw_cursor->w; x++){
								if(x >= 0 && x < wScrX){
									*bd = dst[x];
									dst[x] &= *am;
									dst[x] ^= *xm;
								}
								bd++; am++; xm++;
							} } }
					break;
				}
				case 3:
				{
					for(y = sw_cursor->offy; y < sw_cursor->offy + sw_cursor->h; y++){
						if(y >= 0 && y < wScrY){
							const long ptx = (y - sw_cursor->offy)*sw_cursor->w*3;
							BYTE __far *dst = GetLinePtr(y);
							BYTE __far *bd = cursor_backdata + ptx;
							BYTE __far *am = cursor_andmask  + ptx;
							BYTE __far *xm = cursor_xormask  + ptx;
							for(x = sw_cursor->offx; x < sw_cursor->offx + sw_cursor->w; x++){
								if(x >= 0 && x < wScrX){
									*bd     = dst[x*3  ]; dst[x*3  ] &= *am;     dst[x*3  ] ^= *xm;
									*(bd+1) = dst[x*3+1]; dst[x*3+1] &= *(am+1); dst[x*3+1] ^= *(xm+1);
									*(bd+2) = dst[x*3+2]; dst[x*3+2] &= *(am+2); dst[x*3+2] ^= *(xm+2);							
								}
								bd += 3;
								am += 3;
								xm += 3;
							} } }
					break;
				}
				case 4:
				{
					for(y = sw_cursor->offy; y < sw_cursor->offy + sw_cursor->h; y++){
						if(y >= 0 && y < wScrY){
							const long ptx = (y - sw_cursor->offy)*sw_cursor->w;
							DWORD __far *dst = GetLinePtr(y);
							DWORD __far *bd = (DWORD __far *)(cursor_backdata) + ptx;
							DWORD __far *am = (DWORD __far *)(cursor_andmask)  + ptx;
							DWORD __far *xm = (DWORD __far *)(cursor_xormask)  + ptx;
							for(x = sw_cursor->offx; x < sw_cursor->offx + sw_cursor->w; x++){
								if(x >= 0 && x < wScrX){
									*bd = dst[x];
									dst[x] &= *am;
									dst[x] ^= *xm;
								}
								bd++; am++; xm++;
							} } }
					break;
				}
			} // switch bpp
			
			sw_cursor->state = STATE_BLITED;
			sw_cursor->lastx = sw_cursor->offx;
			sw_cursor->lasty = sw_cursor->offy;
			sw_cursor->lastw = sw_cursor->w;
			sw_cursor->lasth = sw_cursor->h;
			
			if(changes)
			{
				changes->left   = sw_cursor->offx;
				changes->right  = sw_cursor->offx + sw_cursor->w;
				
				changes->top    = sw_cursor->offy;
				changes->bottom = sw_cursor->offy + sw_cursor->h;
			}
			sw_cursor->blitlock--;
		} /* !empty */
		else
		{
			//dbg_printf("cursor_blit: state %ld, lock: %ld\n", sw_cursor->state, sw_cursor->blitlock);
			
			if(changes)
			{
				changes->left = 0;
				changes->right = 0;
				changes->top = 0;
				changes->bottom = 0;
			}
		}
		cursor_cs_leave();
	}
}

