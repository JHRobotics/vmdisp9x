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

#define SVGA

#include "winhack.h"
#include <gdidefs.h>
#include <dibeng.h>
#include <minivdd.h>
#include <string.h>

#include "minidrv.h"
#include "swcursor.h"
#include "hwcursor.h"

#include "svga_all.h"
#include "vxdcall.h"

/* from control.c */
#define LOCK_FIFO 6
BOOL SVGAHDA_trylock(DWORD lockid);
void SVGAHDA_unlock(DWORD lockid);

static BOOL cursorVisible = FALSE;

static LONG cursorW = 0;
static LONG cursorH = 0;
static LONG cursorHX = 0;
static LONG cursorHY = 0;

#pragma code_seg( _TEXT )

/*
	https://learn.microsoft.com/en-us/windows-hardware/drivers/display/drawing-monochrome-pointers
*/
static void CursorMonoToAlpha(CURSORSHAPE __far *lpCursor, void __far *data)
{
	LONG __far *px = data;
	BYTE __far *andmask = (BYTE __far *)(lpCursor + 1);
	BYTE __far *xormask = andmask + lpCursor->cbWidth*lpCursor->cy;
	int xi, xj, y;
	
	dbg_printf("CursorMonoToAlpha\n");
	
	for(y = 0; y < lpCursor->cy; y++)
	{
		for(xj = 0; xj < (lpCursor->cbWidth); xj++)
		{
			BYTE a = andmask[lpCursor->cbWidth*y + xj];
			BYTE x = xormask[lpCursor->cbWidth*y + xj];
			for(xi = 7; xi >= 0; xi--)
			{
				BYTE mx = (((a >> xi) & 0x1) << 1) | ((x >> xi) & 0x1);
				
				switch(mx)
				{
					case 0:
						*px = 0xFF000000; // full black
						break;
					case 1:
						*px = 0xFFFFFFFF; // full white
						break;
					case 2:
						*px = 0x00000000; // transparent
						break;
					case 3:
						*px = 0xFF000000; // inverse = set black (used for example with type cursor)
						break;
				}
				
				px++;
			}
		}
	}
}

static void CursorColor32ToAlpha(CURSORSHAPE __far *lpCursor, void __far *data)
{
	LONG __far *px = data;
	BYTE __far *andmask = (BYTE __far *)(lpCursor + 1);
	DWORD __far *xormask = (DWORD __far *)(andmask + lpCursor->cbWidth*lpCursor->cy);
	int xi, xj, y;
	
	dbg_printf("CursorColor32ToAlpha\n");
	
	for(y = 0; y < lpCursor->cy; y++)
	{
		for(xj = 0; xj < (lpCursor->cbWidth); xj++)
		{
			BYTE a = andmask[lpCursor->cbWidth*y + xj];
			
			for(xi = 7; xi >= 0; xi--)
			{
				DWORD x = xormask[lpCursor->cx*y + xj*8 + (7-xi)] & 0x00FFFFFFUL;
				BYTE ab = ((a >> xi) & 0x1);
				
				switch(ab)
				{
					case 0:
						*px = 0xFF000000UL | x;
						break;
					case 1:
						*px = 0x00000000UL | x;
						break;
				}
				
				px++;
			}
		}
	}
}

void SVGA_UpdateLongRect(longRECT __far *rect)
{
	if(wBpp == 32)
	{
		long w = rect->right - rect->left;
		long h = rect->bottom - rect->top;
		if(w > 0)
		{
			SVGA_UpdateRect(rect->left, rect->top, w, h);
		}
	}
}

BOOL hwcursor_move(WORD absX, WORD absY)
{
	if(wBpp != 32)
	{
		return FALSE;
	}
	
	if((gSVGA.userFlags & SVGA_USER_FLAGS_HWCURSOR) == 0)
	{
		return FALSE;
	}

	if(SVGAHDA_trylock(LOCK_FIFO))
	{
		SVGA_MoveCursor(cursorVisible, absX, absY, 0);
		SVGAHDA_unlock(LOCK_FIFO);
	}
	
	cursorX = absX;
	cursorY = absY;
				
	return TRUE;
}

