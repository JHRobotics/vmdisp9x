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

#ifdef SVGA
#include <stdint.h>
#endif

#if defined(SVGA) || defined(QEMU)
# include "vxdcall.h"
#endif

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
#ifdef QEMU
DWORD   ConfigMGEntryPoint = 0;     /* The configuration manager entry point. */
DWORD   LfbBase = 0;                /* The physical base address of the linear framebuffer. */
#endif

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
#ifdef QEMU
/* Get the first logical configuration for a devnode.
 */
CONFIGRET static _near _cdecl CM_Get_First_Log_Conf(
    PLOG_CONF plcLogConf,
    DEVNODE dnDevNode,
    ULONG ulFlags)
{
    WORD wRc = 0;

    _asm { mov eax, 01Ah }; /* CM_GET_FIRST_LOG_CONF */
    _asm { call ConfigMGEntryPoint };
    _asm { mov wRc, ax };

    return( wRc );
}

/* Get the next resource descriptor of a given type.
 */
CONFIGRET static _near _cdecl CM_Get_Next_Res_Des(
    PRES_DES prdResDes,
    RES_DES CurrentResDesOrLogConf,
    RESOURCEID ForResource,
    PRESOURCEID pResourceID,
    ULONG ulFlags)
{
    WORD wRc = 0;

    _asm { mov eax, 01Fh }; /* CM_GET_NEXT_RES_DES */
    _asm { call ConfigMGEntryPoint };
    _asm { mov wRc, ax };

    return( wRc );
}

/* Get the data for a resource descriptor.
*/
CONFIGRET static _near _cdecl CM_Get_Res_Des_Data(
    RES_DES rdResDes,
    PFARVOID Buffer,
    ULONG BufferLen,
    ULONG ulFlags)
{
    WORD wRc = 0;

    _asm { mov eax, 022h }; /* CM_GET_RES_DES_DATA */
    _asm { call ConfigMGEntryPoint };
    _asm { mov wRc, ax };

    return( wRc );
}
#endif

/* Read the display settings from SYSTEM.INI or Registry.
 */
#ifdef QEMU
DEVNODE ReadDisplayConfig( void )
#else
void ReadDisplayConfig( void )
#endif
{
    WORD        wX, wY;
    UINT        bIgnoreRegistry;
    MODEDESC    mode;
#ifdef QEMU
    DEVNODE     devNode;
    DISPLAYINFO DispInfo;
    DWORD       dwRc;
#endif

    /* Get the DPI, default to 96. */
    wDpi = GetPrivateProfileInt( "display", "dpi", 96, "system.ini" );

    /* Get X and Y resolution. */
    wX = GetPrivateProfileInt( "display", "x_resolution", 0, "system.ini" );
    wY = GetPrivateProfileInt( "display", "y_resolution", 0, "system.ini" );

    /* Get the bits per pixel. */
    wBpp = GetPrivateProfileInt( "display", "bpp", 0, "system.ini" );

    dbg_printf( "SYSTEM.INI: %ux%u %ubpp %udpi\n", wX, wY, wBpp, wDpi );


    bIgnoreRegistry = GetPrivateProfileInt( "display", "IgnoreRegistry", 0, "system.ini" );

#ifdef QEMU
    dwRc = CallVDDGetDispConf( VDD_GET_DISPLAY_CONFIG, sizeof( DispInfo ), &DispInfo );
    if( (dwRc != VDD_GET_DISPLAY_CONFIG) && !dwRc ) {
        devNode = (DEVNODE)DispInfo.diDevNodeHandle;
 
        /* Call succeeded, use the data. */
        if (!bIgnoreRegistry)
        {
             wScrX = DispInfo.diXRes;
             wScrY = DispInfo.diYRes;
             wBpp = DispInfo.diBpp;

             dbg_printf( "Registry: %ux%u %ubpp %udpi\n", DispInfo.diXRes, DispInfo.diYRes, DispInfo.diBpp, DispInfo.diDPI );

             /* DPI might not be set, careful. */
             if( DispInfo.diDPI )
                 wDpi = DispInfo.diDPI;
         }
    } else {
        dbg_printf( "VDD_GET_DISPLAY_CONFIG failed, dwRc=%lX\n",dwRc );
        devNode = 0;
   }
#else
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
#endif

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

#ifdef QEMU
    return( devNode );
#endif
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

#ifdef QEMU
UINT FAR DriverInit( UINT cbHeap, UINT hModule, LPSTR lpCmdLine )
{
    DEVNODE devNode;

     /* Lock the code segment. */
     GlobalSmartPageLock( (__segment)pText );
 
    /* Query the entry point of the Virtual Display Device. */
    VDDEntryPoint = (DWORD)int_2F_GetEP( 0x1684, VDD_ID );

    /* Obtain the "magic number" needed for VDD calls. */
    OurVMHandle = int_2F_GetVMID( 0x1683 );
    
    dbg_printf( "DriverInit: VDDEntryPoint=%WP, OurVMHandle=%x\n", VDDEntryPoint, OurVMHandle );
 
    /* Read the display configuration before doing anything else. */
    LfbBase = 0;
    devNode = ReadDisplayConfig();

    /* Use the Configuration Manager to locate the base address of the linear framebuffer. */
    if( devNode ) {
        RES_DES rd;

        ConfigMGEntryPoint = (DWORD)int_2F_GetEP( 0x1684, CONFIGMG_ID );

        if( CM_Get_First_Log_Conf( &rd, devNode, ALLOC_LOG_CONF ) == CR_SUCCESS ) {
           ULONG cbAllocMax = 0;

            /* Take the largest physical memory range in use by this device
             * and store it into LfbBase. */
            while( CM_Get_Next_Res_Des( &rd, rd, ResType_Mem, NULL, 0 ) == CR_SUCCESS ) {

                /* Experimentally, no MEM_RES was found to be larger than 0x28 bytes
                 * with the QEMU VGA adapter, so this buffer is static, but it would
                 * be better to query the size (with CM_Get_Res_Des_Data_Size) and
                 * then use alloca here. */
                char memRes[0x28];

                if( CM_Get_Res_Des_Data( rd, memRes, sizeof(memRes), 0 ) == CR_SUCCESS ) {
                    PMEM_DES pMemDes = (PMEM_DES)memRes;
                    ULONG cbAlloc = pMemDes->MD_Alloc_End - pMemDes->MD_Alloc_Base + 1;

                    if( cbAlloc > cbAllocMax ) {
                        cbAllocMax = cbAlloc;
                        LfbBase = pMemDes->MD_Alloc_Base;
                    }
                }
            }
        }
    }

    dbg_printf("DriverInit: LfbBase is %lX\n", LfbBase);
 
 		if(!VXD_load())
		{
			dbg_printf("VXD load failure!\n");
			return 0;
		}
 
    /* Return 1 (success) iff we located the physical address of the linear framebuffer. */
    return ( !!LfbBase );
}
#else
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

#ifdef SVGA
		/* connect to 32bit RING-0 driver */
		if(!VXD_load())
		{
			dbg_printf("VXD load failure!\n");
			return 0;
		}
#endif

    return( 1 );    /* Success. */
}
#endif
