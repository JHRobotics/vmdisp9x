/*****************************************************************************

Copyright (c) 2023 Jaroslav Hensl <emulator@emulace.cz>

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

/* DirectDraw driver */

#include "winhack.h"
#include <gdidefs.h>
#include <dibeng.h>
#include "minidrv.h"
#include "drvlib.h"
#include "ddrawi.h"
#include <wchar.h> /* wchar_t */
#include <string.h> /* _fmemset */
#include <stddef.h>

#include "3d_accel.h"
#include "vmdahal.h"

const static DD32BITDRIVERDATA_t drv_vmhal9x = {
	"vmhal9x.dll",
	"DriverInit",
	0
};

/*
 * pre-declare our HAL fns
 */
DWORD __loadds __far __fastcall HALDestroyDriver(LPDDHAL_DESTROYDRIVERDATA);

#define FB_MEM_POOL 16777216UL

/*
 * video memory pool usage
 */
static VIDMEM_t vidMem[] = {
  {
  	VIDMEM_ISLINEAR, 0x00000000, 0x00000000,
		{0},
		{0}
	},
  {
  	VIDMEM_ISLINEAR, 0x00000000, 0x00000000,
		{0},
		{0}
	},
  {
  	VIDMEM_ISLINEAR, 0x00000000, 0x00000000,
		{0},
		{0}
	},
  {
  	VIDMEM_ISLINEAR, 0x00000000, 0x00000000,
		{0},
		{0}
	}
};

/*
 * callbacks from the DIRECTDRAW object
 */
static DDHAL_DDCALLBACKS_t cbDDCallbacks = {
	sizeof(DDHAL_DDCALLBACKS_t),
	0,                          // dwFlags
	HALDestroyDriver,           // DestroyDriver
	NULL,                       // CreateSurface
	NULL,                       // SetColorKey
	NULL,                       // SetMode
	NULL,                       // WaitForVerticalBlank -> uses 32-bit
	NULL,                       // CanCreateSurface
	NULL,                       // CreatePalette
	NULL                        // lpReserved1
};

/*
 * callbacks from the DIRECTDRAWSURFACE object
 */
static DDHAL_DDSURFACECALLBACKS_t cbDDSurfaceCallbacks =
{
	sizeof(DDHAL_DDSURFACECALLBACKS_t),
	0,                          // dwFlags
	NULL,                       // DestroySurface
	NULL,                       // Flip -> uses 32-bit
	NULL,                       // SetClipList
	NULL,                       // Lock -> uses 32-bit
	NULL,                       // Unlock
	NULL,                       // Blt -> uses 32-bit
	NULL,                       // SetColorKey
	NULL,                       // AddAttachedSurface
	NULL,                       // lpReserved
	NULL,                       // lpReserved
	NULL,                       // UpdateOverlay
	NULL,                       // lpReserved
	NULL,                       // lpReserved
	NULL                        // SetPalette
};

/*
 * callbacks from the DIRECTDRAWPALETTE object
 */
static DDHAL_DDPALETTECALLBACKS_t cbDDPaletteCallbacks =
{
	sizeof(DDHAL_DDPALETTECALLBACKS_t),
	0,                          // dwFlags
	NULL,                       // DestroyPalette
	NULL                        // SetEntries
};

static DDHAL_DDEXEBUFCALLBACKS_t cbDDExeBufCallbacks = 
{
	sizeof(DDHAL_DDEXEBUFCALLBACKS_t),
	0,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL
};

static VMDAHAL_t __far *pm16VMDAHAL = NULL;
static DWORD linVMDAHAL = 0;

static LPDDHAL_SETINFO lpDDHAL_SetInfo = NULL;

#pragma code_seg( _INIT )

