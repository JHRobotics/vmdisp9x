/**********************************************************
 * Copyright 2008-2009 VMware, Inc.  All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 **********************************************************/

/*
 * svga.h --
 *
 *      This is a simple example driver for the VMware SVGA device.
 *      It handles initialization, register accesses, low-level
 *      command FIFO writes, and host/guest synchronization.
 */

#ifndef __SVGA_H__
#define __SVGA_H__

#include "types16.h"
#include "pci.h"
//#include "intr.h"

#define REALLY_TINY

// XXX: Shouldn't have to do this here.
//#define INLINE __inline__
#define INLINE inline

#include "svga_reg.h"
#include "svga_escape.h"
#include "svga_overlay.h"
#include "svga3d_reg.h"

typedef struct SVGADevice {
   PCIAddress pciAddr;    // 0
   uint32     ioBase;     // 4
   uint32 FARP *fifoMem;  // 8
   uint8  FARP *fbMem;    // 12
   uint32     fifoSize;   // 16
   
   uint32     fifoLinear; // 20
   uint16     fifoSel;    // 22
   uint16     fifoAct;    // 24
   
   uint32     fifoPhy;
   
   uint32     fbSize;
   uint32     fbLinear;
   uint32     fbPhy;
   uint32     vramSize;
   
   uint32     deviceVersionId;
   uint32     capabilities;

   uint32     width;
   uint32     height;
   uint32     bpp;
   uint32     pitch;

   struct {
      uint32  reservedSize;
#if 0
      Bool    usingBounceBuffer;
      uint8   bounceBuffer[16 * 1024];
#else
      uint32  bouncePhy;
      uint32  bounceLinear;
      uint8 FARP *bounceMem;
#endif
      uint32  nextFence;
   } fifo;
  
   /* VMWare SVGAII and VBox SVGA is same device with different PCI ID */
   uint16 vendorId;
   uint16 deviceId;
   /* adapter in QEMU works only on 32bit */
   uint32 userFlags;

#ifndef REALLY_TINY
   volatile struct {
      uint32        pending;
      uint32        switchContext;
      IntrContext   oldContext;
      IntrContext   newContext;
      uint32        count;
   } irq;
#endif

} SVGADevice;

#define SVGA_BOUNCE_SIZE (16*1024)

/* SVGA working only on 32bpp modes */
#define SVGA_USER_FLAGS_32BITONLY 1

/* SVGA using HW cursor */
#define SVGA_USER_FLAGS_HWCURSOR  2

/* SVGA using alpha channel cursor */
#define SVGA_USER_FLAGS_ALPHA_CUR 4

/* SVGA surface 565 is broken (VBox bug) */
#define SVGA_USER_FLAGS_RGB565_BROKEN 8

/* limit SVGA VRAM to 128 MB */
#define SVGA_USER_FLAGS_128MB_MAX 16

extern SVGADevice gSVGA;

#ifndef VXD32
int  __loadds SVGA_Init(Bool enableFIFO);
#else
int SVGA_Init(Bool enableFIFO);
#endif
void SVGA_Enable(void);
void SVGA_SetMode(uint32 width, uint32 height, uint32 bpp);
void SVGA_Disable(void);
void SVGA_Panic(const char *err);
void SVGA_DefaultFaultHandler(int vector);

void SVGA_Flush(void);

uint32 SVGA_ReadReg(uint32 index);
void SVGA_WriteReg(uint32 index, uint32 value);
uint32 SVGA_ClearIRQ(void);
uint32 SVGA_WaitForIRQ();

Bool SVGA_IsFIFORegValid(int reg);
Bool SVGA_HasFIFOCap(unsigned long cap);

void FARP *SVGA_FIFOReserve(uint32 bytes);
void FARP *SVGA_FIFOReserveCmd(uint32 type, uint32 bytes);
void FARP *SVGA_FIFOReserveEscape(uint32 nsid, uint32 bytes);
void SVGA_FIFOCommit(uint32 bytes);
void SVGA_FIFOCommitAll(void);

uint32 SVGA_InsertFence(void);
void SVGA_SyncToFence(uint32 fence);
Bool SVGA_HasFencePassed(uint32 fence);
void SVGA_RingDoorbell(void);

void FARP *SVGA_AllocGMR(uint32 size, SVGAGuestPtr FARP *ptr);

/* 2D commands */

void SVGA_Update(uint32 x, uint32 y, uint32 width, uint32 height);
void SVGA_BeginDefineCursor(const SVGAFifoCmdDefineCursor FARP *cursorInfo,
                            void FARP * FARP *andMask, void FARP * FARP *xorMask);
void SVGA_BeginDefineAlphaCursor(const SVGAFifoCmdDefineAlphaCursor FARP *cursorInfo,
                                 void FARP * FARP *data);
void SVGA_MoveCursor(uint32 visible, uint32 x, uint32 y, uint32 screenId);

void SVGA_BeginVideoSetRegs(uint32 streamId, uint32 numItems,
                            SVGAEscapeVideoSetRegs FARP * FARP *setRegs);
void SVGA_VideoSetAllRegs(uint32 streamId, SVGAOverlayUnit FARP *regs, uint32 maxReg);
void SVGA_VideoSetReg(uint32 streamId, uint32 registerId, uint32 value);
void SVGA_VideoFlush(uint32 streamId);


#endif /* __SVGA_H__ */
