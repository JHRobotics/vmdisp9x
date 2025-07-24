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

#include "tvout.h" /* VIDEOPARAMETERS */

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

/* VIDEOPARAMETERS */
static VIDEOPARAMETERS_t tvsetup = {
  {0x02C62061UL, 0x1097, 0x11d1, 0x92, 0x0F, 0x00, 0xA0, 0x24, 0xDF, 0x15, 0x6E},
  0, // dwOffset;
  0, // dwCommand;
  0, // dwFlags;
  VP_MODE_WIN_GRAPHICS, // dwMode;
  VP_TV_STANDARD_WIN_VGA, // dwTVStandard;
  VP_MODE_WIN_GRAPHICS | VP_MODE_TV_PLAYBACK, // dwAvailableModes;
  VP_TV_STANDARD_NTSC_M | VP_TV_STANDARD_NTSC_M_J | VP_TV_STANDARD_PAL_B | VP_TV_STANDARD_PAL_D | VP_TV_STANDARD_PAL_H | VP_TV_STANDARD_PAL_I | VP_TV_STANDARD_PAL_M | VP_TV_STANDARD_PAL_N |
  VP_TV_STANDARD_SECAM_B | VP_TV_STANDARD_SECAM_D | VP_TV_STANDARD_SECAM_G | VP_TV_STANDARD_SECAM_H | VP_TV_STANDARD_SECAM_K | VP_TV_STANDARD_SECAM_K1 | VP_TV_STANDARD_SECAM_L |
  VP_TV_STANDARD_WIN_VGA | VP_TV_STANDARD_NTSC_433 | VP_TV_STANDARD_PAL_G | VP_TV_STANDARD_PAL_60 | VP_TV_STANDARD_SECAM_L1, // dwAvailableTVStandard;
  0, // dwFlickerFilter;
  0, // dwOverScanX;
  0, // dwOVerScanY;
  1000, // dwMaxUnscaledX;
  1000, // dwMaxUnscaledY;
  0, // dwPositionX;
  0, // dwPositionY;
  100, // dwBrightness;
  100, // dwContrast;
  0, // dwCPType;
  0, // dwCPCommand;
  VP_TV_STANDARD_NTSC_M | VP_TV_STANDARD_NTSC_M_J | VP_TV_STANDARD_PAL_B | VP_TV_STANDARD_PAL_D | VP_TV_STANDARD_PAL_H | VP_TV_STANDARD_PAL_I | VP_TV_STANDARD_PAL_M | VP_TV_STANDARD_PAL_N |
  VP_TV_STANDARD_SECAM_B | VP_TV_STANDARD_SECAM_D | VP_TV_STANDARD_SECAM_G | VP_TV_STANDARD_SECAM_H | VP_TV_STANDARD_SECAM_K | VP_TV_STANDARD_SECAM_K1 | VP_TV_STANDARD_SECAM_L |
  VP_TV_STANDARD_NTSC_433 | VP_TV_STANDARD_PAL_G | VP_TV_STANDARD_PAL_60 | VP_TV_STANDARD_SECAM_L1, // dwCPStandard;
  0, // dwCPKey;
  0, // bCP_APSTriggerBits
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
 * 2024-II:
 * added VIDEOPARAMETERS support, mostly for DVD programs that want somehow
 * control TV out (usually disabled it).
 *
 **/
LONG WINAPI __loadds Control(LPVOID lpDevice, UINT function,
  LPVOID lpInput, LPVOID lpOutput)
{
  LONG rc = -1;
  
  dbg_printf("Control (16bit): %d (0x%x)\n", function, function);
  
  switch(function)
  {
    case QUERYESCSUPPORT:
    {
      WORD function_code = 0;
      function_code =  *((LPWORD)lpInput);
      switch(function_code)
      {
        case OPENGL_GETINFO:
        case OP_FBHDA_SETUP:
        case MOUSETRAILS:
        case VIDEOPARAMETERS:
          rc = 1;
          break;
        case DCICOMMAND:
          rc = DD_HAL_VERSION;
          break;
        case QUERYESCSUPPORT:
        case QUERYDIBSUPPORT:
          rc = 1;
          break;
        default:
          dbg_printf("Control: function check for unknown: %x\n", function_code);
          break;
      }
      break;
    } /* QUERYESCSUPPORT */
    case OPENGL_GETINFO: /* input: NULL, output: opengl_icd_t */
    {
      if((hda->flags & FB_ACCEL_VMSVGA3D) && ((hda->flags & FB_FORCE_SOFTWARE) == 0))
      {
        _fmemcpy(lpOutput, &vmwsvga_icd, OPENGL_ICD_SIZE);
        rc = 1;
      }
      else if((hda->flags & FB_ACCEL_QEMU3DFX) && ((hda->flags & FB_FORCE_SOFTWARE) == 0))
      {
        _fmemcpy(lpOutput, &qemu3dfx_icd, OPENGL_ICD_SIZE);
        rc = 1;
      }
      else
      {
      	if(drv_is_p2())
      	{
        	_fmemcpy(lpOutput, &software_icd, OPENGL_ICD_SIZE);
        	rc = 1;
        }
      }
      break;
    } /* OPENGL_GETINFO */
    case DCICOMMAND: /* input ptr DCICMD */
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
              rc = 0;
              break;
          }
        }
#if 0
        else if(lpDCICMD->dwVersion == DCI_VERSION)
        {
          dbg_printf("DCI(%d) -> failed!\n", lpDCICMD->dwCommand);
          rc = 0;
        }
        else
        {
          dbg_printf("hal required: %lX\n", lpDCICMD->dwVersion);
          rc = 1;
        }
#else
				else
				{
					/* JH: this come from S3 driver, we need to do this or DDRAW not work on emulators*
						(probably wrong zero page exception handling)

						*emulators = QEMU without KVM/HyperV, DOSBox
					*/
					return DIB_Control(lpDevice, function, lpInput, lpOutput);
				}
#endif
      }
      break;
    } /* DCICOMMAND */
    case MOUSETRAILS:
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
      break;
    } /* MOUSETRAILS */
    case OP_FBHDA_SETUP: /* input: NULL, output: uint32  */
    {
      uint32_t __far *lpOut = lpOutput;
      lpOut[0] = hda_linear;
      dbg_printf("FBHDA request: %lX\n", hda_linear);
      
      rc = 1;
      break;
    } /* OP_FBHDA_SETUP */
    case VIDEOPARAMETERS:
    {
      VIDEOPARAMETERS_t __far *vps = ((VIDEOPARAMETERS_t __far *)lpInput);
      rc = 0;
      
      if(vps->dwCommand == VP_COMMAND_GET)
      {
        DWORD checkMask = vps->dwFlags & (VP_FLAGS_TV_MODE | VP_FLAGS_TV_STANDARD | VP_FLAGS_FLICKER | VP_FLAGS_OVERSCAN | VP_FLAGS_MAX_UNSCALED |
        VP_FLAGS_POSITION | VP_FLAGS_BRIGHTNESS | VP_FLAGS_CONTRAST | VP_FLAGS_COPYPROTECT);
        
        if(checkMask == vps->dwFlags)
        {
          if(vps->dwFlags & VP_FLAGS_TV_MODE)
          {
            vps->dwMode = tvsetup.dwMode;
            vps->dwAvailableModes = tvsetup.dwAvailableModes;
          }
          
          if(vps->dwFlags & VP_FLAGS_TV_STANDARD)
          {
            vps->dwTVStandard = tvsetup.dwTVStandard;
            vps->dwAvailableTVStandard = tvsetup.dwAvailableTVStandard;
          }
          
          if(vps->dwFlags & VP_FLAGS_FLICKER)
            vps->dwFlickerFilter = tvsetup.dwFlickerFilter;
          
          if(vps->dwFlags & VP_FLAGS_OVERSCAN)
          {
            vps->dwOverScanX = tvsetup.dwOverScanX;
            vps->dwOverScanY = tvsetup.dwOverScanY;
          }
          
          if(vps->dwFlags & VP_FLAGS_MAX_UNSCALED)
          {
            vps->dwMaxUnscaledX = tvsetup.dwMaxUnscaledX;
            vps->dwMaxUnscaledY = tvsetup.dwMaxUnscaledY;
          }
          
          if(vps->dwFlags & VP_FLAGS_POSITION)
          {
            vps->dwPositionX = tvsetup.dwPositionX;
            vps->dwPositionY = tvsetup.dwPositionY;
          }
          
          if(vps->dwFlags & VP_FLAGS_BRIGHTNESS)
            vps->dwBrightness = tvsetup.dwBrightness;
          
          if(vps->dwFlags & VP_FLAGS_CONTRAST)
            vps->dwContrast = tvsetup.dwContrast;
          
          
          if(vps->dwFlags & VP_FLAGS_COPYPROTECT)
          {
            vps->dwCPType = tvsetup.dwCPType;
            vps->dwCPStandard = tvsetup.dwCPStandard;
            vps->bCP_APSTriggerBits = tvsetup.bCP_APSTriggerBits;
          }
          
          rc = 1;
        }
      }
      else if(vps->dwCommand == VP_COMMAND_SET)
      {
        rc = 1;
        
        if(vps->dwFlags & VP_FLAGS_TV_MODE)
          tvsetup.dwMode = vps->dwMode;
          
        if(vps->dwFlags & VP_FLAGS_TV_STANDARD)
          tvsetup.dwTVStandard = vps->dwTVStandard;
  
         if(vps->dwFlags & VP_FLAGS_FLICKER)
           tvsetup.dwFlickerFilter = vps->dwFlickerFilter;
          
        if(vps->dwFlags & VP_FLAGS_OVERSCAN)
        {
          tvsetup.dwOverScanX = vps->dwOverScanX;
          tvsetup.dwOverScanY = vps->dwOverScanY;
        }
  
         if(vps->dwFlags & VP_FLAGS_POSITION)
         {
           tvsetup.dwPositionX = vps->dwPositionX;
           tvsetup.dwPositionY = vps->dwPositionY;
         }
  
         if(vps->dwFlags & VP_FLAGS_BRIGHTNESS)
          tvsetup.dwBrightness = vps->dwBrightness;
          
        if(vps->dwFlags & VP_FLAGS_CONTRAST)
          tvsetup.dwContrast = vps->dwContrast;
  
         if(vps->dwFlags & VP_FLAGS_COPYPROTECT)
         {
           switch(vps->dwCPCommand)
           {
             case VP_CP_CMD_ACTIVATE:
               tvsetup.dwCPCommand = VP_CP_CMD_ACTIVATE;
               tvsetup.dwCPType    = vps->dwCPType;
               tvsetup.dwCPKey     = vps->dwCPKey;
               tvsetup.bCP_APSTriggerBits = vps->bCP_APSTriggerBits;
               break;
             case VP_CP_CMD_DEACTIVATE:
             case VP_CP_CMD_CHANGE:
               if(tvsetup.dwCPCommand == VP_CP_CMD_ACTIVATE)
               {
                 if(tvsetup.dwCPKey != vps->dwCPKey)
                 {
                   rc = -1;
                 }
                 else
                 {
                   tvsetup.dwCPCommand = tvsetup.dwCPCommand;
                 }
               }
               break;
             default:
               rc = 0;
               break;
           }
         }
      }
    
      if(rc < 0) /* return RC and don't pass to DIB */
      {
        return rc;
      }
      
      break;
    } /* VIDEOPARAMETERS */
    case QUERYDIBSUPPORT:
    {
      dbg_printf("QUERYDIBSUPPORT -> DIB\n");
      return DIB_Control(lpDevice, function, lpInput, lpOutput);
      break;
    } /* QUERYDIBSUPPORT */
  } /* switch */
 
#if 0
  /* if command accepted, return */
  if(rc >= 0)
  {
    return rc;
  }
  
  /* else, call DIB_Control handle */
  dbg_printf("Control: unknown code: %d\n", function);
  return DIB_Control(lpDevice, function, lpInput, lpOutput);
#else
# if DBGPRINT
  if(rc < 0)
  {
    dbg_printf("Control: unknown code: %d\n", function);
  }
# endif
  /* JH: it's probably not good idea to pass unknown commands directly to DIB  */
  return rc;
#endif
}
