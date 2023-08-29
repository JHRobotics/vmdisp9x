/*****************************************************************************

Copyright (c) 2022-2023 Jaroslav Hensl <emulator@emulace.cz>

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

/* WINAPI Escape/ExtEscape calls ends there */

#include "winhack.h"
#include <gdidefs.h>
#include <dibeng.h>
#include "minidrv.h"
#include "drvlib.h"
#include "ddrawi.h"
#include <wchar.h> /* wchar_t */
#include <string.h> /* _fmemset */

#include "version.h"

#ifdef SVGA
# include "svga_all.h"
# include "vxdcall.h"
#endif

#ifdef QEMU
# include "vxdcall.h"
#endif

/*
 * MS supported functions
 */

#define QUERYESCSUPPORT 8
#define OPENGL_GETINFO 0x1101
/* list of neighbor 'known' functions */
#define OPENGL_CMD     0x1100
#define WNDOBJ_SETUP   0x1102

#ifndef DCICOMMAND
#define DCICOMMAND     0x0C03 // 3075
// ^ defined in gfidefs.h
#endif

/* DCICOMMAND */
#define DDCREATEDRIVEROBJECT    10 // create an object
#define DDGET32BITDRIVERNAME    11 // get a 32-bit driver name
#define DDNEWCALLBACKFNS        12 // new callback fns coming
#define DDVERSIONINFO           13 // tells driver the ddraw version

#define MOUSETRAILS             39

/*
 * new escape codes
 */

/* all frame buffer devices */
#define FBHDA_REQ            0x110A

/* debug output from ring-3 application */
#define SVGA_DBG             0x110B

/* update buffer if 'need_call_update' is set */
#define FBHDA_UPDATE         0x110C

/* check for drv <-> vxd <-> dll match */
#define SVGA_API             0x110F

/* VMWare SVGA II codes */
#define SVGA_READ_REG        0x1110
#define SVGA_HDA_REQ         0x1112
#define SVGA_REGION_CREATE   0x1114
#define SVGA_REGION_FREE     0x1115
#define SVGA_SYNC            0x1116
#define SVGA_RING            0x1117

#define SVGA_HWINFO_REGS   0x1121
#define SVGA_HWINFO_FIFO   0x1122
#define SVGA_HWINFO_CAPS   0x1123

typedef struct _longRECT {
  LONG left;
  LONG top;
  LONG right;
  LONG bottom;
} longRECT;

/**
 * OpenGL ICD driver name (0x1101):
 * -------------------------------------
 * Dump from real nvidia driver:
 * 0x0000: 0x02
 * 0x0006: 0x01
 * 0x0008: 0x82 (R)
 * 0x0009: 0x73 (I)
 * 0x000a: 0x86 (V)
 * 0x000b: 0x65 (A)
 * 0x000c: 0x84 (T)
 * 0x000d: 0x78 (N)
 * 0x000e: 0x84 (T)
 * ------------------------------------
 * Buffer length in opengl32.dll is 532 bytes (0x214)
 *
 * Struct info is here:
 * https://community.khronos.org/t/how-to-get-icd-driver-name/50988
 *
 **/
typedef struct _opengl_icd_t
{
	long Version;
	long DriverVersion;
	char DLL[262]; /* wchat_t DLL */
} opengl_icd_t;

#define OPENGL_ICD_SIZE sizeof(opengl_icd_t)

/* software OpenGL (Mesa3D softpipe/llvmpipe) */
const static opengl_icd_t software_icd = {
	0x2,
	0x1,
	"SOFTWARE"
};

#ifdef QEMU
/* Mesa3D SVGA3D OpengGL */
const static opengl_icd_t qemu_icd = {
	0x2,
	0x1,
	"QEMUFX"
};
#endif

#ifdef SVGA
/* Mesa3D SVGA3D OpengGL */
const static opengl_icd_t vmwsvga_icd = {
	0x2,
	0x1,
	"VMWSVGA"
};

#pragma pack(push)
#pragma pack(1)

