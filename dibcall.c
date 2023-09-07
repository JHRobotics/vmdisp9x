/*****************************************************************************

Copyright (c) 2022  Michal Necasek

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

#include "minidrv.h"

#ifdef SVGA
# include "svga_all.h"
# include <string.h>
# include "vxdcall.h"

/* from control.c */
# define LOCK_FIFO 6
BOOL SVGAHDA_trylock(DWORD lockid);
void SVGAHDA_unlock(DWORD lockid);
#endif

/*
 * What's this all about? Most of the required exported driver functions can
 * be passed straight to the DIB Engine. The DIB Engine in some cases requires
 * an additional parameter.
 *
 * See the dibthunk.asm module for functions that can be handled easily. Note
 * that although the logic could be implemented in C, it can be done more
 * efficiently in assembly. Forwarders turn into simple jumps, and functions
 * with an extra parameter can be implemented with very small overhead, saving
 * stack space and extra copying.
 *
 * This module deals with the very few functions that need additional logic but
 * are not hardware specific.
 */

/* from DDK98 */
#pragma pack(push)
#pragma pack(1)
typedef struct
{
   int     xHotSpot, yHotSpot;
   int     cx, cy;
   int     cbWidth;
   BYTE    Planes;
   BYTE    BitsPixel;
} CURSORSHAPE;
#pragma pack(pop)

#ifdef SVGA
BOOL cursorVisible = FALSE;
#endif

#pragma code_seg( _TEXT )

#ifdef SVGA
static LONG cursorX = 0;
static LONG cursorY = 0;
static LONG cursorW = 0;
static LONG cursorH = 0;
static LONG cursorHX = 0;
static LONG cursorHY = 0;


