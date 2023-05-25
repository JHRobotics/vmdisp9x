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

/* GDI Enable (with ReEnable) and Disable implementation. */

#include "winhack.h"
#include <gdidefs.h>
#include <dibeng.h>
#include <minivdd.h>
#include "minidrv.h"

#include <string.h>

/* Pretend we have a 208 by 156 mm screen. */
#define DISPLAY_HORZ_MM     208
#define DISPLAY_VERT_MM     156

/* Size in English units. Looks pretty random. */
#define DISPLAY_SIZE_EN     325
/* Size in twips. Just as random. */
#define DISPLAY_SIZE_TWP    2340

LPDIBENGINE lpDriverPDevice = 0;    /* This device's PDEV that's passed to GDI. */
WORD wEnabled = 0;                  /* Is this device enabled? */
RGBQUAD FAR *lpColorTable = 0;      /* Current color table. */

static BYTE bReEnabling = 0;        /* Set when re-enabling PDEV. */
static WORD wDIBPdevSize = 0;


/* 1bpp Color Table (non-palettized). */
const DIBColorEntry DIB1ColorTable[] = {
    /* blue  red  green flags */
    { 0   , 0   , 0   , 0          },
    { 0xFF, 0xFF, 0xFF, MAPTOWHITE }
};

/* 4bpp Color Table (non-palettized). */
const DIBColorEntry DIB4ColorTable[] = {
    /* blue  red  green flags */
    { 0   , 0   , 0   , 0          },
    { 0   , 0   , 0x80, 0          },
    { 0   , 0x80, 0   , 0          },
    { 0   , 0x80, 0x80, 0          },
    { 0x80, 0   , 0   , 0          },
    { 0x80, 0   , 0x80, 0          },
    { 0x80, 0x80, 0   , 0          },
    { 0xC0, 0xC0, 0xC0, MAPTOWHITE },
    { 0x80, 0x80, 0x80, MAPTOWHITE },
    { 0   , 0   , 0xFF, 0          },
    { 0   , 0xFF, 0   , MAPTOWHITE },
    { 0   , 0xFF, 0xFF, MAPTOWHITE },
    { 0xFF, 0   , 0   , 0          },
    { 0xFF, 0   , 0xFF, 0          },
    { 0xFF, 0xFF, 0   , MAPTOWHITE },
    { 0xFF, 0xFF, 0xFF, MAPTOWHITE }
};

/* 8bpp Color Table (palettized), first 10 entries. */
const DIBColorEntry DIB8ColorTable1[] = {
    /* blue  red  green flags */
    { 0   , 0   , 0   , 0          },
    { 0   , 0   , 0x80, 0          },
    { 0   , 0x80, 0   , 0          },
    { 0   , 0x80, 0x80, 0          },
    { 0x80, 0   , 0   , 0          },
    { 0x80, 0   , 0x80, 0          },
    { 0x80, 0x80, 0   , 0          },
    { 0xC0, 0xC0, 0xC0, MAPTOWHITE },
    { 0xC0, 0xDC, 0xC0, MAPTOWHITE | NONSTATIC },
    { 0xF0, 0xCA, 0xA6, MAPTOWHITE | NONSTATIC }
};

/* 8bpp Color Table (palettized), all middle entries (10-245). */
const DIBColorEntry DIB8ColorTable2[] = {
    /* blue  red  green flags */
    { 0, 0, 0, NONSTATIC }
};

/* 8bpp Color Table (palettized), last 10 entries (246-255). */
const DIBColorEntry DIB8ColorTable3[] = {
    /* blue  red  green flags */
    { 0xF0, 0xFB, 0xFF, MAPTOWHITE | NONSTATIC },
    { 0xA4, 0xA0, 0xA0, MAPTOWHITE | NONSTATIC },
    { 0x80, 0x80, 0x80, MAPTOWHITE },
    { 0   , 0   , 0xFF, 0          },
    { 0   , 0xFF, 0   , MAPTOWHITE },
    { 0   , 0xFF, 0xFF, MAPTOWHITE },
    { 0xFF, 0   , 0   , 0          },
    { 0xFF, 0   , 0xFF, 0          },
    { 0xFF, 0xFF, 0   , MAPTOWHITE },
    { 0xFF, 0xFF, 0xFF, MAPTOWHITE }
};