/* SVGA HDA = hardware direct access */
typedef struct _svga_hda_t
{
	uint32_t       vram_linear;   /* frame buffer address from PM32 (shared memory region, accesable with user space programs too) */
	uint8_t __far *vram_pm16;     /* access from THIS code */
	uint32_t       vram_physical; /* physical address only for drivers */
	uint32_t       vram_size;     /* video ram size */

	uint32_t        fifo_linear;
	uint32_t __far *fifo_pm16;
	uint32_t        fifo_physical;
	uint32_t        fifo_size;

  uint32_t        ul_flags_index;
  uint32_t        ul_fence_index;
  uint32_t        ul_gmr_start;
  uint32_t        ul_gmr_count;
  uint32_t        ul_ctx_start;
  uint32_t        ul_ctx_count;
  uint32_t        ul_surf_start;
  uint32_t        ul_surf_count;
  uint32_t __far *userlist_pm16;
  uint32_t        userlist_linear;
  uint32_t        userlist_length;
} svga_hda_t;

#pragma pack(pop)

#define ULF_DIRTY  0
#define ULF_WIDTH  1
#define ULF_HEIGHT 2
#define ULF_BPP    3
#define ULF_PITCH  4
#define ULF_LOCK_UL   5
#define ULF_LOCK_FIFO 6

#define GMR_INDEX_CNT 6
#define CTX_INDEX_CNT 2

static svga_hda_t SVGAHDA;

#endif /* SVGA only */

#pragma code_seg( _INIT )

#ifdef SVGA
uint32_t GetDevCap(uint32_t search_id);

/*
 * Fix SVGA caps which are known as bad:
 *  - VirtualBox returning A4R4G4B4 instead of R5G6B5,
 *    try to guess it's value from X8R8G8B8.
 *
 */
uint32_t FixDevCap(uint32_t cap_id, uint32_t cap_val)
{
	switch(cap_id)
	{
		case SVGA3D_DEVCAP_SURFACEFMT_R5G6B5:
		{
			if(gSVGA.userFlags & SVGA_USER_FLAGS_RGB565_BROKEN)
			{
				uint32_t xrgb8888 = GetDevCap(SVGA3D_DEVCAP_SURFACEFMT_X8R8G8B8);
				xrgb8888 &= ~SVGA3DFORMAT_OP_SRGBWRITE; /* nvidia */
				return xrgb8888;
			}
			else
			{
				if(cap_val & SVGA3DFORMAT_OP_SAME_FORMAT_UP_TO_ALPHA_RENDERTARGET) /* VirtualBox BUG */
				{
					uint32_t xrgb8888 = GetDevCap(SVGA3D_DEVCAP_SURFACEFMT_X8R8G8B8);
					xrgb8888 &= ~SVGA3DFORMAT_OP_SRGBWRITE; /* nvidia */
					return xrgb8888;
				}
				
				if((cap_val & SVGA3DFORMAT_OP_3DACCELERATION) == 0) /* VirtualBox BUG, older drivers */
				{
					uint32_t xrgb8888 = GetDevCap(SVGA3D_DEVCAP_SURFACEFMT_X8R8G8B8);
					xrgb8888 &= ~SVGA3DFORMAT_OP_SRGBWRITE; /* nvidia */
					if(xrgb8888 & SVGA3DFORMAT_OP_3DACCELERATION)
					{
						return xrgb8888;
					}
				}
			}
		}
		break;
	}
	
	return cap_val;
}

/**
 * Read HW cap, supported old way (GPU gen9):
 *   struct SVGA_FIFO_3D_CAPS in FIFO,
 * and new way (GPU gen10):
 *   SVGA_REG_DEV_CAP HW register
 *
 **/
uint32_t GetDevCap(uint32_t search_id)
{
	if (gSVGA.capabilities & SVGA_CAP_GBOBJECTS)
	{
		/* new way to read device CAPS */
		SVGA_WriteReg(SVGA_REG_DEV_CAP, search_id);
		return FixDevCap(search_id, SVGA_ReadReg(SVGA_REG_DEV_CAP));
	}
	else
	{
		SVGA3dCapsRecord  __far *pCaps = (SVGA3dCapsRecord  __far *)&(gSVGA.fifoMem[SVGA_FIFO_3D_CAPS]);  
		while(pCaps->header.length != 0)
		{
		  if(pCaps->header.type == SVGA3DCAPS_RECORD_DEVCAPS)
		  {
		   	uint32_t datalen = (pCaps->header.length - 2)/2;
	    	SVGA3dCapPair __far *pData = (SVGA3dCapPair __far *)(&pCaps->data);
	    	uint32_t i;
	    	
		 		for(i = 0; i < datalen; i++)
		 		{
		  		uint32_t id  = pData[i][0];
		  		uint32_t val = pData[i][1];
		  			
		  		if(id == search_id)
					{
						return FixDevCap(search_id, val);
					}
				}
			}
			pCaps = (SVGA3dCapsRecord __far *)((uint32_t __far *)pCaps + pCaps->header.length);
		}
	}
	
	return 0;
}