/*
	https://learn.microsoft.com/en-us/windows-hardware/drivers/display/drawing-monochrome-pointers
*/
static void CursorMonoToAlpha(CURSORSHAPE __far *lpCursor, void __far *data)
{
	LONG __far *px = data;
	BYTE __far *andmask = (BYTE __far *)(lpCursor + 1);
	BYTE __far *xormask = andmask + lpCursor->cbWidth*lpCursor->cy;
	int xi, xj, y;
	
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

void update_cursor()
{
	if(wBpp == 32)
	{
		LONG x = cursorX - cursorHX;
		LONG y = cursorY - cursorHY;
		LONG w = cursorW;
		LONG h = cursorH;
		
		SVGA_UpdateRect(x, y, w, h);
	}
}
#endif

void WINAPI __loadds MoveCursor(WORD absX, WORD absY)
{
	if(wEnabled)
	{
#ifdef SVGA
		if(wBpp == 32)
		{
			if(gSVGA.userFlags & SVGA_USER_FLAGS_HWCURSOR)
			{
				if(SVGAHDA_trylock(LOCK_FIFO))
				{
					SVGA_MoveCursor(cursorVisible, absX, absY, 0);
					SVGAHDA_unlock(LOCK_FIFO);
				}
				
				/* save last position */
	  	  cursorX = absX;
	    	cursorY = absY;
			}
			else
			{
		    DIB_MoveCursorExt(absX, absY, lpDriverPDevice);
		    update_cursor();
	  	  cursorX = absX;
	    	cursorY = absY;
	    	update_cursor();
	    }
			return;
		}
#endif // SVGA
		DIB_MoveCursorExt(absX, absY, lpDriverPDevice);
	}
}

WORD WINAPI __loadds SetCursor_driver(CURSORSHAPE __far *lpCursor)
{
	if(wEnabled)
	{
#ifdef SVGA
		if(wBpp == 32)
		{
			if(gSVGA.userFlags & SVGA_USER_FLAGS_HWCURSOR)
			{
				void __far* ANDMask = NULL;
				void __far* XORMask = NULL;
				void __far* AData   = NULL;
				SVGAFifoCmdDefineCursor cur;
				SVGAFifoCmdDefineAlphaCursor acur;
				
				if(lpCursor != NULL)
				{
					dbg_printf("cx: %d, cy: %d, colors: %d\n", lpCursor->cx, lpCursor->cy, lpCursor->BitsPixel);
					
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
				
				return 1; /* SUCCESS */
			}
			else
			{
				if(lpCursor != NULL)
				{
					cursorW = lpCursor->cx;
					cursorH = lpCursor->cy;
					cursorHX = lpCursor->xHotSpot;
					cursorHY = lpCursor->yHotSpot;
				}
			}
		} // 32bpp
#endif
		DIB_SetCursorExt(lpCursor, lpDriverPDevice);
		return 1;
	}
	return 0;
}

/* Exported as DISPLAY.104 */
void WINAPI __loadds CheckCursor( void )
{
	if( wEnabled ) {
#ifdef SVGA
	if((gSVGA.userFlags & SVGA_USER_FLAGS_HWCURSOR) == 0)
	{
		DIB_CheckCursorExt( lpDriverPDevice );
		if(wBpp == 32)
		{
			update_cursor();
		}
	}
	else
	{
		if(wBpp != 32)
		{
			DIB_CheckCursorExt( lpDriverPDevice );
		}
	}
#else
		DIB_CheckCursorExt( lpDriverPDevice );
#endif
	}
}

/* If there is no hardware screen-to-screen BitBlt, there's no point in
 * this and we can just forward BitBlt to the DIB Engine.
 */
#ifdef HWBLT

extern BOOL WINAPI (* BitBltDevProc)( LPDIBENGINE, WORD, WORD, LPPDEVICE, WORD, WORD,
                                      WORD, WORD, DWORD, LPBRUSH, LPDRAWMODE );

/* See if a hardware BitBlt can be done. */
BOOL WINAPI __loadds BitBlt( LPDIBENGINE lpDestDev, WORD wDestX, WORD wDestY, LPPDEVICE lpSrcDev,
                    WORD wSrcX, WORD wSrcY, WORD wXext, WORD wYext, DWORD dwRop3,
                    LPBRUSH lpPBrush, LPDRAWMODE lpDrawMode )
{
    WORD    dstFlags = lpDestDev->deFlags;

    /* The destination must be video memory and not busy. */
    if( (dstFlags & VRAM) && !(dstFlags & BUSY) ) {
        /* If palette translation is needed, only proceed if source
         * and destination device are identical.
         */
        if( !(dstFlags & PALETTE_XLAT) || (lpDestDev == lpSrcDev) ) {
            /* If there is a hardware acceleration callback, use it. */
            if( BitBltDevProc ) {
                return( BitBltDevProc( lpDestDev, wDestX, wDestY, lpSrcDev, wSrcX, wSrcY, wXext, wYext, dwRop3, lpPBrush, lpDrawMode ) );
            }
        }
    }
    return( DIB_BitBlt( lpDestDev, wDestX, wDestY, lpSrcDev, wSrcX, wSrcY, wXext, wYext, dwRop3, lpPBrush, lpDrawMode ) );
}

#endif

#ifndef ETO_GLYPH_INDEX
#define ETO_GLYPH_INDEX 0x0010
#endif

DWORD WINAPI __loadds ExtTextOut( LPDIBENGINE lpDestDev, WORD wDestXOrg, WORD wDestYOrg, LPRECT lpClipRect,
                                        LPSTR lpString, int wCount, LPFONTINFO lpFontInfo, LPDRAWMODE lpDrawMode,
                                        LPTEXTXFORM lpTextXForm, LPSHORT lpCharWidths, LPRECT lpOpaqueRect, WORD wOptions )
{
/*	if(wOptions & ETO_GLYPH_INDEX)
	{
		return 0x80000000UL;
	}*/
	
	/*if(wCount > 0)
	{
		dbg_printf("ExtTextOut: ");
	 	for(i = 0; i < wCount; i++)
	 	{
	 		dbg_printf("%c", lpString[i]);
	 	}
 	  dbg_printf("(%X) %d\n", lpString[0], wCount);
  }*/
	
	return DIB_ExtTextOut(lpDestDev, wDestXOrg, wDestYOrg, lpClipRect, lpString, wCount, lpFontInfo, lpDrawMode, lpTextXForm, lpCharWidths, lpOpaqueRect, wOptions);
}
