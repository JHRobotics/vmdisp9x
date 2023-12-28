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

#include "vmdahal.h"

const static DD32BITDRIVERDATA_t drv_bridge99 = {
	"vmhal9x.dll",
	"DriverInit",
	0
};

static DDHALMODEINFO_t modeInfo[] = {
	{  640,  480,  640,  8, DDMODEINFO_PALETTIZED, 0, 0x00000000, 0x00000000, 0x00000000, 0x00000000 },
	{  640,  480, 1280, 16,                     0, 0, 0x0000F800, 0x000007E0, 0x0000001F, 0x00000000 },
	{  640,  480, 2560, 32,                     0, 0, 0x00FF0000, 0x0000FF00, 0x000000FF, 0x00000000 },
	{  800,  600,  800,  8, DDMODEINFO_PALETTIZED, 0, 0x00000000, 0x00000000, 0x00000000, 0x00000000 },
	{  800,  600, 1600, 16,                     0, 0, 0x0000F800, 0x000007E0, 0x0000001F, 0x00000000 },
	{  800,  600, 3200, 32,                     0, 0, 0x00FF0000, 0x0000FF00, 0x000000FF, 0x00000000 },
	{ 1024,  768, 1024,  8, DDMODEINFO_PALETTIZED, 0, 0x00000000, 0x00000000, 0x00000000, 0x00000000 },
	{ 1024,  768, 2048, 16,                     0, 0, 0x0000F800, 0x000007E0, 0x0000001F, 0x00000000 },
	{ 1024,  768, 4096, 32,                     0, 0, 0x00FF0000, 0x0000FF00, 0x000000FF, 0x00000000 },
	{ 1280, 1024, 1280,  8, DDMODEINFO_PALETTIZED, 0, 0x00000000, 0x00000000, 0x00000000, 0x00000000 },
	{ 1280, 1024, 2560, 16,                     0, 0, 0x0000F800, 0x000007E0, 0x0000001F, 0x00000000 },
	{ 1280, 1024, 5120, 32,                     0, 0, 0x00FF0000, 0x0000FF00, 0x000000FF, 0x00000000 },
	{ 1600, 1200, 1600,  8, DDMODEINFO_PALETTIZED, 0, 0x00000000, 0x00000000, 0x00000000, 0x00000000 },
	{ 1600, 1200, 3200, 16,                     0, 0, 0x0000F800, 0x000007E0, 0x0000001F, 0x00000000 },
	{ 1600, 1200, 6400, 32,                     0, 0, 0x00FF0000, 0x0000FF00, 0x000000FF, 0x00000000 },
	{  720,  480,  720,  8, DDMODEINFO_PALETTIZED, 0, 0x00000000, 0x00000000, 0x00000000, 0x00000000 },
	{  720,  480, 1440, 16,                     0, 0, 0x0000F800, 0x000007E0, 0x0000001F, 0x00000000 },
	{  720,  480, 2880, 32,                     0, 0, 0x00FF0000, 0x0000FF00, 0x000000FF, 0x00000000 },
	{ 1280,  720, 1280,  8, DDMODEINFO_PALETTIZED, 0, 0x00000000, 0x00000000, 0x00000000, 0x00000000 },
	{ 1280,  720, 2560, 16,                     0, 0, 0x0000F800, 0x000007E0, 0x0000001F, 0x00000000 },
	{ 1280,  720, 5120, 32,                     0, 0, 0x00FF0000, 0x0000FF00, 0x000000FF, 0x00000000 },
	{ 1366,  768, 1280,  8, DDMODEINFO_PALETTIZED, 0, 0x00000000, 0x00000000, 0x00000000, 0x00000000 },
	{ 1366,  768, 2732, 16,                     0, 0, 0x0000F800, 0x000007E0, 0x0000001F, 0x00000000 },
	{ 1366,  768, 5464, 32,                     0, 0, 0x00FF0000, 0x0000FF00, 0x000000FF, 0x00000000 },
	{ 1440,  900, 1440,  8, DDMODEINFO_PALETTIZED, 0, 0x00000000, 0x00000000, 0x00000000, 0x00000000 },
	{ 1440,  900, 2880, 16,                     0, 0, 0x0000F800, 0x000007E0, 0x0000001F, 0x00000000 },
	{ 1440,  900, 5760, 32,                     0, 0, 0x00FF0000, 0x0000FF00, 0x000000FF, 0x00000000 },
	{ 1920, 1080, 1920,  8, DDMODEINFO_PALETTIZED, 0, 0x00000000, 0x00000000, 0x00000000, 0x00000000 },
	{ 1920, 1080, 3840, 16,                     0, 0, 0x0000F800, 0x000007E0, 0x0000001F, 0x00000000 },
	{ 1920, 1080, 7680, 32,                     0, 0, 0x00FF0000, 0x0000FF00, 0x000000FF, 0x00000000 },
	{ 1920, 1200, 1920,  8, DDMODEINFO_PALETTIZED, 0, 0x00000000, 0x00000000, 0x00000000, 0x00000000 },
	{ 1920, 1200, 3840, 16,                     0, 0, 0x0000F800, 0x000007E0, 0x0000001F, 0x00000000 },
	{ 1920, 1200, 7680, 32,                     0, 0, 0x00FF0000, 0x0000FF00, 0x000000FF, 0x00000000 },
};