/**
 * Check for 3D support:
 * - we need SVGA_FIFO_CAP_FENCE ans SVGA_FIFO_CAP_SCREEN_OBJECT
 * - SVGA3D_DEVCAP_3D is none zero
 * - at last SVGA3D_DEVCAP_SURFACEFMT_X8R8G8B8 is accelerated (GPU gen9 feature,
 *   GPU gen10 - eg. DirectX surfaces jet aren't supported)
 *
 **/
WORD SVGA_3DSupport()
{
	if(SVGA_HasFIFOCap(SVGA_FIFO_CAP_FENCE) && (SVGA_HasFIFOCap(SVGA_FIFO_CAP_SCREEN_OBJECT) || SVGA_HasFIFOCap(SVGA_FIFO_CAP_SCREEN_OBJECT_2)))
	{
		if(GetDevCap(SVGA3D_DEVCAP_3D)) /* is 3D enabled */
		{
			uint32_t cap_xgrb = GetDevCap(SVGA3D_DEVCAP_SURFACEFMT_X8R8G8B8);
			uint32_t cap_dx_xgrb = GetDevCap(SVGA3D_DEVCAP_DXFMT_X8R8G8B8);
			if((cap_xgrb & SVGA3DFORMAT_OP_3DACCELERATION) != 0) /* at last X8R8G8B8 needs to be accelerated! */
			{
				return 1;
			}
			else if((cap_dx_xgrb & SVGA3DFORMAT_OP_TEXTURE) != 0) /* vGPU10 - DX11 mode */
			{
				return 1;
			}
			else
			{
				dbg_printf("no surface caps %lX %lX!\n", cap_xgrb, cap_dx_xgrb);
			}
		}
		else
		{
			dbg_printf("!GetDevCap(SVGA3D_DEVCAP_3D)\n");
		}
	}
	else
	{
		dbg_printf("no SVGA_FIFO_CAP_FENCE || SVGA_FIFO_CAP_SCREEN_OBJECT\n");
	}
	
	return 0;
}

#define SVGA3D_MAX_MOBS 33280UL /* SVGA3D_MAX_CONTEXT_IDS + SVGA3D_MAX_CONTEXT_IDS + SVGA3D_MAX_SURFACE_IDS */

/**
 * init SVGAHDA -> memory map between user space and (virtual) hardware memory
 *
 **/
void SVGAHDA_init()
{
	_fmemset(&SVGAHDA, 0, sizeof(svga_hda_t));
  
  SVGAHDA.ul_flags_index = 0; // dirty, width, height, bpp, pitch, fifo_lock, ul_lock, fb_lock
  SVGAHDA.ul_fence_index = SVGAHDA.ul_flags_index + 8;
  SVGAHDA.ul_gmr_start   = SVGAHDA.ul_fence_index + 1;
  SVGAHDA.ul_gmr_count   = SVGA_ReadReg(SVGA_REG_GMR_MAX_IDS);
  //SVGAHDA.ul_gmr_count   = SVGA3D_MAX_MOBS;
  SVGAHDA.ul_ctx_start   = SVGAHDA.ul_gmr_start + SVGAHDA.ul_gmr_count*GMR_INDEX_CNT;
  SVGAHDA.ul_ctx_count   = GetDevCap(SVGA3D_DEVCAP_MAX_CONTEXT_IDS);
  SVGAHDA.ul_surf_start  = SVGAHDA.ul_ctx_start + SVGAHDA.ul_ctx_count*CTX_INDEX_CNT;
  SVGAHDA.ul_surf_count  = GetDevCap(SVGA3D_DEVCAP_MAX_SURFACE_IDS);
	SVGAHDA.userlist_length = SVGAHDA.ul_surf_start + SVGAHDA.ul_surf_count;
	
	SVGAHDA.userlist_pm16  = drv_malloc(SVGAHDA.userlist_length * sizeof(uint32_t), &SVGAHDA.userlist_linear);
	
	if(SVGAHDA.userlist_pm16)
	{
		SVGAHDA.userlist_pm16[ULF_DIRTY] = 0xFFFFFFFFUL;
		
		/* zero the memory, because is large than 64k, is much easier do it in PM32 */
		VXD_zeromem(SVGAHDA.userlist_linear, SVGAHDA.userlist_length * sizeof(uint32_t));
		
		SVGAHDA.userlist_pm16[ULF_LOCK_UL] = 0;
		SVGAHDA.userlist_pm16[ULF_LOCK_FIFO] = 0;
	}
	
	dbg_printf("SVGAHDA_init: %ld\n", SVGAHDA.userlist_length * sizeof(uint32_t));
}

