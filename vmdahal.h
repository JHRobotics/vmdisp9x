#ifndef __VMDAHAL_H__INCLUDED__
#define __VMDAHAL_H__INCLUDED__

#pragma pack(push)
#pragma pack(1)

typedef struct DDHALMODEINFO2
{
    DWORD	dwWidth;		// width (in pixels) of mode
    DWORD	dwHeight;		// height (in pixels) of mode
    LONG	lPitch;			// pitch (in bytes) of mode
    DWORD	dwBPP;			// bits per pixel
    WORD	wFlags;			// flags
    WORD	wRefreshRate;		// refresh rate
    DWORD	dwRBitMask;		// red bit mask
    DWORD	dwGBitMask;		// green bit mask
    DWORD	dwBBitMask;		// blue bit mask
    DWORD	dwAlphaBitMask;		// alpha bit mask
} DDHALMODEINFO2_t;

typedef struct VMDAHALCB32
{
	LPDDHAL_DESTROYDRIVER             DestroyDriver;
	LPDDHAL_CREATESURFACE             CreateSurface;
	LPDDHAL_CANCREATESURFACE          CanCreateSurface;
	LPDDHALSURFCB_DESTROYSURFACE      DestroySurface;
	LPDDHALSURFCB_FLIP                Flip;
	LPDDHALSURFCB_SETCLIPLIST         SetClipList;
	LPDDHALSURFCB_LOCK                Lock;
	LPDDHALSURFCB_UNLOCK              Unlock;
	LPDDHALSURFCB_BLT                 Blt;
	LPDDHALSURFCB_SETCOLORKEY         SetColorKey;
	LPDDHALSURFCB_ADDATTACHEDSURFACE  AddAttachedSurface;
	LPDDHALSURFCB_GETBLTSTATUS        GetBltStatus;
	LPDDHALSURFCB_GETFLIPSTATUS       GetFlipStatus;
	LPDDHALSURFCB_UPDATEOVERLAY       UpdateOverlay;
	LPDDHALSURFCB_SETOVERLAYPOSITION  SetOverlayPosition;
	LPDDHAL_GETDRIVERINFO             GetDriverInfo;
	LPDDHAL_WAITFORVERTICALBLANK      WaitForVerticalBlank;
	LPDDHAL_SETMODE		                SetMode;
	LPDDHAL_SETEXCLUSIVEMODE          SetExclusiveMode;
	LPDDHAL_FLIPTOGDISURFACE          FlipToGDISurface;
	DWORD                             flags;
} VMDAHALCB32_t;

#define DISP_MODES_MAX 512

typedef struct VMDAHAL_D3DCAPS
{
	DWORD ddscaps;
	DWORD zcaps;
	DWORD caps2;
	DWORD alpha_const;
	DWORD alpha_pixel;
	DWORD alpha_surface;
} VMDAHAL_D3DCAPS_t;

typedef struct VMDAHAL
{
	DWORD dwSize;
	DWORD vramLinear;
	DWORD vramSize;
	
	DWORD dwWidth;
	DWORD dwHeight;
	DWORD dwBpp;
	DWORD dwPitch;
	
  DDPIXELFORMAT_t ddpf;
  DDHALINFO_t     ddHALInfo;
  
  VMDAHALCB32_t cb32;
  
  DWORD hInstance;
  
  DWORD pFBHDA32;
  void __far *pFBHDA16;
  DWORD FBHDA_version;
  
  DWORD hDC;
  
  DDHALMODEINFO2_t modes[DISP_MODES_MAX];
  DWORD modes_count;
  DWORD custom_mode_id;

  DWORD d3dhal_global;
  DWORD d3dhal_callbacks;
  VMDAHAL_D3DCAPS_t d3dhal_flags;
  DDHAL_DDEXEBUFCALLBACKS_t d3dhal_exebuffcallbacks;
  
	BOOL invalid;
} VMDAHAL_t;
#pragma pack(pop)

#define VMDAHAL_modes(_hal) ((struct DDHALMODEINFO __far *)(&(_hal->modes[0])))

#endif