#define NUMMODES (sizeof(modeInfo)/sizeof(DDHALMODEINFO_t))

/*
 * pre-declare our HAL fns
 */
DWORD __loadds __far __fastcall HALDestroyDriver(LPDDHAL_DESTROYDRIVERDATA);

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
	static DWORD        AlignTbl [ 9 ] = { 64, 64, 64, 64, 64, 64, 64, 64, 64 };
//    static DWORD      AlignTbl [ 9 ] = { 8, 8, 8, 8, 16, 8, 24, 8, 32 };
	int                 ii;
	BOOL                can_flip;
	WORD                heap;
	WORD                bytes_per_pixel;
//	static DWORD    dwpFOURCCs[3];
//	DWORD               bufpos;
	DWORD               screenSize;

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
	if(wBpp > 8)
	{
		hal->ddHALInfo.lpDDExeBufCallbacks = pExeBuf;
	}
	
	bytes_per_pixel = wBpp/8;
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
	hal->ddHALInfo.vmiData.fpPrimary       = dwScreenFlatAddr;
	hal->ddHALInfo.vmiData.dwDisplayWidth  = (DWORD) wScreenX;
	hal->ddHALInfo.vmiData.dwDisplayHeight = (DWORD) wScreenY;
	hal->ddHALInfo.vmiData.lDisplayPitch   = (DWORD) CalcPitch(wScreenX, wBpp);

	_fmemset(&hal->ddHALInfo.vmiData.ddpfDisplay, 0, sizeof(hal->ddHALInfo.vmiData.ddpfDisplay));
	hal->ddHALInfo.vmiData.ddpfDisplay.dwSize = sizeof(DDPIXELFORMAT_t);
	if(modeidx >= 0)
	{
		buildPixelFormat(&modeInfo[modeidx], &hal->ddHALInfo.vmiData.ddpfDisplay);
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
	can_flip = TRUE;

	screenSize = hal->ddHALInfo.vmiData.lDisplayPitch * hal->ddHALInfo.vmiData.dwDisplayHeight;

	vidMem[0].dwFlags = VIDMEM_ISLINEAR;
	vidMem[0].ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
	vidMem[0].fpStart = dwScreenFlatAddr + screenSize;
	vidMem[0].fpEnd   = dwScreenFlatAddr + dwVideoMemorySize - 1;
	
	hal->ddHALInfo.vmiData.dwNumHeaps = 1;
    
	/*
	 * capabilities supported
	 */
	 
	//! hal->ddHALInfo.ddCaps.dwCaps = DDCAPS_GDI;
/*
	hal->ddHALInfo.ddCaps.dwCaps         = DDCAPS_GDI |
                                      DDCAPS_BLT |
                                      //DDCAPS_3D |  //fix #SPR 15230
                                      DDCAPS_ALPHA |
                                      DDCAPS_BLTDEPTHFILL |
                                      DDCAPS_BLTCOLORFILL |
                                      DDCAPS_COLORKEY ;

	hal->ddHALInfo.ddCaps.dwCKeyCaps     = DDCKEYCAPS_SRCBLT;

	hal->ddHALInfo.ddCaps.dwFXCaps       = 0x00000000;

	hal->ddHALInfo.ddCaps.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN |
                                      DDSCAPS_PRIMARYSURFACE |
                                      DDSCAPS_ALPHA;//ly |               //###
//ly                                      DDSCAPS_3DDEVICE |
//ly                                      DDSCAPS_TEXTURE |
//ly                                      DDSCAPS_ZBUFFER;

//    if (S3MobileData.b3DCaps & (Virge_3D | VirgeGX_3D)){
    if (wBpp >= 16){ //ly
    ddHALInfo.ddCaps.dwCaps     |= DDCAPS_BLTDEPTHFILL;
    ddHALInfo.ddCaps.ddsCaps.dwCaps |=
                       DDSCAPS_3DDEVICE |
                       DDSCAPS_TEXTURE |
                       DDSCAPS_ZBUFFER |
                       DDSCAPS_MIPMAP;

    //ddHALInfo.ddCaps.dwZBufferBitDepths = DDBD_16;
    }
*/
/*
        if( STREAMS_PROCESSOR_PRESENT )
        {
        ddHALInfo.ddCaps.dwCaps |=
                    DDCAPS_ALPHA |
                    DDCAPS_COLORKEY |
                    DDCAPS_OVERLAY |
                    DDCAPS_OVERLAYSTRETCH |
                    DDCAPS_OVERLAYFOURCC |
                    DDCAPS_OVERLAYCANTCLIP;

        ddHALInfo.ddCaps.dwCKeyCaps |=
                    DDCKEYCAPS_SRCOVERLAY |
                    DDCKEYCAPS_SRCOVERLAYCLRSPACEYUV |
                    DDCKEYCAPS_SRCOVERLAYONEACTIVE |
                    DDCKEYCAPS_SRCOVERLAYYUV |
                    DDCKEYCAPS_DESTOVERLAY |
                    DDCKEYCAPS_DESTOVERLAYCLRSPACEYUV |
                    DDCKEYCAPS_DESTOVERLAYONEACTIVE |
                    DDCKEYCAPS_DESTOVERLAYYUV;

          ddHALInfo.ddCaps.ddsCaps.dwCaps |= DDSCAPS_OVERLAY;   //###

        ddHALInfo.ddCaps.dwFXAlphaCaps |= DDFXALPHACAPS_OVERLAYALPHAPIXELS;
        ddHALInfo.ddCaps.dwFXCaps |=
                    DDFXCAPS_OVERLAYSTRETCHX |
                    DDFXCAPS_OVERLAYSTRETCHY;
        }*/
	hal->ddHALInfo.ddCaps.dwCaps = DDCAPS_GDI | /* HW is shared with GDI */
	                               DDCAPS_BLT | /* BLT is supported */
	                               DDCAPS_BLTDEPTHFILL | /* depth fill */
                                 DDCAPS_BLTCOLORFILL | /* color fill */
                                 DDCAPS_COLORKEY; /* transparentBlt */

	hal->ddHALInfo.ddCaps.dwCKeyCaps     = DDCKEYCAPS_SRCBLT;
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
	                                       DDSCAPS_PRIMARYSURFACE |
	                                       DDSCAPS_ALPHA;

	if(can_flip)
	{
		hal->ddHALInfo.ddCaps.ddsCaps.dwCaps |= DDSCAPS_FLIP;
	}
	else
	{
		hal->ddHALInfo.ddCaps.ddsCaps.dwCaps &= ~DDSCAPS_FLIP;
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
	hal->ddHALInfo.vmiData.dwOffscreenAlign = AlignTbl[ wBpp >> 2 ];
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
	 *  FOURCCs supported
	 */
	hal->ddHALInfo.ddCaps.dwNumFourCCCodes = 0;
	hal->ddHALInfo.lpdwFourCC = NULL;
	
	/*
	 * mode information
   */
	hal->ddHALInfo.dwNumModes = NUMMODES;
	hal->ddHALInfo.lpModeInfo = modeInfo;

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

	for(modeidx = 0; modeidx < NUMMODES; modeidx++)
	{
		if((modeInfo[modeidx].dwWidth == wScreenX) &&
       (modeInfo[modeidx].dwHeight == wScreenY) &&
       (modeInfo[modeidx].dwBPP == wBpp) )
    {
    	break;
    }
	}
	
	if(modeidx == NUMMODES)
	{
		modeidx = -1;
	}
	
	hal->vramLinear = dwScreenFlatAddr;
	hal->vramSize   = dwVideoMemorySize;

  /*
   * set up hal info
   */
	buildDDHALInfo(hal, modeidx);

  /*
   * copy new data into area of memory shared with 32-bit DLL
   */
	hal->dwWidth  = wScreenX;
	hal->dwHeight = wScreenY;
	hal->dwBpp = wBpp;
	hal->dwPitch = CalcPitch(wScreenX, wBpp);
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
		
	cbDDCallbacks.WaitForVerticalBlank = hal->cb32.WaitForVerticalBlank;
	if(cbDDCallbacks.WaitForVerticalBlank) cbDDCallbacks.dwFlags |= DDHAL_CB32_WAITFORVERTICALBLANK;
	
	cbDDCallbacks.SetMode = hal->cb32.SetMode;
	if(cbDDCallbacks.SetMode) cbDDCallbacks.dwFlags |= DDHAL_CB32_SETMODE;
		
	cbDDCallbacks.SetExclusiveMode = hal->cb32.SetExclusiveMode;
	if(cbDDCallbacks.SetExclusiveMode) cbDDCallbacks.dwFlags |= DDHAL_CB32_SETEXCLUSIVEMODE;
	
	dbg_printf("Dup DD32 calls\n");
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
	dbg_printf("  WaitForVerticalBlank = %lX\n\n", cbDDCallbacks.WaitForVerticalBlank);
	
	hal->pFBHDA32 = FBHDA_linear;
	hal->pFBHDA16 = FBHDA_ptr;
  hal->FBHDA_version = 2;
  
	return lpDDHAL_SetInfo(&(hal->ddHALInfo), bReset);
}

BOOL DDGet32BitDriverName(DD32BITDRIVERDATA_t __far *dd32)
{
	_fmemcpy(dd32, &drv_bridge99, sizeof(drv_bridge99));
	
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