/**
 * update frame buffer and fifo addresses in SVGAHDA from gSVGA
 *
 **/
void SVGAHDA_setmode()
{
	if(SVGAHDA.userlist_pm16)
	{
		SVGAHDA.vram_linear   = gSVGA.fbLinear;
		SVGAHDA.vram_pm16     = gSVGA.fbMem;
		SVGAHDA.vram_physical = gSVGA.fbPhy;
		SVGAHDA.vram_size     = gSVGA.vramSize;
		
		SVGAHDA.fifo_linear   = gSVGA.fifoLinear;
	  SVGAHDA.fifo_pm16     = gSVGA.fifoMem;
	  SVGAHDA.fifo_physical = gSVGA.fifoPhy;
		SVGAHDA.fifo_size     = gSVGA.fifoSize;
	}
}

/**
 * Lock resource
 *
 * Note: this is simple spinlock implementation because multitasking isn't
 * preemptive in Win9x this can lead to deadlock. Please use SVGAHDA_trylock
 * instead.
 *
 * Note: please call SVGAHDA_unlock every time after this function
 *
 **/
BOOL SVGAHDA_lock(uint32_t lockid)
{
	volatile uint32_t __far * ptr_lock = SVGAHDA.userlist_pm16 + lockid;
	
	if(SVGAHDA.userlist_pm16)
	{
		_asm
		{
			.386
			push eax
			push ebx
			
			spin_lock:
				mov  eax, 1
				les   bx, ptr_lock
				lock xchg eax, es:[bx]
				test eax, eax
				jnz  spin_lock
			
			pop ebx
			pop eax
			
		};
		
		return TRUE;
	}
	
	return FALSE;
}

/**
 * Try to lock resource and return TRUE is success.
 *
 * Note: if is function successfull you must call SVGAHDA_unlock!
 *
 **/
BOOL SVGAHDA_trylock(uint32_t lockid)
{
	volatile uint32_t __far * ptr_lock = SVGAHDA.userlist_pm16 + lockid;
	BOOL locked = FALSE;
	
	if(SVGAHDA.userlist_pm16)
	{
		_asm
		{
			.386
			push eax
			push ebx
			
			mov  eax, 1
			les   bx, ptr_lock
			lock xchg eax, es:[bx]
			test eax, eax
			jnz lock_false
			mov locked, 1
			lock_false:
			
			pop ebx
			pop eax
			
		};
	}
	
	return locked;
}

/**
 * Unlock resource
 *
 **/
void SVGAHDA_unlock(uint32_t lockid)
{
	volatile uint32_t __far * ptr_lock = SVGAHDA.userlist_pm16 + lockid;
	
	if(SVGAHDA.userlist_pm16)
	{
		_asm
		{
			.386
			push eax
			push ebx
			
			xor  eax, eax
			les   bx, ptr_lock
			lock xchg eax, es:[bx]
			
			pop ebx
			pop eax
			
		};
	}
}

/**
 * This is called after every screen change
 **/
void SVGAHDA_update(DWORD width, DWORD height, DWORD bpp, DWORD pitch)
{
	if(SVGAHDA.userlist_pm16)
	{
		SVGAHDA.userlist_pm16[ULF_WIDTH]  = width;
		SVGAHDA.userlist_pm16[ULF_HEIGHT] = height;
		SVGAHDA.userlist_pm16[ULF_BPP]    = bpp;
		SVGAHDA.userlist_pm16[ULF_PITCH]  = pitch;
	}
}

#endif /* SVGA only */

