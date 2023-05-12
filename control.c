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
#include <wchar.h> /* wchar_t */
#include <string.h> /* _fmemset */

#include "version.h"

#ifdef SVGA
# include "svga_all.h"
# include "control_vxd.h"
#endif

/*
 * MS supported functions
 */

#define QUERYESCSUPPORT 8
#define OPENGL_GETINFO 0x1101
/* list of neighbor 'known' functions */
#define OPENGL_CMD     0x1100
#define WNDOBJ_SETUP   0x1102

/*
 * new escape codes
 */

/* all frame buffer devices */
#define FBHDA_REQ            0x110A

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
#ifdef CAP_R5G6B5_ALWAYS_WRONG
		{
			uint32_t xrgb8888 = GetDevCap(SVGA3D_DEVCAP_SURFACEFMT_X8R8G8B8);
			xrgb8888 &= ~SVGA3DFORMAT_OP_SRGBWRITE; /* nvidia */
			return xrgb8888;
		}
#else
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
#endif
			
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
			uint32_t surfcap = GetDevCap(SVGA3D_DEVCAP_SURFACEFMT_X8R8G8B8);
			if((surfcap & SVGA3DFORMAT_OP_3DACCELERATION) != 0) /* at last X8R8G8B8 needs to be accelerated! */
			{
				return 1;
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

/**
 * init SVGAHDA -> memory map between user space and (virtual) hardware memory
 *
 **/
void SVGAHDA_init()
{
	_fmemset(&SVGAHDA, 0, sizeof(svga_hda_t));
  
  SVGAHDA.ul_flags_index = 0; // dirty, width, height, bpp, pitch
  SVGAHDA.ul_fence_index = SVGAHDA.ul_flags_index + 5;
  SVGAHDA.ul_gmr_start   = SVGAHDA.ul_fence_index + 1;
  SVGAHDA.ul_gmr_count   = SVGA_ReadReg(SVGA_REG_GMR_MAX_IDS);
  SVGAHDA.ul_ctx_start   = SVGAHDA.ul_gmr_start + SVGAHDA.ul_gmr_count*3;
  SVGAHDA.ul_ctx_count   = GetDevCap(SVGA3D_DEVCAP_MAX_CONTEXT_IDS);
  SVGAHDA.ul_surf_start  = SVGAHDA.ul_ctx_start + SVGAHDA.ul_ctx_count;
  SVGAHDA.ul_surf_count  = GetDevCap(SVGA3D_DEVCAP_MAX_SURFACE_IDS);
	SVGAHDA.userlist_length = SVGAHDA.ul_surf_start + SVGAHDA.ul_surf_count;
	
	SVGAHDA.userlist_pm16  = drv_malloc(SVGAHDA.userlist_length * sizeof(uint32_t), &SVGAHDA.userlist_linear);
	
	if(SVGAHDA.userlist_pm16)
	{
		SVGAHDA.userlist_pm16[ULF_DIRTY] = 0xFFFFFFFFUL;
		
		/* zero the memory, because is large than 64k, is much easier do it in PM32 */
		if(VXD_load())
		{
			VXD_zeromem(SVGAHDA.userlist_linear, SVGAHDA.userlist_length * sizeof(uint32_t));
		}
		
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
 **/
LONG WINAPI __loadds Control(LPVOID lpDevice, UINT function,
  LPVOID lpInput, LPVOID lpOutput)
{
	LONG rc = -1;
	
  if(function == QUERYESCSUPPORT)
  {
  	WORD function_code = 0;
  	function_code =  *((LPWORD)lpInput);
  	switch(function_code)
  	{
  		case OPENGL_GETINFO:
  		case FBHDA_REQ:
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
  	}
  	//dbg_printf("Control: function check for: %x\n", function_code);
  }
  else if(function == OPENGL_GETINFO) /* input: NULL, output: opengl_icd_t */
  {
#ifdef SVGA  	
  	if(wMesa3DEnabled == 0)
  	{
  		_fmemcpy(lpOutput, &software_icd, OPENGL_ICD_SIZE); /* no 3D use software OPENGL */
  	}
  	else
  	{
  		_fmemcpy(lpOutput, &vmwsvga_icd, OPENGL_ICD_SIZE); /* accelerated OPENGL */
  	}
#else
    _fmemcpy(lpOutput, &software_icd, OPENGL_ICD_SIZE);
#endif
  	
  	rc = 1;
  }
  else if(function == FBHDA_REQ) /* input: NULL, output: uint32  */
  {
  	uint32_t __far *lpOut = lpOutput;
  	lpOut[0] = FBHDA_linear;
  	dbg_printf("FBHDA request: %lX\n", FBHDA_linear);
  	
  	rc = 1;
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
  	
  	dbg_printf("Region id = %ld\n", rid);	
  	
	  if(rid)
	  {
	  	uint32_t lAddr;
	  	uint32_t ppn;
	  	
	  	VXD_load();
	  	
	  	if(VXD_CreateRegion(lpIn[1], &lAddr, &ppn)) /* allocate physical memory */
	  	{
	  		dbg_printf("Region address = %lX, PPN = %lX\n", lAddr, ppn);
	  		
		    SVGA_WriteReg(SVGA_REG_GMR_ID, rid);
		    SVGA_WriteReg(SVGA_REG_GMR_DESCRIPTOR, ppn);
		    
		    /* refresh all register, so make sure that new commands will accepts this region */
		    SVGA_Flush();
		    
		    lpOut[0] = rid;
		    lpOut[1] = lAddr + 4096;
	  	}
	  	else
	  	{
	  		lpOut[0] = 0;
	  		lpOut[1] = 0;
	  	}
	  }
	  
	  rc = 1;
  }
  else if(function == SVGA_REGION_FREE) /* input: 2*uint32_t, output: NULL */
  {
  	uint32_t __far *lpin = lpInput;
    uint32_t id     = lpin[0];
    uint32_t linear = lpin[1] - 4096;
    
    /* flush all register inc. fifo so make sure, that all commands are processed */
    SVGA_Flush();
    
    SVGA_WriteReg(SVGA_REG_GMR_ID, id);
    SVGA_WriteReg(SVGA_REG_GMR_DESCRIPTOR, 0);
    
    /* sync again */
    SVGA_Flush();
    
    /* region physical delete */
    VXD_load();
    VXD_FreeRegion(linear);
    
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
  	
  	if(VXD_load())
    {
    	lpver[1] = VXD_apiver();
    }
    else
    {
    	lpver[1] = 0;
    }
    
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
