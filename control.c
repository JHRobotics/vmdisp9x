/*****************************************************************************

Copyright (c) 2022-2024 Jaroslav Hensl <emulator@emulace.cz>

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
#include "3d_accel.h"

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

/* Mesa3D SVGA3D OpengGL */
const static opengl_icd_t qemu3dfx_icd = {
	0x2,
	0x1,
	"QEMUFX"
};

/* Mesa3D SVGA3D OpengGL */
const static opengl_icd_t vmwsvga_icd = {
	0x2,
	0x1,
	"VMWSVGA"
};

#pragma code_seg( _INIT )

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
 * 2024:
 * removed all VMWare SVGA II commands from here and move to VXD driver
 * you can use DeviceIoControl to call them
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
  		case OP_FBHDA_SETUP:
  		case MOUSETRAILS:
  			rc = 1;
  			break;
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
  	if((hda->flags & FB_ACCEL_VMSVGA3D) && ((hda->flags & FB_FORCE_SOFTWARE) == 0))
  	{
  		_fmemcpy(lpOutput, &vmwsvga_icd, OPENGL_ICD_SIZE);
  	}
  	else if((hda->flags & FB_ACCEL_QEMU3DFX) && ((hda->flags & FB_FORCE_SOFTWARE) == 0))
  	{
  		_fmemcpy(lpOutput, &qemu3dfx_icd, OPENGL_ICD_SIZE);
  	}
  	else
  	{
  		_fmemcpy(lpOutput, &software_icd, OPENGL_ICD_SIZE);
  	}
  	
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
#ifdef DBGPRINT
  	if(lpInput)
  	{
  		int trails = *((int __far *)lpInput);
  		dbg_printf("MOUSETRAILS: %d\n", trails);
  	}
#endif
  	//JH: mouse trails are disabled!
  	//DIB_Control(lpDevice, MOUSETRAILS, lpInput, lpOutput);
  	rc = 1;
  }
  else if(function == OP_FBHDA_SETUP) /* input: NULL, output: uint32  */
  {
  	uint32_t __far *lpOut = lpOutput;
  	lpOut[0] = hda_linear;
  	dbg_printf("FBHDA request: %lX\n", hda_linear);
  	
  	rc = 1;
  }
  
  /* if command accepted, return */
  if(rc >= 0)
  {
  	return rc;
  }
  
  /* else, call DIB_Control handle */
  dbg_printf("Control: unknown code: %d\n", function);
  return DIB_Control(lpDevice, function, lpInput, lpOutput);
}