static BOOL DDGetPtr(VMDAHAL_t __far *__far *pm16ptr, DWORD __far *linear)
{
	if(pm16VMDAHAL == NULL)
	{
		pm16VMDAHAL = drv_malloc(sizeof(VMDAHAL_t), &linVMDAHAL);
		if(pm16VMDAHAL != NULL)
		{
			_fmemset(pm16VMDAHAL, 0, sizeof(VMDAHAL_t));
			pm16VMDAHAL->dwSize = sizeof(VMDAHAL_t);

			pm16VMDAHAL->pFBHDA32 = hda_linear;
			pm16VMDAHAL->pFBHDA16 = hda;
 		 	pm16VMDAHAL->FBHDA_version = 2024;
		}
	}
	
	if(pm16VMDAHAL == NULL)
	{
		return FALSE;
	}
	
	if(pm16ptr != NULL)
	{
		*pm16ptr = pm16VMDAHAL;
	}
	
	if(linear != NULL)
	{
		*linear = linVMDAHAL;
	}
	
	return TRUE;
}

/*
 * buildPixelFormat
 *
 * build DDPIXELFORMAT structure
 */
static void buildPixelFormat(LPDDHALMODEINFO lpMode, LPDDPIXELFORMAT lpddpf)
{
	lpddpf->dwFlags = DDPF_RGB;
	lpddpf->dwRGBBitCount = lpMode->dwBPP;

	if( lpMode->wFlags & DDMODEINFO_PALETTIZED)
	{
		lpddpf->dwFlags  |= DDPF_PALETTEINDEXED8;
	}

	lpddpf->dwRBitMask = lpMode->dwRBitMask;
	lpddpf->dwGBitMask = lpMode->dwGBitMask;
	lpddpf->dwBBitMask = lpMode->dwBBitMask;
	lpddpf->dwRGBAlphaBitMask = lpMode->dwAlphaBitMask;
} /* buildPixelFormat */

/*
 * buildDDHALInfo
 *
 * build DDHALInfo structure
 */