/**
 * 2022:
 * Control now serve system QUERYESCSUPPORT (0x8), OPENGL_ICD_DRIVER (0x1101),
 *
 * For second one there is no documentation known to me, so functionality was
 * reverse-enginered from exists drivers and system's opengl32.dll.
 * (update: some note here: https://community.khronos.org/t/how-to-get-icd-driver-name/50988)
 * 
 * Retuned driver names is 'SOFTWARE' and OpenGL library name could be
 * located here:
 *   HKLM\Software\Microsoft\Windows\CurrentVersion\OpenGLDrivers
 * For example:
 *   "SOFTWARE"="mesa3d.dll"
 *
 * 2023:
 * Added driver specific call to:
 *   1) accelerate drawing to access framebuffer directly
        FBHDA_REQ
 *   2) access VMWare SVGA II registers and memory to real GPU acceleration
 *
 * 2023-II:
 * Added DCICOMMAND to query DirectDraw/DirectX interface
 *
 **/
LONG WINAPI __loadds Control(LPVOID lpDevice, UINT function,
  LPVOID lpInput, LPVOID lpOutput)
{
	LONG rc = -1;
	
	//dbg_printf("Control (16bit): %d (0x%x)\n", function, function);
	
  if(function == QUERYESCSUPPORT)
  {
  	WORD function_code = 0;
  	function_code =  *((LPWORD)lpInput);
  	switch(function_code)
  	{
  		case OPENGL_GETINFO:
  		case FBHDA_REQ:
  		case FBHDA_UPDATE:
#ifdef SVGA
  		case SVGA_API:	
  		/*
  		 * allow read HW registry/fifo registry and caps even if 3D is 
  	   * disabled for diagnostic tool to check why is disabled
  		 */
  		case SVGA_HWINFO_REGS:
  		case SVGA_HWINFO_FIFO:
  		case SVGA_HWINFO_CAPS:
#endif
  			rc = 1;
  			break;
#ifdef SVGA
  		case SVGA_READ_REG:
  		case SVGA_HDA_REQ:
  		case SVGA_REGION_CREATE:
  		case SVGA_REGION_FREE:
  		case SVGA_SYNC:
			case SVGA_RING:
  			if(wMesa3DEnabled)
  			{
  				rc = 1;
  			}
  			break;
#endif
  		case DCICOMMAND:
  			rc = DD_HAL_VERSION;
  			break;
  		case QUERYESCSUPPORT:
  			rc = 1;
  			break;
  		default:
  			dbg_printf("Control: function check for unknown: %x\n", function_code);
  			break;
  	}
  	
  }
  else if(function == OPENGL_GETINFO) /* input: NULL, output: opengl_icd_t */
  {
#if defined(SVGA)
  	if(wMesa3DEnabled == 0)
  	{
  		_fmemcpy(lpOutput, &software_icd, OPENGL_ICD_SIZE); /* no 3D use software OPENGL */
  	}
  	else
  	{
  		_fmemcpy(lpOutput, &vmwsvga_icd, OPENGL_ICD_SIZE); /* accelerated OPENGL */
  	}
#elif defined(QEMU)
		if(VXD_QEMUFX_supported())
		{
			_fmemcpy(lpOutput, &qemu_icd, OPENGL_ICD_SIZE);
		}
		else
		{
			_fmemcpy(lpOutput, &software_icd, OPENGL_ICD_SIZE);
		}
		
#else
    _fmemcpy(lpOutput, &software_icd, OPENGL_ICD_SIZE);
#endif
  	
  	rc = 1;
  }
  else if(function == DCICOMMAND) /* input ptr DCICMD */
  {
  	DCICMD_t __far *lpDCICMD = lpInput;
  	if(lpDCICMD != NULL)
  	{
  		if(lpDCICMD->dwVersion == DD_VERSION)
  		{
  			switch(lpDCICMD->dwCommand)
  			{
  				case DDCREATEDRIVEROBJECT:
  					dbg_printf("DDCREATEDRIVEROBJECT: ");
  					if(DDCreateDriverObject(0))
  					{
  						uint32_t __far *lpOut = lpOutput;
  						DWORD hInst = DDHinstance();
  						*lpOut = hInst;
  						
  						dbg_printf("success\n");
  						rc = 1;
  					}
  					else
  					{
  						dbg_printf("failure\n");
  						rc = 0;
  					}
  					break;
  				case DDGET32BITDRIVERNAME:
  					dbg_printf("DDGET32BITDRIVERNAME: ");
  					if(DDGet32BitDriverName(lpOutput))
  					{
  						dbg_printf("success\n");
  						rc = 1;
  					}
  					else
  					{
  						dbg_printf("failure\n");
  						rc = 0;
  					}
  					break;
  				case DDNEWCALLBACKFNS:
  					dbg_printf("DDNEWCALLBACKFNS: ");
  					if(DDNewCallbackFns(lpInput))
  					{
  						dbg_printf("success\n");
  						rc = 1;
  					}
  					else
  					{
  						dbg_printf("failure\n");
  						rc = 0;
  					}
  					break;
  				case DDVERSIONINFO:
  					dbg_printf("DDGetVersion\n");
  					DDGetVersion(lpOutput);
  					rc = 1;
  					break;
  				default:
  					dbg_printf("UNK DD code: %lX\n", lpDCICMD->dwCommand);
  					break;
  			}
  		}
  		else if(lpDCICMD->dwVersion == DCI_VERSION)
  		{
  			dbg_printf("DCI(%d) -> failed!\n", lpDCICMD->dwCommand);
  		}
  		else
  		{
  			dbg_printf("hal required: %lX\n", lpDCICMD->dwVersion);
  		}
  	}
  }
  else if(function == MOUSETRAILS)
  {
  	if(lpOutput)
  	{
  		//...
  	}
  	rc = 1;
  }
  else if(function == FBHDA_REQ) /* input: NULL, output: uint32  */
  {
  	uint32_t __far *lpOut = lpOutput;
  	lpOut[0] = FBHDA_linear;
  	dbg_printf("FBHDA request: %lX\n", FBHDA_linear);
  	
  	rc = 1;
  }
  else if(function == FBHDA_UPDATE) /* input RECT, output: NULL */
  {
#ifdef SVGA
		longRECT __far *lpRECT = lpInput;
		SVGA_UpdateRect(lpRECT->left, lpRECT->top, lpRECT->right - lpRECT->left, lpRECT->bottom - lpRECT->top);
#endif
  }
#ifdef SVGA
  else if(function == SVGA_READ_REG) /* input: uint32_t, output: uint32_t */
  {
  	unsigned long val;
  	unsigned long regname = *((unsigned long __far *)lpInput);
  	
  	if(regname == SVGA_REG_ID)
  	{
  		val = gSVGA.deviceVersionId;
  	}
  	else if(regname == SVGA_REG_CAPABILITIES)
  	{
  		val = gSVGA.capabilities;
  	}
  	else
  	{
  		val = SVGA_ReadReg(regname);
  	}
  	
  	*((unsigned long __far *)lpOutput) = val;
  	
  	rc = 1;
  }
  else if(function == SVGA_HDA_REQ) /* input: NULL, output: svga_hda_t  */
  {
  	svga_hda_t __far *lpHDA  = lpOutput;
  	_fmemcpy(lpHDA, &SVGAHDA, sizeof(svga_hda_t));
  	rc = 1;
  }
  else if(function == SVGA_REGION_CREATE) /* input: 2*uint32_t, output: 2*uint32_t */
  {
  	uint32_t __far *lpIn  = lpInput;
  	uint32_t __far *lpOut = lpOutput;
  	uint32_t rid = lpIn[0];
  	
  	dbg_printf("Region id = %ld, max desc = %ld\n", rid, SVGA_ReadReg(SVGA_REG_GMR_MAX_DESCRIPTOR_LENGTH));	
  	
	  if(rid)
	  {
	  	uint32_t lAddr;
	  	uint32_t ppn;
	  	uint32_t pgblk;
	  	
	  	if(VXD_CreateRegion(lpIn[1], &lAddr, &ppn, &pgblk)) /* allocate physical memory */
	  	{
	  		dbg_printf("Region address = %lX, PPN = %lX, GMRBLK = %lX\n", lAddr, ppn, pgblk);
	  		
		    SVGA_WriteReg(SVGA_REG_GMR_ID, rid);
		    SVGA_WriteReg(SVGA_REG_GMR_DESCRIPTOR, ppn);
		    
		    /* refresh all register, so make sure that new commands will accepts this region */
		    SVGA_Flush();
		    
		    lpOut[0] = rid;
		    lpOut[1] = lAddr;
		    lpOut[2] = pgblk;
	  	}
	  	else
	  	{
	  		lpOut[0] = 0;
	  		lpOut[1] = 0;
	  		lpOut[2] = 0;
	  	}
	  }
	  
	  rc = 1;
  }
  else if(function == SVGA_REGION_FREE) /* input: 2*uint32_t, output: NULL */
  {
  	uint32_t __far *lpin = lpInput;
    uint32_t id     = lpin[0];
    uint32_t linear = lpin[1];
    uint32_t pgblk  = lpin[2];
    
    /* flush all register inc. fifo so make sure, that all commands are processed */
    SVGA_Flush();
    
    SVGA_WriteReg(SVGA_REG_GMR_ID, id);
    SVGA_WriteReg(SVGA_REG_GMR_DESCRIPTOR, 0);
    
    /* sync again */
    SVGA_Flush();
    
    /* region physical delete */
    VXD_FreeRegion(linear, pgblk);
    
    rc = 1;
  }
  else if(function == SVGA_HWINFO_REGS) /* input: NULL, output: 256*uint32_t */
  {
  	int i;
  	uint32_t __far *lpreg = lpOutput;
  	for(i = 0; i < 256; i++)
  	{
  		*lpreg = SVGA_ReadReg(i);
  		lpreg++;
  	}
  	
  	rc = 1;
  }
  else if(function == SVGA_HWINFO_FIFO) /* input: NULL, output: 1024*uint32_t */
  {
  	int i;
  	uint32_t __far *lpreg = lpOutput;
  	
  	for(i = 0; i < 1024; i++)
  	{
  		*lpreg = gSVGA.fifoMem[i];
  		lpreg++;
  	}
  	
  	rc = 1;
  }
  else if(function == SVGA_HWINFO_CAPS) /* input: NULL, output: 512*uint32_t */
  {
  	int i;
  	uint32_t         __far *lpreg = lpOutput;
    SVGA3dCapsRecord __far *pCaps;
    SVGA3dCapPair    __far *pData;
    
    
    if (gSVGA.capabilities & SVGA_CAP_GBOBJECTS)
    {
    	/* new way to read device CAPS */
			for (i = 0; i < 512; i++)
			{
				SVGA_WriteReg(SVGA_REG_DEV_CAP, i);
				lpreg[i] = FixDevCap(i, SVGA_ReadReg(SVGA_REG_DEV_CAP));
				dbg_printf("VMSVGA3d_3: cap[%d]=0x%lX\n", i, lpreg[i]);
			}
		}
    else
    {
	    /* old way to read device CAPS */
	    pCaps = (SVGA3dCapsRecord  __far *)&(gSVGA.fifoMem[SVGA_FIFO_3D_CAPS]);
	    
	    while(pCaps->header.length != 0)
	    {
	    	dbg_printf("SVGA_FIFO_3D_CAPS: %ld, %ld\n", pCaps->header.type, pCaps->header.length);
		    if(pCaps->header.type == SVGA3DCAPS_RECORD_DEVCAPS)
		    {
		    	uint32_t datalen = (pCaps->header.length - 2)/2;
		    	pData = (SVGA3dCapPair __far *)(&pCaps->data);
		  		for(i = 0; i < datalen; i++)
		  		{
		  			uint32_t id  = pData[i][0];
		  			uint32_t val = pData[i][1];
		  			
		  			if(id < 512)
		  			{
		  				lpreg[id] = FixDevCap(id, val);
		  			}
		  			
		  			dbg_printf("VMSVGA3d_2: cap[%ld]=0x%lX\n", id, val);
		  		}
		  	}
		  	dbg_printf("SVGA_FIFO_3D_CAPS: end\n");
		  	
		  	pCaps = (SVGA3dCapsRecord __far *)((uint32_t __far *)pCaps + pCaps->header.length);
		  }
		}
  	
  	rc = 1;
  }
  else if(function == SVGA_SYNC) /* input: NULL, output: NULL */
  {
		SVGA_Flush();
		
		rc = 1;
  }
  else if(function == SVGA_RING) /* input: NULL, output: NULL */
  {
		SVGA_WriteReg(SVGA_REG_SYNC, 1);
		
		rc = 1;
  }
  else if(function == SVGA_API) /* input: NULL, output: 2x DWORD */
  {
  	uint32_t __far *lpver = lpOutput;
  	
  	lpver[0] = DRV_API_LEVEL;
  	
    lpver[1] = VXD_apiver();
    
    rc = 1;
  }
#endif /* SVGA only */
  
  /* if command accepted, return */
  if(rc >= 0)
  {
  	return rc;
  }
  
  /* else, call DIB_Control handle */
  dbg_printf("Control: unknown code: %d\n", function);
  return DIB_Control(lpDevice, function, lpInput, lpOutput);
}