/* An undocumented function called by USER to check if display driver
 * contains a more suitable version of a resource.
 * Exported as ordinal 450.
 */
DWORD WINAPI __loadds GetDriverResourceID( WORD wResID, LPSTR lpResType )
{
    if( wResID == OBJ_FONT ) {
        if( wDpi != 96 ) {
            return( 2003 ); /* If DPI is not 96, return fonts120.bin instead. */
        }
    }
    return( wResID );
}

/* Some genius at Microsoft decided that CreateDIBPDevice returns the result
 * in EAX, not DX:AX. This is not at all documented in the Win95 DDK, but it
 * is stated clearly in U.S. Patent 6,525,743 (granted in 2003).
 * We just create a tiny thunk to produce a sane calling convention.
 */
DWORD PASCAL CreateDIBPDeviceX( LPBITMAPINFO lpInfo, LPPDEVICE lpDevice, LPVOID lpBits, WORD wFlags );
#pragma aux CreateDIBPDeviceX = \
    "call   CreateDIBPDevice"   \
    "mov    dx, ax"             \
    "shr    eax, 16"            \
    "xchg   ax, dx"

#pragma code_seg( _INIT )

/* GDI calls Enable twice at startup, first to query the GDIINFO structure
 * and then to initialize the video hardware.
 */
