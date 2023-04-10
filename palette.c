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

/* Display driver palette (RAMDAC) functions. */

#include "winhack.h"
#include <gdidefs.h>
#include <dibeng.h>
#include "minidrv.h"
#include <conio.h>      /* For port I/O prototypes. */
#include "boxvint.h"    /* For VGA register definitions. */

#ifdef SVGA
#include "svga_all.h"

#pragma code_seg( _INIT )

static void __loadds __far SetColor(unsigned index, unsigned char r, unsigned char g, unsigned char b)
{
	UINT sIndex = SVGA_PALETTE_BASE + index*3;
	
  SVGA_WriteReg(sIndex+0, r);
  SVGA_WriteReg(sIndex+1, g);
  SVGA_WriteReg(sIndex+2, b);
}
#endif

#pragma code_seg( _TEXT )

/* Load the VGA DAC with values from color table. */
static void SetRAMDAC( UINT bStart, UINT bCount, RGBQUAD FAR *lpPal )
{
#ifdef SVGA
	  BYTE    bIndex = bStart;

    while( bCount-- ) {
       SetColor(bIndex++, lpPal[bIndex].rgbRed, lpPal[bIndex].rgbGreen, lpPal[bIndex].rgbBlue);
    }
#else
    BYTE    bIndex = bStart;

    /* The data format is too weird for BOXV_dac_set(). Do it by hand. */
    outp( VGA_DAC_W_INDEX, bIndex );    /* Set starting index. */
    while( bCount-- ) {
        outp( VGA_DAC_DATA, lpPal[bIndex].rgbRed );
        outp( VGA_DAC_DATA, lpPal[bIndex].rgbGreen );
        outp( VGA_DAC_DATA, lpPal[bIndex].rgbBlue );
        ++bIndex;
    }
#endif
}

/* Allow calls from the _INIT segment. */
void __far SetRAMDAC_far( UINT bStart, UINT bCount, RGBQUAD FAR *lpPal )
{
    SetRAMDAC( bStart, bCount, lpPal );
}

UINT WINAPI __loadds SetPalette( UINT wStartIndex, UINT wNumEntries, LPVOID lpPalette )
{
    /* Let the DIB engine do what it can. */
    DIB_SetPaletteExt( wStartIndex, wNumEntries, lpPalette, lpDriverPDevice );

    if( !(lpDriverPDevice->deFlags & BUSY) )
        SetRAMDAC( wStartIndex, wNumEntries, lpColorTable );

    return( 0 );
}