static void buildDDHALInfo(VMDAHAL_t __far *hal, int modeidx)
{
	static DWORD        AlignTbl [ 9 ] = {
		FBHDA_ROW_ALIGN, FBHDA_ROW_ALIGN, FBHDA_ROW_ALIGN,
		FBHDA_ROW_ALIGN, FBHDA_ROW_ALIGN, FBHDA_ROW_ALIGN,
		FBHDA_ROW_ALIGN, FBHDA_ROW_ALIGN, FBHDA_ROW_ALIGN
	}; /* reduced to 8 to work correctly with glPixelStorei */
	
	int                 ii;
	BOOL                can_flip;
	WORD                heap;
	WORD                bytes_per_pixel;
//	static DWORD    dwpFOURCCs[3];
//	DWORD               bufpos;

	LPDWORD pGbl, pHAL;
	LPDDHAL_DDEXEBUFCALLBACKS pExeBuf;

	pGbl = hal->ddHALInfo.lpD3DGlobalDriverData;
	pHAL = hal->ddHALInfo.lpD3DHALCallbacks;
	pExeBuf = hal->ddHALInfo.lpDDExeBufCallbacks;

	// dont trash the 3D callbacks. only the 2D info.
	_fmemset(&hal->ddHALInfo, 0, sizeof(hal->ddHALInfo));
	hal->ddHALInfo.lpD3DHALCallbacks = pHAL;
	hal->ddHALInfo.lpD3DGlobalDriverData = pGbl;
    
	//clean 3d callback pointers on 8bpps
	if(hda->bpp > 8)
	{
		hal->ddHALInfo.lpDDExeBufCallbacks = pExeBuf;
	}
	
	bytes_per_pixel = (hda->bpp+7)/8;
	hal->ddHALInfo.dwSize = sizeof(hal->ddHALInfo);

  hal->ddHALInfo.hInstance = hal->hInstance;
	hal->ddHALInfo.lpPDevice = lpDriverPDevice;

	/*
	 * current video mode
	 */
	hal->ddHALInfo.dwModeIndex = (DWORD)modeidx;

	/*
	 * current primary surface attributes
	 */
	hal->ddHALInfo.vmiData.fpPrimary       = hda->vram_pm32 + hda->system_surface;
	hal->ddHALInfo.vmiData.dwDisplayWidth  = hda->width;
	hal->ddHALInfo.vmiData.dwDisplayHeight = hda->height;
	hal->ddHALInfo.vmiData.lDisplayPitch   = hda->pitch;

	_fmemset(&hal->ddHALInfo.vmiData.ddpfDisplay, 0, sizeof(hal->ddHALInfo.vmiData.ddpfDisplay));
	hal->ddHALInfo.vmiData.ddpfDisplay.dwSize = sizeof(DDPIXELFORMAT_t);
	if(modeidx >= 0)
	{
		buildPixelFormat(&(VMDAHAL_modes(hal)[modeidx]), &hal->ddHALInfo.vmiData.ddpfDisplay);
	}

	/*
	 * video memory pool information
	 */
	hal->ddHALInfo.vmiData.pvmList = vidMem;

	/*
	 * set up the pointer to the first available video memory after
	 * the primary surface
	 */
	hal->ddHALInfo.vmiData.dwNumHeaps  = 0;
	heap = 0;
	can_flip = hda->flags & FB_SUPPORT_FLIPING;

	vidMem[0].dwFlags = VIDMEM_ISLINEAR;
	vidMem[0].ddsCaps.dwCaps = 0;//DDSCAPS_OFFSCREENPLAIN; - what this memory CANNOT be used for

#if 0
	if(hda->system_surface + hda->stride < FB_MEM_POOL && hda->vram_size > FB_MEM_POOL)
	{
		vidMem[0].fpStart = hda->vram_pm32 + FB_MEM_POOL;
		vidMem[0].fpEnd   = hda->vram_pm32 + hda->vram_size - hda->overlays_size - 1;

		vidMem[1].fpStart = hda->vram_pm32 + hda->system_surface + hda->stride;
		vidMem[1].fpEnd   = hda->vram_pm32 + FB_MEM_POOL - 1;
		vidMem[1].dwFlags = VIDMEM_ISLINEAR;
		vidMem[1].ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;

		hal->ddHALInfo.vmiData.dwNumHeaps = 2;
	}
	else
	{
#endif
		vidMem[0].fpStart = hda->vram_pm32 + hda->system_surface + hda->stride;	
		vidMem[0].fpEnd   = hda->vram_pm32 + hda->vram_size - hda->overlays_size - 1;
		hal->ddHALInfo.vmiData.dwNumHeaps = 1;
#if 0
	}
#endif

	/*
	 * capabilities supported
	 */
	hal->ddHALInfo.ddCaps.dwCaps = DDCAPS_GDI | /* HW is shared with GDI */
	                               DDCAPS_BLT | /* BLT is supported */
	                               DDCAPS_BLTDEPTHFILL | /* depth fill */
                                 DDCAPS_BLTCOLORFILL | /* color fill */
                                 DDCAPS_BLTSTRETCH   | /* stretching blt */
                                 DDCAPS_COLORKEY     | /* transparentBlt */
                                 DDCAPS_CANBLTSYSMEM; /* from to sysmem blt */

	hal->ddHALInfo.ddCaps.dwCKeyCaps     = DDCKEYCAPS_SRCBLT | DDCKEYCAPS_DESTBLT;
	hal->ddHALInfo.ddCaps.dwFXCaps       = DDFXCAPS_BLTARITHSTRETCHY |
	                                       DDFXCAPS_BLTARITHSTRETCHYN |
	                                       DDFXCAPS_BLTMIRRORLEFTRIGHT |
	                                       DDFXCAPS_BLTMIRRORUPDOWN |
	                                       DDFXCAPS_BLTSHRINKX |
	                                       DDFXCAPS_BLTSHRINKXN |
	                                       DDFXCAPS_BLTSHRINKY |
	                                       DDFXCAPS_BLTSHRINKYN |
	                                       DDFXCAPS_BLTSTRETCHX | 
	                                       DDFXCAPS_BLTSTRETCHXN |
	                                       DDFXCAPS_BLTSTRETCHY |
	                                       DDFXCAPS_BLTSTRETCHYN;
	                                       // DDFXCAPS_BLTROTATION
	                                       // DDFXCAPS_BLTROTATION90

	hal->ddHALInfo.ddCaps.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN |
	                                       DDSCAPS_ALPHA;


	/* 3D support */
	if(hal->d3dhal_global != NULL)
	{
		hal->ddHALInfo.ddCaps.dwCaps         |= DDCAPS_3D | DDCAPS_ALPHA;
		hal->ddHALInfo.ddCaps.dwCaps2         = hal->d3dhal_flags.caps2;
		
  	hal->ddHALInfo.ddCaps.ddsCaps.dwCaps |= hal->d3dhal_flags.ddscaps;
  	hal->ddHALInfo.ddCaps.dwZBufferBitDepths |= hal->d3dhal_flags.zcaps;

		hal->ddHALInfo.ddCaps.dwAlphaBltConstBitDepths = hal->d3dhal_flags.alpha_const;
		hal->ddHALInfo.ddCaps.dwAlphaBltPixelBitDepths = hal->d3dhal_flags.alpha_pixel;
		hal->ddHALInfo.ddCaps.dwAlphaBltSurfaceBitDepths  = hal->d3dhal_flags.alpha_surface;
  }

	if(can_flip)
	{
		hal->ddHALInfo.ddCaps.ddsCaps.dwCaps |= DDSCAPS_FLIP | DDSCAPS_PRIMARYSURFACE;
	}

  /*
   * fill in rops supported
   */
	for(ii=0; ii<DD_ROP_SPACE; ii++)
	{
		hal->ddHALInfo.ddCaps.dwRops[ii] = 0xFFFFFFFFUL;// JH: we have all ROP3s :-)
	}

	/*
	 * required alignments of the scan lines for each kind of memory
	 */
	hal->ddHALInfo.vmiData.dwOffscreenAlign = AlignTbl[ hda->bpp >> 2 ];
	hal->ddHALInfo.vmiData.dwOverlayAlign = 8;
	hal->ddHALInfo.vmiData.dwTextureAlign = 8;
	hal->ddHALInfo.vmiData.dwZBufferAlign = 8;

	/*
	 * callback functions
	 */
	_fmemset(&cbDDCallbacks, 0, sizeof(cbDDCallbacks));
	_fmemset(&cbDDSurfaceCallbacks, 0, sizeof(cbDDSurfaceCallbacks));
	_fmemset(&cbDDPaletteCallbacks, 0, sizeof(cbDDPaletteCallbacks));

	cbDDCallbacks.dwSize = sizeof(cbDDCallbacks);
	cbDDCallbacks.DestroyDriver = (LPDDHAL_DESTROYDRIVER) HALDestroyDriver;
	
	cbDDSurfaceCallbacks.dwSize = sizeof(cbDDSurfaceCallbacks);
	cbDDPaletteCallbacks.dwSize = sizeof(cbDDPaletteCallbacks);

	hal->ddHALInfo.lpDDCallbacks        = &cbDDCallbacks;
	hal->ddHALInfo.lpDDSurfaceCallbacks = &cbDDSurfaceCallbacks;
	hal->ddHALInfo.lpDDPaletteCallbacks = &cbDDPaletteCallbacks;

	/*
	 *  FOURCCs not supported
	 */
	hal->ddHALInfo.ddCaps.dwNumFourCCCodes = 0;
	hal->ddHALInfo.lpdwFourCC = NULL;
	
	/*
	 * mode information
   */
	hal->ddHALInfo.dwNumModes = hal->modes_count;
	hal->ddHALInfo.lpModeInfo = VMDAHAL_modes(hal);
	
	/*
	 * D3D HAL
	 */
	hal->ddHALInfo.lpD3DGlobalDriverData = (ULONG_PTR)hal->d3dhal_global;
	hal->ddHALInfo.lpD3DHALCallbacks     = (ULONG_PTR)hal->d3dhal_callbacks;
	
	/*
	 * Exec Buffers (active when fill)
	 */
	_fmemset(&cbDDExeBufCallbacks, 0, sizeof(DDHAL_DDEXEBUFCALLBACKS_t));
	cbDDExeBufCallbacks.dwSize = sizeof(DDHAL_DDEXEBUFCALLBACKS_t);
	if(hal->ddHALInfo.lpDDExeBufCallbacks == NULL)
	{
		hal->ddHALInfo.lpDDExeBufCallbacks = &cbDDExeBufCallbacks;
	}
} /* buildDDHALInfo */