BOOL hwcursor_load(CURSORSHAPE __far *lpCursor)
{
	void __far* ANDMask = NULL;
	void __far* XORMask = NULL;
	void __far* AData   = NULL;
	SVGAFifoCmdDefineCursor cur;
	SVGAFifoCmdDefineAlphaCursor acur;
	
	if(wBpp != 32)
	{
		return FALSE;
	}
	
	if((gSVGA.userFlags & SVGA_USER_FLAGS_HWCURSOR) == 0)
	{
		return FALSE;
	}
				
	if(lpCursor != NULL)
	{
		dbg_printf("cx: %d, cy: %d, colors: %d\n", lpCursor->cx, lpCursor->cy, lpCursor->BitsPixel);
		
		dbg_printf("flags %lx\n", gSVGA.userFlags);
		
		if(gSVGA.userFlags & SVGA_USER_FLAGS_ALPHA_CUR)
		{
			acur.id = 0;
			acur.hotspotX = lpCursor->xHotSpot;
			acur.hotspotY = lpCursor->yHotSpot;
			acur.width    = lpCursor->cx;
			acur.height   = lpCursor->cy;
						
			if(SVGAHDA_trylock(LOCK_FIFO))
			{
				SVGA_BeginDefineAlphaCursor(&acur, &AData);
				if(AData)
				{
					switch(lpCursor->BitsPixel)
					{
						case 1:
							CursorMonoToAlpha(lpCursor, AData);
							break;
						case 32:
							CursorColor32ToAlpha(lpCursor, AData);
							break;
					}
				}
				VXD_FIFOCommitAll();
				SVGAHDA_unlock(LOCK_FIFO);
			}
		}
		else
		{
			cur.id = 0;
			cur.hotspotX = lpCursor->xHotSpot;
			cur.hotspotY = lpCursor->yHotSpot;
			cur.width    = lpCursor->cx;
			cur.height   = lpCursor->cy;
			cur.andMaskDepth = 1;
			cur.xorMaskDepth = lpCursor->BitsPixel;
			
			if(SVGAHDA_trylock(LOCK_FIFO))
			{
				SVGA_BeginDefineCursor(&cur, &ANDMask, &XORMask);
				
				if(ANDMask)
				{
					_fmemcpy(ANDMask, lpCursor+1, lpCursor->cbWidth*lpCursor->cy);
				}
				
				if(XORMask)
				{
					BYTE __far *ptr = (BYTE __far *)(lpCursor+1);
					ptr += lpCursor->cbWidth*lpCursor->cy;
					
					_fmemcpy(XORMask, ptr, lpCursor->cx*lpCursor->cy*((lpCursor->BitsPixel+7)/8));
				}
							
				VXD_FIFOCommitAll();
				SVGAHDA_unlock(LOCK_FIFO);
			}
		}
		
		/* move cursor to last known position */
		if(SVGAHDA_trylock(LOCK_FIFO))
		{
			SVGA_MoveCursor(cursorVisible, cursorX, cursorY, 0);
			SVGAHDA_unlock(LOCK_FIFO);
		}
					
		cursorVisible = TRUE;
		
		cursorW = lpCursor->cx;
		cursorH = lpCursor->cy;
		cursorHX = lpCursor->xHotSpot;
		cursorHY = lpCursor->yHotSpot;
	}
	else
	{
		/* virtual box bug on SVGA_MoveCursor(FALSE, ...)
		 * so if is cursor NULL, create empty
		 */
		cur.id = 0;
		cur.hotspotX = 0;
		cur.hotspotY = 0;
		cur.width    = 32;
		cur.height   = 32;
		cur.andMaskDepth = 1;
		cur.xorMaskDepth = 1;
		
		if(SVGAHDA_trylock(LOCK_FIFO))
		{
			SVGA_BeginDefineCursor(&cur, &ANDMask, &XORMask);
			
			if(ANDMask) _fmemset(ANDMask, 0xFF, 4*32);
			if(XORMask) _fmemset(XORMask, 0, 4*32);
					
			//SVGA_FIFOCommitAll();
			VXD_FIFOCommitAll();
			
			SVGA_MoveCursor(FALSE, 0, 0, 0);
						
			SVGAHDA_unlock(LOCK_FIFO);
		}
		cursorVisible = FALSE;
	}
	
	dbg_printf("hwcursor_load\n");
				
	return TRUE;
}

void hwcursor_update()
{
	if(wBpp == 32)
	{
		if((gSVGA.userFlags & SVGA_USER_FLAGS_HWCURSOR) != 0)
		{
			LONG x = cursorX - cursorHX;
			LONG y = cursorY - cursorHY;
			LONG w = cursorW;
			LONG h = cursorH;
			
			SVGA_UpdateRect(x, y, w, h);
		}
	}
}

BOOL hwcursor_available()
{
	if(wBpp != 32)
	{
		return FALSE;
	}
	
	if((gSVGA.userFlags & SVGA_USER_FLAGS_HWCURSOR) == 0)
	{
		return FALSE;
	}
	
	return TRUE;
}
