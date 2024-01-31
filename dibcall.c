/*****************************************************************************

Copyright (c) 2022  Michal Necasek
              2023  Jaroslav Hensl

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
#include "cursor.h"

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

#pragma code_seg( _TEXT )

void WINAPI __loadds MoveCursor(WORD absX, WORD absY)
{	
	if(wEnabled)
	{
		DIB_MoveCursorExt(absX, absY, lpDriverPDevice);
	}
}

WORD WINAPI __loadds SetCursor_driver(CURSORSHAPE __far *lpCursor)
{
	if(wEnabled)
	{
		DIB_SetCursorExt(lpCursor, lpDriverPDevice);
	}
	
	return 0;
}

/* Exported as DISPLAY.104 */
void WINAPI __loadds CheckCursor( void )
{
	if(wEnabled)
	{
		DIB_CheckCursorExt(lpDriverPDevice);
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