UINT WINAPI __loadds Enable( LPVOID lpDevice, UINT style, LPSTR lpDeviceType,
                             LPSTR lpOutputFile, LPVOID lpStuff )
{
    WORD    rc;
    WORD    wPalCnt;

    dbg_printf( "Enable: lpDevice=%WP style=%X bReEnabling=%u wPalettized=%u\n", lpDevice, style, bReEnabling, wPalettized );
    if( !(style & 1) ) {    /* Only test the low bit! */
        LPDIBENGINE     lpEng = lpDevice;
        LPBITMAPINFO    lpInfo;
        WORD            wFlags;
        DWORD           dwRet;

        /* Initialize the PDEVICE. */
        lpDriverPDevice = lpDevice;
        rc = PhysicalEnable();
        if( !rc ) {
            dbg_printf( "Enable: PhysicalEnable failed!\n" );
            return( 0 );
        }
        if( !bReEnabling ) {
            int_2Fh( STOP_IO_TRAP );
        }

        /* Pass down to the DIB engine. */
        DIB_Enable( lpDevice, style, lpDeviceType, lpOutputFile, lpStuff );

        if( wPalettized )
            DIB_SetPaletteTranslateExt( NULL, lpDriverPDevice );

        dbg_printf( "Enable: wBpp=%u wDpi=%u wScreenX=%u wScreenY=%u wDIBPdevSize=%x\n",
                    wBpp, wDpi, wScreenX, wScreenY, wDIBPdevSize );

        /* Fill out the bitmap header. */
        /// @todo Does wDIBPdevSize have to equal sizeof(DIBENGINE)?
        lpInfo = (LPVOID)((LPBYTE)lpDevice + wDIBPdevSize);
        _fmemset( &lpInfo->bmiHeader, 0, sizeof( lpInfo->bmiHeader ) );
        lpInfo->bmiHeader.biSize     = sizeof( lpInfo->bmiHeader );
        lpInfo->bmiHeader.biWidth    = wScreenX;
        lpInfo->bmiHeader.biHeight   = wScreenY;
        lpInfo->bmiHeader.biPlanes   = 1;
        lpInfo->bmiHeader.biBitCount = wBpp;

        /* Set up the color table for non-direct color modes. */
        if( wBpp <= 8 ) {
            DIBColorEntry FAR   *lpDefaultClr;

            switch( wBpp ) {
            case 8:
                lpDefaultClr = DIB8ColorTable1;
                wPalCnt = sizeof( DIB8ColorTable1 );
                break;
            case 4:
                lpDefaultClr = DIB4ColorTable;
                wPalCnt = sizeof( DIB4ColorTable );
                break;
            case 1:
            default:
                lpDefaultClr = DIB1ColorTable;
                wPalCnt = sizeof( DIB1ColorTable );
                break;
            }
            lpColorTable = &lpInfo->bmiColors;

            if( !bReEnabling ) {
                _fmemcpy( lpColorTable, lpDefaultClr, wPalCnt );
                /* For 8bpp, fix up the rest of the palette. */
                if( wBpp == 8 ) {
                    int     i;

                    /* The entries at index 10 to 245 are all the same. */
                    for( i = 10; i < 246; ++i )
                        lpColorTable[i] = DIB8ColorTable2[0];

                    _fmemcpy( &lpColorTable[246], DIB8ColorTable3, sizeof( DIB8ColorTable3 ) );
                }
            }
        }

        wFlags = wPDeviceFlags;
        if( wPalettized )
            wFlags |= PALETTIZED;

        /* Call the DIB Engine to set up the PDevice. */
        dbg_printf( "lpInfo=%WP lpDevice=%WP lpColorTable=%WP wFlags=%X ScreenSelector=%X\n", lpInfo, lpDevice, lpColorTable, wFlags, ScreenSelector );
        dwRet = CreateDIBPDeviceX( lpInfo, lpDevice, ScreenSelector :> 0, wFlags );
        if( !dwRet ) {
            dbg_printf( "Enable: CreateDIBPDevice failed!\n" );
            return( 0 );
        }
        dbg_printf( "Enable: CreateDIBPDevice returned %lX\n", dwRet );

        /* Now fill out the begin/end access callbacks. */
        lpEng->deBeginAccess = DIB_BeginAccess;
        lpEng->deEndAccess   = DIB_EndAccess;

        /* Program the DAC in non-direct color modes. */
        if( wBpp <= 8 ) {
            switch( wBpp ) {
            case 8:
                wPalCnt = 256;
                break;
            case 4:
                wPalCnt = 16;
                break;
            case 1:
            default:
                wPalCnt = 2;
                break;
            }
            SetRAMDAC_far( 0, wPalCnt, lpColorTable );
        }

        if( !bReEnabling ) {
            HookInt2Fh();
        }
        DDCreateDriverObject(1);
        
        wEnabled = 1;
        
        return( 1 );
    } else {
        /* Fill out GDIINFO for GDI. */
        LPGDIINFO   lpInfo = lpDevice;

        /* Start with passing down to the DIB engine. It will set dpCurves through dpStyleLen. */
        DIB_Enable( lpDevice, style, lpDeviceType, lpOutputFile, lpStuff );

        /* Fill out some static data. Note that some fields are set by the DIB Engine
         * and we don't touch them (curves, lines, polygons etc.).
         */
        lpInfo->dpVersion    = DRV_VERSION;
        lpInfo->dpTechnology = DT_RASDISPLAY;
        lpInfo->dpHorzSize   = DISPLAY_HORZ_MM;
        lpInfo->dpVertSize   = DISPLAY_VERT_MM;
        lpInfo->dpPlanes     = 1;
        lpInfo->dpCapsFE     = 0;
        lpInfo->dpNumFonts   = 0;

        /* Now set the fields that depend on current mode. */
        lpInfo->dpHorzRes = wScrX;
        lpInfo->dpVertRes = wScrY;

        lpInfo->dpMLoWin.xcoord = DISPLAY_HORZ_MM * 10;
        lpInfo->dpMLoWin.ycoord = DISPLAY_VERT_MM * 10;
        lpInfo->dpMLoVpt.xcoord = wScrX;
        lpInfo->dpMLoVpt.ycoord = -wScrY;

        lpInfo->dpMHiWin.xcoord = DISPLAY_HORZ_MM * 100;
        lpInfo->dpMHiWin.ycoord = DISPLAY_VERT_MM * 100;
        lpInfo->dpMHiVpt.xcoord = wScrX;
        lpInfo->dpMHiVpt.ycoord = -wScrY;

        /* These calculations are a wild guess and probably don't matter. */
        lpInfo->dpELoWin.xcoord = DISPLAY_SIZE_EN;
        lpInfo->dpELoWin.ycoord = DISPLAY_SIZE_EN;
        lpInfo->dpELoVpt.xcoord = wScrX / 5;
        lpInfo->dpELoVpt.ycoord = -lpInfo->dpELoVpt.xcoord;

        lpInfo->dpELoWin.xcoord = DISPLAY_SIZE_EN * 5;
        lpInfo->dpELoWin.ycoord = DISPLAY_SIZE_EN * 5;
        lpInfo->dpEHiVpt.xcoord = wScrX / 10;
        lpInfo->dpEHiVpt.ycoord = -lpInfo->dpEHiVpt.xcoord;

        lpInfo->dpTwpWin.xcoord = DISPLAY_SIZE_TWP;
        lpInfo->dpTwpWin.ycoord = DISPLAY_SIZE_TWP;
        lpInfo->dpTwpVpt.xcoord = wScrX / 10;
        lpInfo->dpTwpVpt.ycoord = -lpInfo->dpTwpVpt.xcoord;

        /* Update more GDIINFO bits. */
        lpInfo->dpLogPixelsX = wDpi;
        lpInfo->dpLogPixelsY = wDpi;
        lpInfo->dpBitsPixel  = wBpp;
        lpInfo->dpDCManage   = DC_IgnoreDFNP;
        /* In theory we should set the C1_SLOW_CARD flag since this driver is unaccelerated.
         * This flag disables certain visual effects like "embossed" disabled text or animations.
         * Realistically, software rendering in a VM on a modern system is going to be a lot
         * faster than most mid-1990s graphics cards.
         */
        lpInfo->dpCaps1 |= C1_COLORCURSOR | C1_REINIT_ABLE | C1_GLYPH_INDEX | C1_BYTE_PACKED; /* | C1_SLOW_CARD */;
        //lpInfo->dpCaps1 &= ~C1_GLYPH_INDEX;
        
        dbg_printf( "lpInfo->dpCaps1: %lX\n", (DWORD)lpInfo->dpCaps1);

        /* Grab the DIB Engine PDevice size before we add to it. */
        wDIBPdevSize = lpInfo->dpDEVICEsize;
        dbg_printf( "Enable: wDIBPdevSize=%X wScrX=%u wScrY=%u wBpp=%u wDpi=%u wScreenX=%u wScreenY=%u\n",
                    wDIBPdevSize, wScrX, wScrY, wBpp, wDpi, wScreenX, wScreenY );

        lpInfo->dpNumBrushes = -1;  /* Too many to count, always the same.. */

        if( wBpp == 8 ) {
            if( wPalettized ) {
                lpInfo->dpNumPens     = 16;     /* Pens realized by driver. */
                lpInfo->dpNumColors   = 20;     /* Colors in color table. */
                lpInfo->dpNumPalReg   = 256;
                lpInfo->dpPalReserved = 20;
                lpInfo->dpColorRes    = 18;
                lpInfo->dpRaster     |= RC_DIBTODEV + RC_PALETTE + RC_SAVEBITMAP;
            } else {
                lpInfo->dpNumPens     = 256;    /* Pens realized by driver. */
                lpInfo->dpNumColors   = 256;    /* Colors in color table. */
                lpInfo->dpNumPalReg   = 0;
                lpInfo->dpPalReserved = 0;
                lpInfo->dpColorRes    = 0;
                lpInfo->dpRaster     |= RC_DIBTODEV;
            }
            lpInfo->dpDEVICEsize += sizeof( BITMAPINFOHEADER ) + 256 * 4;
        } else if( wBpp > 8 ) {
            lpInfo->dpNumPens     = -1;     /* Pens realized by driver. */
            lpInfo->dpNumColors   = -1;     /* Colors in color table. */
            lpInfo->dpNumPalReg   = 0;
            lpInfo->dpPalReserved = 0;
            lpInfo->dpColorRes    = 0;
            lpInfo->dpRaster     |= RC_DIBTODEV;
            lpInfo->dpDEVICEsize += sizeof( BITMAPINFOHEADER );
        } else if( wBpp < 8 ) {
            WORD    wCount;

            wCount = 1 << wBpp; /* 2 or 4 for 2 or 16 bpp. */
            lpInfo->dpNumPens     = wCount; /* Pens realized by driver. */
            lpInfo->dpNumColors   = wCount; /* Colors in color table. */
            lpInfo->dpNumPalReg   = 0;
            lpInfo->dpPalReserved = 0;
            lpInfo->dpColorRes    = 0;
            lpInfo->dpRaster     |= RC_DIBTODEV;
            wCount *= 4;
            lpInfo->dpDEVICEsize += sizeof( BITMAPINFOHEADER ) + 8 + wCount;
        }
        
        dbg_printf( "sizeof(GDIINFO)=%d (%X), dpDEVICEsize=%X\n", sizeof( GDIINFO ), sizeof( GDIINFO ), lpInfo->dpDEVICEsize );
        return( sizeof( GDIINFO ) );
    }
}


