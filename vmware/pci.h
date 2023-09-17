/* -*- Mode: C; c-basic-offset: 3 -*-
 *
 * pci.h - Simple PCI configuration interface. This implementation
 *         only supports type 1 accesses to configuration space,
 *         and it ignores the PCI BIOS.
 *
 * This file is part of Metalkit, a simple collection of modules for
 * writing software that runs on the bare metal. Get the latest code
 * at http://svn.navi.cx/misc/trunk/metalkit/
 *
 * Copyright (c) 2008-2009 Micah Dowty
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef __PCI_H__
#define __PCI_H__

#include "types16.h"

#pragma pack(push)
#pragma pack(1)
typedef union PCIConfigSpace {
   uint32 words[16];
   struct {                    // offset
      uint16 vendorId;         //  0
      uint16 deviceId;         //  2
      uint16 command;          //  4
      uint16 status;           //  6
      uint16 revisionId;       //  8
      uint8  subclass;         // 10
      uint8  classCode;        // 11
      uint8  cacheLineSize;    // 12
      uint8  latTimer;         // 13
      uint8  headerType;       // 14
      uint8  BIST;             // 15
      uint32 BAR[6];           // 16
      uint32 cardbusCIS;       // 40
      uint16 subsysVendorId;   // 44
      uint16 subsysId;         // 46
      uint32 expansionRomAddr; // 48
      uint32 reserved0;        // 52
      uint32 reserved1;        // 56
      uint8  intrLine;         // 60
      uint8  intrPin;          // 61
      uint8  minGrant;         // 62
      uint8  maxLatency;       // 63
   };
} PCIConfigSpace;

typedef struct PCIAddress {
   uint8 bus, device, function, padding;
} PCIAddress;

typedef struct PCIScanState {
   uint16     vendorId;
   uint16     deviceId;
   PCIAddress nextAddr;
   PCIAddress addr;
} PCIScanState;

#pragma pack(pop)

// BAR bits
#define PCI_CONF_BAR_IO          0x01
#define PCI_CONF_BAR_64BIT       0x04
#define PCI_CONF_BAR_PREFETCH    0x08

uint32 PCI_ConfigRead32(const PCIAddress FARP *addr, uint16 offset);
uint16 PCI_ConfigRead16(const PCIAddress FARP *addr, uint16 offset);
uint8 PCI_ConfigRead8(const PCIAddress FARP *addr, uint16 offset);
void PCI_ConfigWrite32(const PCIAddress FARP *addr, uint16 offset, uint32 data);
void PCI_ConfigWrite16(const PCIAddress FARP *addr, uint16 offset, uint16 data);
void PCI_ConfigWrite8(const PCIAddress FARP *addr, uint16 offset, uint8 data);

Bool PCI_ScanBus(PCIScanState  FARP *state);
Bool PCI_FindDevice(uint16 vendorId, uint16 deviceId, PCIAddress FARP *addrOut);
void PCI_SetBAR(const PCIAddress FARP *addr, int index, uint32 value);
uint32 PCI_GetBARAddr(const PCIAddress FARP *addr, int index);
uint32 PCI_GetBARSize(const PCIAddress FARP *addr, int index);
void PCI_SetMemEnable(const PCIAddress FARP *addr, Bool enable);
uint32 PCI_GetSubsystem(const PCIAddress FARP *addr);

#endif /* __PCI_H__ */
