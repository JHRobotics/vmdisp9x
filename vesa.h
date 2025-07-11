/*****************************************************************************

Copyright (c) 2025 Jaroslav Hensl <emulator@emulace.cz>

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

#ifndef __VESA_H__INCLUDED__
#define __VESA_H__INCLUDED__

#include "types16.h"

/* https://wiki.osdev.org/VESA_Video_Modes */

#pragma pack(push)
#pragma pack(1)

/* VBE 3.0 specification, p.25 */

typedef struct vesa_info_block
{
	uint8  VESASignature[4];    /* "VESA" 4 byte signature */
	uint16 VESAVersion;         /* VBE version number */
	uint32 OEMStringPtr;        /* Pointer to OEM string */
	uint8  Capabilities[4];     /* Capabilities of video card */
	uint32 VideoModePtr;        /* Pointer to supported modes */
	uint16 TotalMemory;         /* Number of 64kb memory blocks */
	uint16 OemSoftwareRev;      /* VBE implementation Software revision */
	uint32 OemVendorNamePtr;    /* VbeFarPtr to Vendor Name String */
	uint32 OemProductNamePtr;   /* VbeFarPtr to Product Name String */
	uint32 OemProductRevPtr;    /* VbeFarPtr to Product Revision String */
	uint8  Reserved[222];       /* Pad to 256 byte block size */
	uint8  OemData[256];        /* Data Area for OEM Strings */
} vesa_info_block_t;

/* p.30 */

typedef struct vesa_mode_info
{
	uint16 ModeAttributes;      /* mode attributes */
	uint8  WinAAttributes;      /* window A attributes */
	uint8  WinBAttributes;      /* window B attributes */
	uint16 WinGranularity;      /* window granularity */
	uint16 WinSize;             /* window size */
	uint16 WinASegment;         /* window A start segment */
	uint16 WinBSegment;         /* window B start segment */
	uint32 WinFuncPtr;		      /* real mode pointer to window function */
	uint16 BytesPerScanLine;    /* bytes per scan line (pitch) */
	/* VBE 1.2 */
	uint16 XResolution;         /* horizontal resolution in pixels or characters (width) */
	uint16 YResolution;         /* vertical resolution in pixels or characters (height) */
	uint8 XCharSize;            /* character cell width in pixels */
	uint8 YCharSize;            /* character cell height in pixels */
	uint8 NumberOfPlanes;       /* number of memory planes */
	uint8 BitsPerPixel;         /* bits per pixel (bpp) */
	uint8 NumberOfBanks;        /* number of banks */
	uint8 MemoryModel;          /* memory model type */
	uint8 BankSize;             /* bank size in KB */
	uint8 NumberOfImagePages;   /* number of images */
	uint8 Reserved0;            /* reserved for page function */
  /* Direct Color fields (required for direct/6 and YUV/7 memory models) */
	uint8 RedMaskSize;          /* size of direct color red mask in bits */
	uint8 RedFieldPosition;     /* bit position of lsb of red mask */
	uint8 GreenMaskSize;        /* size of direct color green mask in bits */
	uint8 GreenFieldPosition;   /* bit position of lsb of green mask */
	uint8 BlueMaskSize;         /* size of direct color blue mask in bits */
	uint8 BlueFieldPosition;    /* bit position of lsb of blue mask */
	uint8 RsvdMaskSize;         /* size of direct color reserved mask in bits */
	uint8 RsvdFieldPosition;    /* bit position of lsb of reserved mask  */
	uint8 DirectColorModeInfo;  /* direct color mode attributes */
	/* VBE 2.0 */
	uint32 PhysBasePtr;         /* physical address for flat memory frame buffer */
	uint32 Reserved1;           /* Reserved - always set to 0 */
	uint16 Reserved2;	          /* Reserved - always set to 0 */
	/* VBE 3.0 */
	uint16 LinBytesPerScanLine;    /* bytes per scan line for linear modes */
	uint8  BnkNumberOfImagePages;  /* number of images for banked modes */
	uint8  LinNumberOfImagePages;  /* number of images for linear modes */
	uint8  LinRedMaskSize;         /* size of direct color red mask (linear modes) */
	uint8  LinRedFieldPosition;    /* bit position of lsb of red mask (linear modes) */
	uint8  LinGreenMaskSize;       /* size of direct color green mask (linear modes) */
	uint8  LinGreenFieldPosition;  /* bit position of lsb of green mask (linear modes) */
	uint8  LinBlueMaskSize;        /* size of direct color blue mask (linear modes) */
	uint8  LinBlueFieldPosition;   /* bit position of lsb of blue mask (linear modes) */
	uint8  LinRsvdMaskSize;        /* size of direct color reserved mask (linear modes) */
	uint8  LinRsvdFieldPosition;   /* bit position of lsb of reserved mask (linear modes) */
	uint32 MaxPixelClock;          /* maximum pixel clock (in Hz) for graphics mode */
	uint8  Reserved3[189];         /* remainder of ModeInfoBlock */
} vesa_mode_info_t;

#define VESA_MODE_HW_SUPPORTED       0x0001
#define VESA_MODE_TTY_OUTPUT         0x0004
#define VESA_MODE_COLOR              0x0008
#define VESA_MODE_GRAPHICS           0x0010
#define VESA_MODE_VGA_COMPACT        0x0020
#define VESA_MODE_WINDOWED           0x0040
#define VESA_MODE_LINEAR_FRAMEBUFFER 0x0080
#define VESA_MODE_DOUBLE_SCAN        0x0100
#define VESA_MODE_INTERLACED         0x0200
#define VESA_MODE_TRIPLE_BUFFERING   0x0400
#define VESA_MODE_STEREOSCOPIC       0x0800
#define VESA_MODE_DUAL_DISPLAY       0x1000

/* p.41 */

typedef struct vesa_crtc_info
{
	uint16 HorizontalTotal;        /* Horizontal total in pixels */
	uint16 HorizontalSyncStart;    /* Horizontal sync start in pixels */
	uint16 HorizontalSyncEnd;      /* Horizontal sync end in pixels */
	uint16 VerticalTotal;          /* Vertical total in lines */
	uint16 VerticalSyncStart;      /* Vertical sync start in lines */
	uint16 VerticalSyncEnd;        /* Vertical sync end in lines */
	uint8  Flags;                  /* Flags (Interlaced, Double Scan etc) */
	uint32 PixelClock;             /* Pixel clock in units of Hz */
	uint16 RefreshRate;            /* Refresh rate in units of 0.01 Hz */
	uint8  Reserved[40];           /* remainder of ModeInfoBlock */
} vesa_crtc_info_t;

#pragma pack(pop)

#endif /* __VESA_H__INCLUDED__ */