/* The ReEnable function is called to dynamically change resolution.
 * It must query the new display mode settings and then call Enable.
 * NB: Windows 9x will not dynamically change the color depth, only
 * resolution. Documented in MS KB Article Q127139.
 */
UINT WINAPI __loadds ReEnable( LPVOID lpDevice, LPGDIINFO lpInfo )
{
    WORD    wLastValidBpp  = wBpp;
    WORD    wLastValidX    = wScreenX;
    WORD    wLastValidY    = wScreenY;
    WORD    rc;

    dbg_printf( "ReEnable: lpDevice=%WP lpInfo=%WP wScreenX=%u wScreenY=%u\n", lpDevice, lpInfo, wScreenX, wScreenY );

    /* Figure out the new mode. */
    ReadDisplayConfig();
    dbg_printf( "ReEnable: wScreenX=%u wScreenY=%u wBpp=%u\n", wScreenX, wScreenY, wBpp );

    /* Let Enable know it doesn't need to do everything. */
    bReEnabling = 1;

    /* Don't let the cursor mess with things. */
    DIB_BeginAccess( lpDevice, 0, 0, wScreenX - 1, wScreenY - 1, CURSOREXCLUDE );

    /* Create a new PDevice and set the new mode. Returns zero on failure. */
    rc = Enable( lpDevice, 0, NULL, NULL, NULL );

    /* Drawing the cursor is safe again. */
    DIB_EndAccess( lpDevice, CURSOREXCLUDE );

    if( rc ) {
        /* Enable succeeded, fill out GDIINFO. */
        Enable( lpInfo, 1, NULL, NULL, NULL );
        rc = 1;
    } else {
        dbg_printf( "ReEnable: Enable failed!\n" );
        /* Couldn't set new mode. Try to get the old one back. */
        wScreenX = wLastValidX;
        wScreenY = wLastValidY;
        wBpp     = wLastValidBpp;

        Enable( lpDevice, 0, NULL, NULL, NULL );

        /* And force a repaint. */
        RepaintFunc();
        rc = 0;
    }

    bReEnabling = 0;
    return( rc );
}

void int_10h( unsigned ax );
#pragma aux int_10h =   \
    "int    10h"        \
    parm [ax];

/* Disable graphics and go back to a text mode. */
UINT WINAPI __loadds Disable( LPVOID lpDevice )
{
    LPDIBENGINE lpEng = lpDevice;

    dbg_printf( "Disable: lpDevice=%WP\n", lpDevice );

    /* Start disabling and mark the PDevice busy. */
    wEnabled = 0;
    lpEng->deFlags |= BUSY; /// @todo Does this need to be a locked op?

    /* Re-enable I/O trapping before we start setting a standard VGA mode. */
    int_2Fh( START_IO_TRAP );
    
    /* Disable device if needed */
    PhysicalDisable();

    /* Tell VDD we're going away. */
    CallVDD( VDD_DRIVER_UNREGISTER );

    /* Set standard 80x25 text mode using the BIOS. */
    int_10h( 3 );

    /* And unhook INT 2F. */
    UnhookInt2Fh();

    return( 1 );
}