BOOL DDCreateDriverObject(int bReset)
{
	VMDAHAL_t __far *hal;
	int modeidx = -1;
	
	if(lpDDHAL_SetInfo == NULL)
	{
		dbg_printf("DDCreateDriverObject: lpDDHAL_SetInfo = NULL\n");
		return FALSE;
	}
	
	if(!DDGetPtr(&hal, NULL))
	{
		dbg_printf("DDCreateDriverObject: DDGetPtr = NULL\n");
		return FALSE;
	}

	for(modeidx = 0; modeidx < hal->modes_count; modeidx++)
	{
		if((hal->modes[modeidx].dwWidth == hda->width) &&
       (hal->modes[modeidx].dwHeight == hda->height) &&
       (hal->modes[modeidx].dwBPP == hda->bpp) )
    {
    	break;
    }
	}
	
	if(modeidx == hal->modes_count)
	{
		return FALSE;
	}
	
	hal->vramLinear = hda->vram_pm32;
	hal->vramSize   = hda->vram_size;

  /*
   * set up hal info
   */
	buildDDHALInfo(hal, modeidx);

  /*
   * copy new data into area of memory shared with 32-bit DLL
   */
	hal->dwWidth  = hda->width;
	hal->dwHeight = hda->height;
	hal->dwBpp = hda->bpp;
	hal->dwPitch = hda->pitch;
  /*
   * get addresses of 32-bit routines
   */
	cbDDCallbacks.CanCreateSurface = hal->cb32.CanCreateSurface;
	if(cbDDCallbacks.CanCreateSurface) cbDDCallbacks.dwFlags |= DDHAL_CB32_CANCREATESURFACE;
	
	cbDDCallbacks.CreateSurface = hal->cb32.CreateSurface;	
	if(cbDDCallbacks.CreateSurface) cbDDCallbacks.dwFlags |= DDHAL_CB32_CREATESURFACE;

	cbDDSurfaceCallbacks.Blt = hal->cb32.Blt;
	if(cbDDSurfaceCallbacks.Blt) cbDDSurfaceCallbacks.dwFlags |= DDHAL_SURFCB32_BLT;
		
	cbDDSurfaceCallbacks.Flip = hal->cb32.Flip;
	if(cbDDSurfaceCallbacks.Flip)	cbDDSurfaceCallbacks.dwFlags |= DDHAL_SURFCB32_FLIP;
	
	cbDDSurfaceCallbacks.Lock = hal->cb32.Lock;
	if(cbDDSurfaceCallbacks.Lock) cbDDSurfaceCallbacks.dwFlags |= DDHAL_SURFCB32_LOCK;
	
	cbDDSurfaceCallbacks.Unlock = hal->cb32.Unlock;
	if(cbDDSurfaceCallbacks.Unlock) cbDDSurfaceCallbacks.dwFlags |= DDHAL_SURFCB32_UNLOCK;
	
	cbDDSurfaceCallbacks.GetBltStatus = hal->cb32.GetBltStatus;
	if(cbDDSurfaceCallbacks.GetBltStatus) cbDDSurfaceCallbacks.dwFlags |= DDHAL_SURFCB32_GETBLTSTATUS;
	
	cbDDSurfaceCallbacks.GetFlipStatus = hal->cb32.GetFlipStatus;
	if(cbDDSurfaceCallbacks.GetFlipStatus) cbDDSurfaceCallbacks.dwFlags |= DDHAL_SURFCB32_GETFLIPSTATUS;
	
	cbDDSurfaceCallbacks.DestroySurface = hal->cb32.DestroySurface;
	if(cbDDSurfaceCallbacks.DestroySurface) cbDDSurfaceCallbacks.dwFlags |= DDHAL_SURFCB32_DESTROYSURFACE;
	
	hal->ddHALInfo.GetDriverInfo = hal->cb32.GetDriverInfo;
	if(hal->ddHALInfo.GetDriverInfo) hal->ddHALInfo.dwFlags |= DDHALINFO_GETDRIVERINFOSET;
	
	hal->ddHALInfo.dwFlags |= hal->cb32.flags;
		
	cbDDCallbacks.WaitForVerticalBlank = hal->cb32.WaitForVerticalBlank;
	if(cbDDCallbacks.WaitForVerticalBlank) cbDDCallbacks.dwFlags |= DDHAL_CB32_WAITFORVERTICALBLANK;
	
	cbDDCallbacks.SetMode = hal->cb32.SetMode;
	if(cbDDCallbacks.SetMode) cbDDCallbacks.dwFlags |= DDHAL_CB32_SETMODE;
		
	cbDDCallbacks.SetExclusiveMode = hal->cb32.SetExclusiveMode;
	if(cbDDCallbacks.SetExclusiveMode) cbDDCallbacks.dwFlags |= DDHAL_CB32_SETEXCLUSIVEMODE;
	
	cbDDCallbacks.FlipToGDISurface = hal->cb32.FlipToGDISurface;
	if(cbDDCallbacks.FlipToGDISurface) cbDDCallbacks.dwFlags |= DDHAL_CB32_FLIPTOGDISURFACE;
	
	cbDDSurfaceCallbacks.SetColorKey = hal->cb32.SetColorKey;
	if(cbDDSurfaceCallbacks.SetColorKey) cbDDSurfaceCallbacks.dwFlags |= DDHAL_SURFCB32_SETCOLORKEY;

	cbDDSurfaceCallbacks.AddAttachedSurface = hal->cb32.AddAttachedSurface;
	if(cbDDSurfaceCallbacks.AddAttachedSurface) cbDDSurfaceCallbacks.dwFlags |= DDHAL_SURFCB32_ADDATTACHEDSURFACE;

	cbDDSurfaceCallbacks.SetOverlayPosition = hal->cb32.SetOverlayPosition;
	if(cbDDSurfaceCallbacks.SetOverlayPosition) cbDDSurfaceCallbacks.dwFlags |= DDHAL_SURFCB32_SETOVERLAYPOSITION;

	if(hal->cb32.DestroyDriver)
	{
		cbDDCallbacks.DestroyDriver = hal->cb32.DestroyDriver;
		cbDDCallbacks.dwFlags |= DDHAL_CB32_DESTROYDRIVER;
	}
	
	if(hal->ddHALInfo.lpDDExeBufCallbacks)
	{
		if(hal->d3dhal_exebuffcallbacks.CanCreateExecuteBuffer)
		{
			hal->ddHALInfo.lpDDExeBufCallbacks->CanCreateExecuteBuffer = hal->d3dhal_exebuffcallbacks.CanCreateExecuteBuffer;
			hal->ddHALInfo.lpDDExeBufCallbacks->dwFlags |= DDHAL_EXEBUFCB32_CANCREATEEXEBUF;
		}

		if(hal->d3dhal_exebuffcallbacks.CreateExecuteBuffer)
		{
			hal->ddHALInfo.lpDDExeBufCallbacks->CreateExecuteBuffer = hal->d3dhal_exebuffcallbacks.CreateExecuteBuffer;
			hal->ddHALInfo.lpDDExeBufCallbacks->dwFlags |= DDHAL_EXEBUFCB32_CREATEEXEBUF;
		}

		if(hal->d3dhal_exebuffcallbacks.DestroyExecuteBuffer)
		{
			hal->ddHALInfo.lpDDExeBufCallbacks->DestroyExecuteBuffer = hal->d3dhal_exebuffcallbacks.DestroyExecuteBuffer;
			hal->ddHALInfo.lpDDExeBufCallbacks->dwFlags |= DDHAL_EXEBUFCB32_DESTROYEXEBUF;
		}

		if(hal->d3dhal_exebuffcallbacks.LockExecuteBuffer)
		{
			hal->ddHALInfo.lpDDExeBufCallbacks->LockExecuteBuffer = hal->d3dhal_exebuffcallbacks.LockExecuteBuffer;
			hal->ddHALInfo.lpDDExeBufCallbacks->dwFlags |= DDHAL_EXEBUFCB32_LOCKEXEBUF;
		}

		if(hal->d3dhal_exebuffcallbacks.UnlockExecuteBuffer)
		{
			hal->ddHALInfo.lpDDExeBufCallbacks->UnlockExecuteBuffer = hal->d3dhal_exebuffcallbacks.UnlockExecuteBuffer;
			hal->ddHALInfo.lpDDExeBufCallbacks->dwFlags |= DDHAL_EXEBUFCB32_UNLOCKEXEBUF;
		}
	}

	dbg_printf("Dump DD32 calls\n");
	dbg_printf("  CanCreateSurface = %lX\n", cbDDCallbacks.CanCreateSurface);
	dbg_printf("  CreateSurface = %lX\n", cbDDCallbacks.CreateSurface);
	dbg_printf("  Blt = %lX\n", cbDDSurfaceCallbacks.Blt);
	dbg_printf("  Flip = %lX\n", cbDDSurfaceCallbacks.Flip);
	dbg_printf("  Lock = %lX\n", cbDDSurfaceCallbacks.Lock);
	dbg_printf("  Unlock = %lX\n", cbDDSurfaceCallbacks.Unlock);
	dbg_printf("  GetBltStatus = %lX\n", cbDDSurfaceCallbacks.GetBltStatus);
	dbg_printf("  GetFlipStatus = %lX\n", cbDDSurfaceCallbacks.GetFlipStatus);
	dbg_printf("  DestroySurface = %lX\n", cbDDSurfaceCallbacks.DestroySurface);
	dbg_printf("  GetDriverInfo = %lX\n", hal->ddHALInfo.GetDriverInfo);
	dbg_printf("  WaitForVerticalBlank = %lX\n", cbDDCallbacks.WaitForVerticalBlank);
	dbg_printf("  DestroyDriver = %lX\n\n", cbDDCallbacks.DestroyDriver);

	return lpDDHAL_SetInfo(&(hal->ddHALInfo), bReset);
}

