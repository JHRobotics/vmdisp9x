/*****************************************************************************

Copyright (c) 2022  Michal Necasek
              2023  Philip Kelley

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

/* Display driver module initialization. */

#include "winhack.h"
#include <gdidefs.h>
#include <dibeng.h>
#include <minivdd.h>
#include "minidrv.h"
#include <configmg.h>

#include "pm16_calls.h"
#include "3d_accel.h"

/* GlobalSmartPageLock is a semi-undocumented function. Not officially
 * documented but described in KB Article Q180586. */
UINT WINAPI GlobalSmartPageLock( HGLOBAL hglb );

WORD    wScrX       = 640;  /* Current X resolution. */
WORD    wScrY       = 480;  /* Current Y resolution. */
WORD    wDpi        = 96;   /* Current DPI setting. */
WORD    wBpp        = 8;    /* Current BPP setting. */
WORD    wPalettized = 0;    /* Non-zero if palettized. */

WORD    OurVMHandle   = 0;  /* The current VM's ID. */
DWORD   VDDEntryPoint = 0;  /* The VDD entry point. */

/* On Entry:
 * EAX    = Function code (VDD_GET_DISPLAY_CONFIG)
 * EBX    = This VM's handle
 * ECX    = Size of DISPLAYINFO structure
 * EDX    = Zero to tell VDD to try virtualizing
 * ES:EDI = Pointer to DISPLAYINFO structure to be
 *          filled
 *
 * On Return:
 * EAX    = Return code (0 = succeess, -1 = failed)
 */
extern DWORD CallVDDGetDispConf( WORD Function, WORD wDInfSize, LPVOID pDInf );
#pragma aux CallVDDGetDispConf =    \
    ".386"                          \
    "movzx  eax, ax"                \
    "movzx  ecx, cx"                \
    "movzx  ebx, OurVMHandle"       \
    "movzx  edi, di"                \
    "call   dword ptr VDDEntryPoint"\
    "mov    edx, eax"               \
    "shr    edx, 16"                \
    parm [ax] [cx] [es di] modify [bx];
/* phkelley:               ^^^^^^^^^^^
 * This was a bug in the upstream driver where bx was being trashed.
 */

#pragma code_seg( _INIT )

/* Read the display settings from SYSTEM.INI or Registry.
 */
void ReadDisplayConfig( void )
{
    WORD        wX, wY;
    UINT        bIgnoreRegistry;
    MODEDESC    mode;

    /* Get the DPI, default to 96. */
    wDpi = GetPrivateProfileInt( "display", "dpi", 96, "system.ini" );

    /* Get X and Y resolution. */
    wX = GetPrivateProfileInt( "display", "x_resolution", 0, "system.ini" );
    wY = GetPrivateProfileInt( "display", "y_resolution", 0, "system.ini" );

    /* Get the bits per pixel. */
    wBpp = GetPrivateProfileInt( "display", "bpp", 0, "system.ini" );

    dbg_printf( "SYSTEM.INI: %ux%u %ubpp %udpi\n", wX, wY, wBpp, wDpi );


    bIgnoreRegistry = GetPrivateProfileInt( "display", "IgnoreRegistry", 0, "system.ini" );

    if( !bIgnoreRegistry ) {
        DISPLAYINFO DispInfo;
        DWORD       dwRc;

        dwRc = CallVDDGetDispConf( VDD_GET_DISPLAY_CONFIG, sizeof( DispInfo ), &DispInfo );
        if( (dwRc != VDD_GET_DISPLAY_CONFIG) && !dwRc ) {
            /* Call succeeded, use the data. */
            wScrX = DispInfo.diXRes;
            wScrY = DispInfo.diYRes;
            wBpp = DispInfo.diBpp;

            dbg_printf( "Registry: %ux%u %ubpp %udpi\n", DispInfo.diXRes, DispInfo.diYRes, DispInfo.diBpp, DispInfo.diDPI );

            /* DPI might not be set, careful. */
            if( DispInfo.diDPI )
                wDpi = DispInfo.diDPI;
        } else {
            dbg_printf( "VDD_GET_DISPLAY_CONFIG failed, dwRc=%lX\n",dwRc );
        }
    }

    mode.xRes = wScrX;
    mode.yRes = wScrY;
    mode.bpp  = wBpp;
    
    dbg_printf( "Entry: %ux%u\n", wScrX, wScrY);

    if( !FixModeInfo( &mode ) ) {
        /* Values were changed. */
        wScrX = mode.xRes;
        wScrY = mode.yRes;
        wBpp  = mode.bpp;
    }
    
    dbg_printf( "After fix: %ux%u %u bpp\n", wScrX, wScrY, wBpp);

    /* For 8bpp, read the 'palettized' setting. Default to enabled. */
    if( wBpp == 8 )
        wPalettized = GetPrivateProfileInt( "display", "palettized", 1, "system.ini" );
    else
        wPalettized = 0;
}

#define VDD_ID      10  /* Virtual Display Driver ID. */
#define CONFIGMG_ID 51  /* Configuration Manager Driver ID. */

/* Get Device API Entry Point. */
void __far *int_2F_GetEP( unsigned ax, unsigned bx );
#pragma aux int_2F_GetEP =  \
    "int    2Fh"            \
    parm [ax] [bx] value [es di];

/* Get "magic number" (current Virtual Machine ID) for VDD calls. */
WORD int_2F_GetVMID( unsigned ax );
#pragma aux int_2F_GetVMID =    \
    "int    2Fh"                \
    parm [ax] value [bx];

/* Dummy pointer to get at the _TEXT segment. Is there any easier way? */
extern char __based( __segname( "_TEXT" ) ) *pText;

/* This is a standard DLL entry point. Note that DS is already set
 * to point to this DLL's data segment on entry.
 */

#pragma aux DriverInit parm [cx] [di] [es si]

UINT FAR DriverInit( UINT cbHeap, UINT hModule, LPSTR lpCmdLine )
{
    /* Lock the code segment. */
    GlobalSmartPageLock( (__segment)pText );

    /* Query the entry point of the Virtual Display Device. */
    VDDEntryPoint = (DWORD)int_2F_GetEP( 0x1684, VDD_ID );

    /* Obtain the "magic number" needed for VDD calls. */
    OurVMHandle = int_2F_GetVMID( 0x1683 );

    dbg_printf( "DriverInit: VDDEntryPoint=%WP, OurVMHandle=%x\n", VDDEntryPoint, OurVMHandle );
    
    /* Read the display configuration before doing anything else. */
    ReadDisplayConfig();

		/* connect to 32bit RING-0 driver */
		if(!VXD_VM_connect())
		{
			dbg_printf("VXD connect failure!\n");
			return 0;
		}
		
		dbg_printf("VXD connect success!\n");
		FBHDA_setup(&hda, &hda_linear);
		
		mouse_buffer(&mouse_buf, &mouse_buf_lin);
		if(mouse_buf != NULL)
		{
			dbg_printf("VXD mouse ON\n");
			mouse_vxd = TRUE;
		}
		
		if(hda == NULL)
		{
			dbg_printf("DriverInit: failed to get FBHDA!\n");
			return 0;
		}

    return( 1 );    /* Success. */
}
