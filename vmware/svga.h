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
   uint32 __far *fifoMem; // 8
   uint8  __far *fbMem;   // 16
   uint32     fifoSize;   // 20
   
   uint32     fifoLinear; // 24
   uint16     fifoSel;    // 28
   uint16     fifoAct;    // 32
   
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
      Bool    usingBounceBuffer;
      uint8   bounceBuffer[16 * 1024];
      uint32  nextFence;
   } fifo;
  
   /* VMWare SVGAII and VBox SVGA is same device with different PCI ID */
   uint16 vendorId;
   uint16 deviceId;

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

void __far *SVGA_FIFOReserve(uint32 bytes);
void __far *SVGA_FIFOReserveCmd(uint32 type, uint32 bytes);
void __far *SVGA_FIFOReserveEscape(uint32 nsid, uint32 bytes);
void SVGA_FIFOCommit(uint32 bytes);
void SVGA_FIFOCommitAll(void);

uint32 SVGA_InsertFence(void);
void SVGA_SyncToFence(uint32 fence);
Bool SVGA_HasFencePassed(uint32 fence);
void SVGA_RingDoorbell(void);

void __far *SVGA_AllocGMR(uint32 size, SVGAGuestPtr __far *ptr);

/* 2D commands */

void SVGA_Update(uint32 x, uint32 y, uint32 width, uint32 height);
void SVGA_BeginDefineCursor(const SVGAFifoCmdDefineCursor __far *cursorInfo,
                            void __far * __far *andMask, void __far * __far *xorMask);
void SVGA_BeginDefineAlphaCursor(const SVGAFifoCmdDefineAlphaCursor __far *cursorInfo,
                                 void __far * __far *data);
void SVGA_MoveCursor(uint32 visible, uint32 x, uint32 y, uint32 screenId);

void SVGA_BeginVideoSetRegs(uint32 streamId, uint32 numItems,
                            SVGAEscapeVideoSetRegs __far * __far *setRegs);
void SVGA_VideoSetAllRegs(uint32 streamId, SVGAOverlayUnit __far *regs, uint32 maxReg);
void SVGA_VideoSetReg(uint32 streamId, uint32 registerId, uint32 value);
void SVGA_VideoFlush(uint32 streamId);


#endif /* __SVGA_H__ */