BOOL DDGet32BitDriverName(DD32BITDRIVERDATA_t __far *dd32)
{
	_fmemcpy(dd32, &drv_vmhal9x, sizeof(drv_vmhal9x));
	
	if(DDGetPtr(NULL, &(dd32->dwContext)))
	{
		return TRUE;
	}
	
	return FALSE;
}

BOOL DDNewCallbackFns(DCICMD_t __far *lpDCICMD)
{
  LPDDHALDDRAWFNS pfns;
  
	pfns = (LPVOID) lpDCICMD->dwParam1;
	lpDDHAL_SetInfo = pfns->lpSetInfo;
	
	return TRUE;
}

DWORD __loadds __far __fastcall HALDestroyDriver(LPDDHAL_DESTROYDRIVERDATA lpDestroyDriverData)
{
	lpDestroyDriverData->ddRVal = DD_OK;
	
	lpDDHAL_SetInfo = NULL;
	
	return DDHAL_DRIVER_HANDLED;
} /* HALDestroyDriver */

void DDGetVersion(DDVERSIONDATA_t __far *lpVer)
{
	_fmemset(lpVer, 0, sizeof(DDVERSIONDATA_t));
	lpVer->dwHALVersion = DD_RUNTIME_VERSION;
	
	dbg_printf("struct size: %d (%d %d %d %d)\n",
		sizeof(VMDAHAL_t),
		offsetof(VMDAHAL_t, ddpf),
		offsetof(VMDAHAL_t, ddHALInfo),
		offsetof(VMDAHAL_t, cb32),
		offsetof(VMDAHAL_t, FBHDA_version)
	);
}

DWORD DDHinstance(void)
{
	VMDAHAL_t __far *hal;
	
	if(!DDGetPtr(&hal, NULL))
	{
		return 0;
	}
	
	return hal->ddHALInfo.hInstance;	
}

