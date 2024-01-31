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
 * svga.c --
 *
 *      This is a simple example driver for the VMware SVGA device.
 *      It handles initialization, register accesses, low-level
 *      command FIFO writes, and host/guest synchronization.
 */

//#include <string.h> /* include some basics like NULL */
#include "winhack.h"

#include "svga_all.h"

#ifdef VXD32
#include "code32.h"
#endif

#ifdef VXD32
#define IO_IN32
#define IO_OUT32
#include "io32.h"
#else
#include "io.h"
#endif

#include "drvlib.h"

#ifndef VXD32
#include "dpmi.h"
#endif

#ifdef DBGPRINT
void dbg_printf( const char *s, ... );
#else
#define dbg_printf(...)
#endif

#ifndef MIN
#define MIN(a, b) (((a) > (b)) ? (b) : (a))
#endif

#ifndef MAX
#define MAX(a, b) (((b) > (a)) ? (b) : (a))
#endif

SVGADevice gSVGA = {0};

static void SVGAFIFOFull(void);

#ifndef REALLY_TINY
static void SVGAInterruptHandler(int vector);
#endif

#ifdef REALLY_TINY
# ifdef VXD32
#  define Console_Panic(x)
#  define SVGA_Panic(x)
# else
#  define Console_Panic(x) dbg_printf("PANIC: %s\n", x)
#  define SVGA_Panic(x) dbg_printf("PANIC: %s\n", x)
# endif
#endif

#ifndef VXD32
#pragma code_seg( _INIT )
#endif

#ifdef DBGPRINT
static char dbg_addr[] = "PCI bars: %lx %lx %lx\n";
static char dbg_siz[] = "Size of gSVGA(1) = %d %d\n";
#endif

/*
 *-----------------------------------------------------------------------------
 *
 * SVGA_Init --
 *
 *      Initialize the global SVGA device. This locates it on the PCI bus,
 *      negotiates device version, and maps the command FIFO and framebuffer
 *      memory.
 *
 *      Intr_Init() must have already been called. If the SVGA device
 *      supports interrupts, this will initalize them.
 *
 *      Does not switch video modes.
 *
 * Results:
 *      void.
 *
 * Side effects:
 *      Steals various IOspace and memory regions.
 *      In this example code they're constant addresses, but in reality
 *      you'll need to negotiate these with the operating system.
 *
 *-----------------------------------------------------------------------------
 */

int
#ifndef VXD32
SVGA_Init(Bool enableFIFO)
#else
SVGA_Init(Bool enableFIFO, uint32 hwversion)
#endif
{
	 int vga_found = 0;
	
   if (PCI_FindDevice(PCI_VENDOR_ID_VMWARE, PCI_DEVICE_ID_VMWARE_SVGA3, &gSVGA.pciAddr)) {
      gSVGA.vendorId = PCI_VENDOR_ID_VMWARE;
      gSVGA.deviceId = PCI_DEVICE_ID_VMWARE_SVGA3;
#ifndef VXD32
      dbg_printf("PCI VMWare SVGA-III: %X:%X\n", gSVGA.vendorId, gSVGA.deviceId);
#endif
      
      vga_found = 1;
   }
	
   if (PCI_FindDevice(PCI_VENDOR_ID_VMWARE, PCI_DEVICE_ID_VMWARE_SVGA2, &gSVGA.pciAddr)) {
      gSVGA.vendorId = PCI_VENDOR_ID_VMWARE;
      gSVGA.deviceId = PCI_DEVICE_ID_VMWARE_SVGA2;
#ifndef VXD32
      dbg_printf("PCI VMWare SVGA-II: %X:%X\n", gSVGA.vendorId, gSVGA.deviceId);
#endif
      
      vga_found = 1;
   }
   
   if (PCI_FindDevice(PCI_VENDOR_ID_INNOTEK, PCI_DEVICE_ID_VBOX_VGA, &gSVGA.pciAddr)) {
   	  uint32 subsys = PCI_GetSubsystem(&gSVGA.pciAddr);
      if(subsys == PCI_SUBCLASS_ID_SVGA2)
      {
         gSVGA.vendorId = PCI_VENDOR_ID_INNOTEK;
         gSVGA.deviceId = PCI_DEVICE_ID_VBOX_VGA;
#ifndef VXD32
         dbg_printf("PCI VBox SVGA: %X:%X\n", gSVGA.vendorId, gSVGA.deviceId);
#endif
         vga_found = 1;
      }
      else
      {
#ifndef VXD32
      	dbg_printf("PCI VBox VGA, subclass %lX = this driver is not for it!\n", subsys);
#endif
      }
   }
   
   if(vga_found == 0)
   {
      Console_Panic("No VMware SVGA device found.");
      return 1;
   }
   
   /*
    * Use the default base address for each memory region.
    * We must map at least ioBase before using ReadReg/WriteReg.
    */

   PCI_SetMemEnable(&gSVGA.pciAddr, TRUE);
   
   if(gSVGA.deviceId == PCI_DEVICE_ID_VBOX_VGA) { /* VBOX SVGA */
   	  /* VBox SVGA have swapped IO and VRAM in PCI BAR */
      gSVGA.fbPhy  = PCI_GetBARAddr(&gSVGA.pciAddr, 0);
      gSVGA.vramSize = PCI_GetBARSize(&gSVGA.pciAddr, 0);
      gSVGA.ioBase = PCI_GetBARAddr(&gSVGA.pciAddr, 1);
      gSVGA.fifoPhy = PCI_GetBARAddr(&gSVGA.pciAddr, 2);
      
   } else {
   	  if(SVGA_IsSVGA3()) /* VMware SVGA3 */
   	  {
      	gSVGA.rmmio_start = PCI_GetBARAddr(&gSVGA.pciAddr, 0);
      	gSVGA.rmmio_size  = PCI_GetBARSize(&gSVGA.pciAddr, 0);
   	  	
      	gSVGA.fbPhy  = PCI_GetBARAddr(&gSVGA.pciAddr, 2);
      	gSVGA.vramSize = PCI_GetBARSize(&gSVGA.pciAddr, 2);
      	
      	gSVGA.fifoPhy = 0;
      	gSVGA.ioBase = 0;
   	  }
   	  else /* VMware SVGA2 */
   	  {
      	gSVGA.ioBase = PCI_GetBARAddr(&gSVGA.pciAddr, 0);
      	
      	gSVGA.fbPhy  = PCI_GetBARAddr(&gSVGA.pciAddr, 1);
      	gSVGA.vramSize = PCI_GetBARSize(&gSVGA.pciAddr, 1);
      	
      	gSVGA.fifoPhy = PCI_GetBARAddr(&gSVGA.pciAddr, 2);
      }
   }
   
   
   dbg_printf(dbg_addr, gSVGA.ioBase, gSVGA.fbPhy, gSVGA.fifoPhy);
   SVGA_MapIO();

#ifdef VXD32 /* version negotiation is done in PM32 driver now */
   /*
    * Version negotiation:
    *
    *   1. Write to SVGA_REG_ID the maximum ID supported by this driver.
    *   2. Read from SVGA_REG_ID
    *      a. If we read back the same value, this ID is supported. We're done.
    *      b. If not, decrement the ID and repeat.
    */
    
   if(hwversion == 0)
   {
   		if(gSVGA.deviceId == PCI_DEVICE_ID_VMWARE_SVGA3)
   		{
   			hwversion = SVGA_VERSION_3;
   		}
      else
      {
      	hwversion = SVGA_VERSION_2;
      }
   }

   gSVGA.deviceVersionId = SVGA_MAKE_ID(hwversion);
   do {
      SVGA_WriteReg(SVGA_REG_ID, gSVGA.deviceVersionId);
      if (SVGA_ReadReg(SVGA_REG_ID) == gSVGA.deviceVersionId) {
         break;
      } else {
         gSVGA.deviceVersionId--;
      }
   } while (gSVGA.deviceVersionId >= SVGA_ID_0);

   if (gSVGA.deviceVersionId < SVGA_ID_0) {
#if 0
      Console_Panic("Error negotiating SVGA device version.");
#endif
      return 2;
   }
#else /* !VXD32 */
   /* version is already set by VXD driver - only read register with ID */
   gSVGA.deviceVersionId = SVGA_ReadReg(SVGA_REG_ID);
#endif

   /*
    * We must determine the FIFO and FB size after version
    * negotiation, since the default version (SVGA_ID_0)
    * does not support the FIFO buffer at all.
    */

/*
   gSVGA.vramSize = SVGA_ReadReg(SVGA_REG_VRAM_SIZE);
   gSVGA.fbSize = SVGA_ReadReg(SVGA_REG_FB_SIZE);
*/
   gSVGA.fifoSize = SVGA_ReadReg(SVGA_REG_MEM_SIZE);
   gSVGA.fbSize = SVGA_ReadReg(SVGA_REG_FB_SIZE);

   /*
    * Sanity-check the FIFO and framebuffer sizes.
    * These are arbitrary values.
    */

#ifdef VXD32
   if (gSVGA.fbSize < 0x100000) {
      SVGA_Panic("FB size very small, probably incorrect.");
      return 4;
   }
#endif

   if(SVGA_IsSVGA3())
   {
      if (gSVGA.fifoSize < 0x20000) {
         SVGA_Panic("FIFO size very small, probably incorrect.");
         return 5;
      }
   }

   /*
    * If the device is new enough to support capability flags, get the
    * capabilities register.
    */

   if (gSVGA.deviceVersionId >= SVGA_ID_1) {
      gSVGA.capabilities = SVGA_ReadReg(SVGA_REG_CAPABILITIES);
   }

   /*
    * Optional interrupt initialization.
    *
    * This uses the default IRQ that was assigned to our
    * device by the BIOS.
    */

#ifndef REALLY_TINY
   if (gSVGA.capabilities & SVGA_CAP_IRQMASK) {
      uint8 irq = PCI_ConfigRead8(&gSVGA.pciAddr, offsetof(PCIConfigSpace, intrLine));

      /* Start out with all SVGA IRQs masked */
      SVGA_WriteReg(SVGA_REG_IRQMASK, 0);

      /* Clear all pending IRQs stored by the device */
      IO_Out32(gSVGA.ioBase + SVGA_IRQSTATUS_PORT, 0xFF);

      /* Clear all pending IRQs stored by us */
      SVGA_ClearIRQ();

      /* Enable the IRQ */
      Intr_SetHandler(IRQ_VECTOR(irq), SVGAInterruptHandler);
      Intr_SetMask(irq, TRUE);
   }
#endif
   /*gSVGA.fifoLinear = 0;
   gSVGA.fifoSel    = 0;
   gSGA.fifoAct    = 0;*/
   
#ifdef VXD32
	gSVGA.userFlags = 0;

   /* check if SVGA supporting different BPP then 32 */
   SVGA_WriteReg(SVGA_REG_ENABLE, TRUE);
   SVGA_WriteReg(SVGA_REG_CONFIG_DONE, TRUE);
   
   SVGA_WriteReg(SVGA_REG_BITS_PER_PIXEL, 8);
   if(SVGA_ReadReg(SVGA_REG_CONFIG_DONE) == 0)
   {
      gSVGA.userFlags |= SVGA_USER_FLAGS_32BITONLY;
   }
   SVGA_WriteReg(SVGA_REG_ENABLE, FALSE);
   SVGA_WriteReg(SVGA_REG_CONFIG_DONE, FALSE);
   
   if(enableFIFO)
   {
   	 SVGA_Enable();
   }
#endif

   dbg_printf(dbg_siz, sizeof(gSVGA), sizeof(uint8 FARP *));
   
   return 0;
}

Bool SVGA_IsSVGA3()
{
	return gSVGA.deviceId == PCI_DEVICE_ID_VMWARE_SVGA3;
}

/*
 *-----------------------------------------------------------------------------
 *
 * SVGA_Enable --
 *
 *      Enable the SVGA device along with the SVGA FIFO.
 *
 * Results:
 *      void.
 *
 * Side effects:
 *      Initializes the command FIFO.
 *
 *-----------------------------------------------------------------------------
 */

void
SVGA_Enable(void)
{
   /*
    * Initialize the command FIFO. The beginning of FIFO memory is
    * used for an additional set of registers, the "FIFO registers".
    * These are higher-performance memory mapped registers which
    * happen to live in the same space as the FIFO. The driver is
    * responsible for allocating space for these registers, according
    * to the maximum number of registers supported by this driver
    * release.
    */

   gSVGA.fifoMem[SVGA_FIFO_MIN] = SVGA_FIFO_NUM_REGS * sizeof(uint32);
   gSVGA.fifoMem[SVGA_FIFO_MAX] = gSVGA.fifoSize;
   //gSVGA.fifoMem[SVGA_FIFO_MAX] = 0xFFFCUL - gSVGA.fifoMem[SVGA_FIFO_MIN];
   gSVGA.fifoMem[SVGA_FIFO_NEXT_CMD] = gSVGA.fifoMem[SVGA_FIFO_MIN];
   gSVGA.fifoMem[SVGA_FIFO_STOP] = gSVGA.fifoMem[SVGA_FIFO_MIN];

   /*
    * Prep work for 3D version negotiation. See SVGA3D_Init for
    * details, but we have to give the host our 3D protocol version
    * before enabling the FIFO.
    */

   if (SVGA_HasFIFOCap(SVGA_CAP_EXTENDED_FIFO) &&
       SVGA_IsFIFORegValid(SVGA_FIFO_GUEST_3D_HWVERSION)) {

      gSVGA.fifoMem[SVGA_FIFO_GUEST_3D_HWVERSION] = SVGA3D_HWVERSION_CURRENT;
   }

   /*
    * Enable the SVGA device and FIFO.
    */

   SVGA_WriteReg(SVGA_REG_ENABLE, TRUE);
   SVGA_WriteReg(SVGA_REG_CONFIG_DONE, TRUE);
   
   /*
    * Now that the FIFO is initialized, we can do an IRQ sanity check.
    * This makes sure that the VM's chipset and our own IRQ code
    * works. Better to find out now if something's wrong, than to
    * deadlock later.
    *
    * This inserts a FIFO fence, does a legacy sync to drain the FIFO,
    * then ensures that we received all applicable interrupts.
    */

#ifndef REALLY_TINY
   if (gSVGA.capabilities & SVGA_CAP_IRQMASK) {

      SVGA_WriteReg(SVGA_REG_IRQMASK, SVGA_IRQFLAG_ANY_FENCE);
      SVGA_ClearIRQ();

      SVGA_InsertFence();

      SVGA_WriteReg(SVGA_REG_SYNC, 1);
      while (SVGA_ReadReg(SVGA_REG_BUSY) != FALSE);

      SVGA_WriteReg(SVGA_REG_IRQMASK, 0);

      /* Check whether the interrupt occurred without blocking. */
      if ((gSVGA.irq.pending & SVGA_IRQFLAG_ANY_FENCE) == 0) {
         SVGA_Panic("SVGA IRQ appears to be present but broken.");
      }

      SVGA_WaitForIRQ();
   }
#endif
}


/*
 *-----------------------------------------------------------------------------
 *
 * SVGA_Disable --
 *
 *      Disable the SVGA portion of the video adapter, switching back
 *      to VGA mode.
 *
 * Results:
 *      void.
 *
 * Side effects:
 *      Switches modes. Disables the command FIFO, without flushing it.
 *      (Any commands still in the FIFO will be lost.)
 *
 *-----------------------------------------------------------------------------
 */

void
SVGA_Disable(void)
{
   SVGA_WriteReg(SVGA_REG_ENABLE, FALSE);
}


/*
 *-----------------------------------------------------------------------------
 *
 * SVGA_SetMode --
 *
 *      This switches the SVGA video mode and enables SVGA device.
 *
 * Results:
 *      void.
 *
 * Side effects:
 *      Transitions to SVGA mode if SVGA device currently disabled.
 *
 *-----------------------------------------------------------------------------
 */

void
SVGA_SetModeLegacy(uint32 width,   // IN
             uint32 height,  // IN
             uint32 bpp)     // IN
{
   gSVGA.width = width;
   gSVGA.height = height;
   gSVGA.bpp = bpp;

   SVGA_WriteReg(SVGA_REG_WIDTH, width);
   SVGA_WriteReg(SVGA_REG_HEIGHT, height);
   SVGA_WriteReg(SVGA_REG_BITS_PER_PIXEL, bpp);
   SVGA_ReadReg(SVGA_REG_FB_OFFSET);
   SVGA_WriteReg(SVGA_REG_ENABLE, TRUE);
   gSVGA.pitch = SVGA_ReadReg(SVGA_REG_BYTES_PER_LINE);
}


/*
 *-----------------------------------------------------------------------------
 *
 * SVGA_Panic --
 *
 *      This is a Panic handler which works in SVGA mode. It disables SVGA,
 *      then executes the Console's panic handler.
 *
 * Results:
 *      Never returns.
 *
 * Side effects:
 *      Disables SVGA mode, initializes VGA text mode.
 *
 *-----------------------------------------------------------------------------
 */

#ifndef REALLY_TINY
void
SVGA_Panic(const char *msg)  // IN
{
   SVGA_Disable();
   ConsoleVGA_Init();
   Console_Panic(msg);
}
#endif


/*
 *-----------------------------------------------------------------------------
 *
 * SVGA_DefaultFaultHandler --
 *
 *      This is a default fault handler which works in SVGA mode. It
 *      disables SVGA, then executes the Console's default fault
 *      handler.
 *
 * Results:
 *      Never returns.
 *
 * Side effects:
 *      Disables SVGA mode, initializes VGA text mode.
 *
 *-----------------------------------------------------------------------------
 */

void
SVGA_DefaultFaultHandler(int vector)  // IN
{
#ifndef REALLY_TINY
   SVGA_Disable();
   Console_UnhandledFault(vector);
#endif
}


/*
 *-----------------------------------------------------------------------------
 *
 * SVGA_ReadReg --
 *
 *      Read an SVGA device register (SVGA_REG_*).
 *      This must be called after the PCI device's IOspace has been mapped.
 *
 * Results:
 *      32-bit register value.
 *
 * Side effects:
 *      Depends on the register value. Some special registers
 *      like SVGA_REG_BUSY have significant side-effects.
 *
 *-----------------------------------------------------------------------------
 */

uint32
SVGA_ReadReg(uint32 index)  // IN
{
   if(SVGA_IsSVGA3())
   {
      return gSVGA.rmmio[index];
   }
   else
   {
   	  /* locking? */
      outpd(gSVGA.ioBase + SVGA_INDEX_PORT, index);
      return inpd(gSVGA.ioBase + SVGA_VALUE_PORT);
   }
}


/*
 *-----------------------------------------------------------------------------
 *
 * SVGA_WriteReg --
 *
 *      Write an SVGA device register (SVGA_REG_*).
 *      This must be called after the PCI device's IOspace has been mapped.
 *
 * Results:
 *      void.
 *
 * Side effects:
 *      Depends on the register value.
 *
 *-----------------------------------------------------------------------------
 */

void
SVGA_WriteReg(uint32 index,  // IN
              uint32 value)  // IN
{
   if(SVGA_IsSVGA3())
   {
      gSVGA.rmmio[index] = value;
   }
   else
   {
   	outpd(gSVGA.ioBase + SVGA_INDEX_PORT, index);
   	outpd(gSVGA.ioBase + SVGA_VALUE_PORT, value);
   }
}


/*
 *-----------------------------------------------------------------------------
 *
 * SVGA_IsFIFORegValid --
 *
 *      Check whether space has been allocated for a particular FIFO register.
 *
 *      This isn't particularly useful for our simple examples, since the
 *      same binary is responsible for both allocating and using the FIFO,
 *      but a real driver may need to check this if the module which handles
 *      initialization and mode switches is separate from the module which
 *      actually writes to the FIFO.
 *
 * Results:
 *      TRUE if the register has been allocated, FALSE if the register
 *      does not exist.
 *
 * Side effects:
 *      None.
 *
 *-----------------------------------------------------------------------------
 */

Bool
SVGA_IsFIFORegValid(int reg)
{
   return gSVGA.fifoMem[SVGA_FIFO_MIN] > (reg << 2);
}


/*
 *-----------------------------------------------------------------------------
 *
 * SVGA_HasFIFOCap --
 *
 *      Check whether the SVGA device has a particular FIFO capability bit.
 *
 * Results:
 *      TRUE if the capability is present, FALSE if it's absent.
 *
 * Side effects:
 *      None.
 *
 *-----------------------------------------------------------------------------
 */

Bool
SVGA_HasFIFOCap(unsigned long cap)
{
   return (gSVGA.fifoMem[SVGA_FIFO_CAPABILITIES] & cap) != 0;
}

/*
 *-----------------------------------------------------------------------------
 *
 * SVGA_RingDoorbell --
 *
 *      FIFO fences are fundamentally a host-to-guest notification
 *      mechanism.  This is the opposite: we can explicitly wake up
 *      the host when we know there is work for it to do.
 *
 *      Background: The host processes the SVGA command FIFO in a
 *      separate thread which runs asynchronously with the virtual
 *      machine's CPU and other I/O devices. When the SVGA device is
 *      idle, this thread is sleeping. It periodically wakes up to
 *      poll for new commands. This polling must occur for various
 *      reasons, but it's mostly related to the historical way in
 *      which the SVGA device processes 2D updates.
 *
 *      This polling introduces significant latency between when the
 *      first new command is placed in an empty FIFO, and when the
 *      host begins processing it. Normally this isn't a huge problem
 *      since the host and guest run fairly asynchronously, but in
 *      a synchronization-heavy workload this can be a bottleneck.
 *
 *      For example, imagine an application with a worst-case
 *      synchronization bottleneck: The guest enqueues a single FIFO
 *      command, then waits for that command to finish using
 *      SyncToFence, then the guest spends a little time doing
 *      CPU-intensive processing before the cycle repeats. The
 *      workload may be latency-bound if the host-to-guest or
 *      guest-to-host notifications ever block.
 *
 *      One solution would be for the guest to explicitly wake up the
 *      SVGA3D device any time a command is enqueued. This would solve
 *      the latency bottleneck above, but it would be inefficient on
 *      single-CPU host machines. One could easily imagine a situation
 *      in which we wake up the host after enqueueing one FIFO
 *      command, the physical CPU context switches to the SVGA
 *      device's thread, the single command is processed, then we
 *      context switch back to running guest code.
 *
 *      Our recommended solution is to wake up the host only either:
 *
 *         - After a "significant" amount of work has been enqueued into
 *           the FIFO. For example, at least one 3D drawing command.
 *
 *         - After a complete frame has been rendered.
 *
 *         - Just before the guest sleeps for any reason.
 *
 *      This function implements the above guest wakeups. It uses the
 *      SVGA_FIFO_BUSY register to quickly assess whether the SVGA
 *      device may be idle. If so, it asynchronously wakes up the host
 *      by writing to SVGA_REG_SYNC.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      May wake up the SVGA3D device.
 *
 *-----------------------------------------------------------------------------
 */

void
SVGA_RingDoorbell(void)
{
   if (SVGA_IsFIFORegValid(SVGA_FIFO_BUSY) &&
       gSVGA.fifoMem[SVGA_FIFO_BUSY] == FALSE) {

      /* Remember that we already rang the doorbell. */
      gSVGA.fifoMem[SVGA_FIFO_BUSY] = TRUE;

      /*
       * Asynchronously wake up the SVGA3D device.  The second
       * parameter is an arbitrary nonzero 'sync reason' which can be
       * used for debugging, but which isn't part of the SVGA3D
       * protocol proper and which isn't used by release builds of
       * VMware products.
       */
      SVGA_WriteReg(SVGA_REG_SYNC, 1);
   }
}


/*
 *-----------------------------------------------------------------------------
 *
 * SVGA_Flush --
 *
 *      Force to apply all register changes. Useful if registry changes
 *      needs be applyed immediately
 *
 *-----------------------------------------------------------------------------
 */
void SVGA_Flush(void)
{
		SVGA_WriteReg(SVGA_REG_SYNC, 1);
		while (SVGA_ReadReg(SVGA_REG_BUSY) != FALSE);
#ifndef VXD32
    dbg_printf("SVGA_Flush\n");
#endif
}

#if 0 /* JH: all commands using bounce buffer and fifo, better write them myself */

/*
 *-----------------------------------------------------------------------------
 *
 * SVGA_FIFOReserve --
 *
 *      Begin writing a command to the FIFO buffer. There are several
 *      examples floating around which show how to write to the FIFO
 *      buffer, but this is the preferred method: write directly to
 *      FIFO memory in the common case, but if the command would not
 *      be contiguous, use a bounce buffer.
 *
 *      This method is easy to use, and quite fast. The X.org driver
 *      does not yet use this method, but recent Windows drivers use
 *      it.
 *
 *      The main principles here are:
 *
 *        - There are multiple code paths. In the best case, we write
 *          directly to the FIFO. In the next-best case, we use a
 *          static bounce buffer.  If you need to support arbitrarily
 *          large commands, you can have a worst case in which you use
 *          a dynamically sized bounce buffer.
 *
 *        - We must tell the host that we're reserving FIFO
 *          space. This is important because the device doesn't
 *          guarantee it will preserve the contents of FIFO memory
 *          which hasn't been reserved. If we write to a totally
 *          unused portion of the FIFO and the VM is suspended, on
 *          resume that data will no longer exist.
 *
 *      This function is not re-entrant. If your driver is
 *      multithreaded or may be used from multiple processes
 *      concurrently, you must make sure to serialize all FIFO
 *      commands.
 *
 *      The caller must pair this command with SVGA_FIFOCommit or
 *      SVGA_FIFOCommitAll.
 *
 * Results:
 *      Returns a pointer to the location where the FIFO command can
 *      be written. There will be room for at least 'bytes' bytes of
 *      data.
 *
 * Side effects:
 *      Begins a FIFO command, reserves space in the FIFO.
 *      May block (in SVGAFIFOFull) if the FIFO is full.
 *
 *-----------------------------------------------------------------------------
 */

void FARP *
SVGA_FIFOReserve(uint32 bytes)  // IN
{
   volatile uint32 FARP *fifo = gSVGA.fifoMem;
   uint32 max = fifo[SVGA_FIFO_MAX];
   uint32 min = fifo[SVGA_FIFO_MIN];
#if 0
   uint32 nextCmd = fifo[SVGA_FIFO_NEXT_CMD];
   Bool reserveable = SVGA_HasFIFOCap(SVGA_FIFO_CAP_RESERVE);
#endif

   /*
    * This example implementation uses only a statically allocated
    * buffer.  If you want to support arbitrarily large commands,
    * dynamically allocate a buffer if and only if it's necessary.
    */

   if (bytes > SVGA_BOUNCE_SIZE /*sizeof gSVGA.fifo.bounceBuffer*/ ||
       bytes > (max - min)) {
      SVGA_Panic("FIFO command too large");
   }

   if (bytes % sizeof(uint32)) {
      SVGA_Panic("FIFO command length not 32-bit aligned");
   }

   if (gSVGA.fifo.reservedSize != 0) {
      SVGA_Panic("FIFOReserve before FIFOCommit");
   }

   gSVGA.fifo.reservedSize = bytes;

#if 0
   while (1) {
      uint32 stop = fifo[SVGA_FIFO_STOP];
      Bool reserveInPlace = FALSE;
      Bool needBounce = FALSE;

      /*
       * Find a strategy for dealing with "bytes" of data:
       * - reserve in place, if there's room and the FIFO supports it
       * - reserve in bounce buffer, if there's room in FIFO but not
       *   contiguous or FIFO can't safely handle reservations
       * - otherwise, sync the FIFO and try again.
       */

      if (nextCmd >= stop) {
         /* There is no valid FIFO data between nextCmd and max */

         if (nextCmd + bytes < max ||
             (nextCmd + bytes == max && stop > min)) {
            /*
             * Fastest path 1: There is already enough contiguous space
             * between nextCmd and max (the end of the buffer).
             *
             * Note the edge case: If the "<" path succeeds, we can
             * quickly return without performing any other tests. If
             * we end up on the "==" path, we're writing exactly up to
             * the top of the FIFO and we still need to make sure that
             * there is at least one unused DWORD at the bottom, in
             * order to be sure we don't fill the FIFO entirely.
             *
             * If the "==" test succeeds, but stop <= min (the FIFO
             * would be completely full if we were to reserve this
             * much space) we'll end up hitting the FIFOFull path below.
             */
            reserveInPlace = TRUE;
         } else if ((max - nextCmd) + (stop - min) <= bytes) {
            /*
             * We have to split the FIFO command into two pieces,
             * but there still isn't enough total free space in
             * the FIFO to store it.
             *
             * Note the "<=". We need to keep at least one DWORD
             * of the FIFO free at all times, or we won't be able
             * to tell the difference between full and empty.
             */
            SVGAFIFOFull();
         } else {
            /*
             * Data fits in FIFO but only if we split it.
             * Need to bounce to guarantee contiguous buffer.
             */
            needBounce = TRUE;
         }

      } else {
         /* There is FIFO data between nextCmd and max */

         if (nextCmd + bytes < stop) {
            /*
             * Fastest path 2: There is already enough contiguous space
             * between nextCmd and stop.
             */
            reserveInPlace = TRUE;
         } else {
            /*
             * There isn't enough room between nextCmd and stop.
             * The FIFO is too full to accept this command.
             */
            SVGAFIFOFull();
         }
      }

      /*
       * If we decided we can write directly to the FIFO, make sure
       * the VMX can safely support this.
       */
      if (reserveInPlace) {
         if (reserveable || bytes <= sizeof(uint32)) {
            gSVGA.fifo.usingBounceBuffer = FALSE;
            if (reserveable) {
               fifo[SVGA_FIFO_RESERVED] = bytes;
            }
            return nextCmd + (uint8 FARP*) fifo;
         } else {
            /*
             * Need to bounce because we can't trust the VMX to safely
             * handle uncommitted data in FIFO.
             */
            needBounce = TRUE;
         }
      }

      /*
       * If we reach here, either we found a full FIFO, called
       * SVGAFIFOFull to make more room, and want to try again, or we
       * decided to use a bounce buffer instead.
       */
      if (needBounce) {
         gSVGA.fifo.usingBounceBuffer = TRUE;
         return (void FARP *)(&gSVGA.fifo.bounceBuffer[0]);
      }
   } /* while (1) */
#else
#if 0
  gSVGA.fifo.usingBounceBuffer = TRUE;
  return (void FARP *)(&gSVGA.fifo.bounceBuffer[0]);
#endif
	return (void FARP *)(gSVGA.fifo.bounceMem);
#endif
}


/*
 *-----------------------------------------------------------------------------
 *
 * SVGA_FIFOCommit --
 *
 *      Commit a block of FIFO data which was placed in the buffer
 *      returned by SVGA_FIFOReserve. Every Reserve must be paired
 *      with exactly one Commit, but the sizes don't have to match.
 *      The caller is free to commit less space than they
 *      reserved. This can be used if the command size isn't known in
 *      advance, but it is reasonable to make a worst-case estimate.
 *
 *      The commit size does not have to match the size of a single
 *      FIFO command. This can be used to write a partial command, or
 *      to write multiple commands at once.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      None.
 *
 *-----------------------------------------------------------------------------
 */


#ifndef VXD32
static void WriteFifo(uint32 pos, uint32 dw)
{
	uint32 act = pos >> 16;
	uint16 off = pos & 0xFFFF;
	
	uint32 FARP *ptr;
	
	if(act != gSVGA.fifoAct)
	{
		DPMI_SetSegBase(gSVGA.fifoSel, gSVGA.fifoLinear + (act << 16));
		gSVGA.fifoAct = act;
	}
	
	//dbg_printf("Fifo: %ld (%X:%X)\n", pos, gSVGA.fifoAct, off);
	
	ptr = (uint32 FARP *)(gSVGA.fifoSel :> off);
	
	*ptr = dw;
}
#endif

void
SVGA_FIFOCommit(uint32 bytes)  // IN
{
   volatile uint32 FARP *fifo = gSVGA.fifoMem;
   uint32 nextCmd = fifo[SVGA_FIFO_NEXT_CMD];
   uint32 max = fifo[SVGA_FIFO_MAX];
   uint32 min = fifo[SVGA_FIFO_MIN];
   //Bool reserveable = SVGA_HasFIFOCap(SVGA_FIFO_CAP_RESERVE);

   if (gSVGA.fifo.reservedSize == 0) {
      SVGA_Panic("FIFOCommit before FIFOReserve");
   }

   gSVGA.fifo.reservedSize = 0;

#if 0 /* always using bounce buffer */
   if (gSVGA.fifo.usingBounceBuffer)
#endif
   {
      /*
       * Slow paths: copy out of a bounce buffer.
       */
      uint8 FARP *buffer = gSVGA.fifo.bounceMem;

#if 0
      if (reserveable) {
         /*
          * Slow path: bulk copy out of a bounce buffer in two chunks.
          *
          * Note that the second chunk may be zero-length if the reserved
          * size was large enough to wrap around but the commit size was
          * small enough that everything fit contiguously into the FIFO.
          *
          * Note also that we didn't need to tell the FIFO about the
          * reservation in the bounce buffer, but we do need to tell it
          * about the data we're bouncing from there into the FIFO.
          */

         uint32 chunkSize = MIN(bytes, max - nextCmd);
         fifo[SVGA_FIFO_RESERVED] = bytes;
         drv_memcpy(nextCmd + (uint8 FARP*) fifo, buffer, chunkSize);
         drv_memcpy(min + (uint8 FARP*) fifo, buffer + chunkSize, bytes - chunkSize);

      } else
#endif
      {
         /*
          * Slowest path: copy one dword at a time, updating NEXT_CMD as
          * we go, so that we bound how much data the guest has written
          * and the host doesn't know to checkpoint.
          */

         uint32 FARP *dword = (uint32 FARP *)buffer;

         while (bytes > 0) {
            //fifo[nextCmd / sizeof(uint32)] = *dword++;

#ifndef VXD32
            WriteFifo(nextCmd, *dword);
#else
            fifo[nextCmd / sizeof(uint32)] = *dword;
#endif
            dword++;
            
            nextCmd += sizeof(uint32);
            if (nextCmd >= max) {
               nextCmd = min;
            }
            fifo[SVGA_FIFO_NEXT_CMD] = nextCmd;
            bytes -= sizeof *dword;
         }
      }
   }

#if 0
   /*
    * Atomically update NEXT_CMD, if we didn't already
    */
   if (!gSVGA.fifo.usingBounceBuffer || reserveable) {
      nextCmd += bytes;
      if (nextCmd >= max) {
         nextCmd -= max - min;
      }
      fifo[SVGA_FIFO_NEXT_CMD] = nextCmd;
   }

   /*
    * Clear the reservation in the FIFO.
    */
   if (reserveable) {
      fifo[SVGA_FIFO_RESERVED] = 0;
   }
#endif
}


/*
 *-----------------------------------------------------------------------------
 *
 * SVGA_FIFOCommitAll --
 *
 *      This is a convenience wrapper for SVGA_FIFOCommit(), which
 *      always commits the last reserved block in its entirety.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      SVGA_FIFOCommit.
 *
 *-----------------------------------------------------------------------------
 */

void
SVGA_FIFOCommitAll(void)
{
   SVGA_FIFOCommit(gSVGA.fifo.reservedSize);
}


/*
 *-----------------------------------------------------------------------------
 *
 * SVGA_FIFOReserveCmd --
 *
 *      This is a convenience wrapper around SVGA_FIFOReserve, which
 *      prefixes the reserved memory block with a uint32 that
 *      indicates the command type.
 *
 * Results:
 *      Always returns a pointer to 'bytes' bytes of reserved space in the FIFO.
 *
 * Side effects:
 *      Begins a FIFO command, reserves space in the FIFO. Writes a
 *      1-word header into the FIFO.  May block (in SVGAFIFOFull) if
 *      the FIFO is full.
 *
 *-----------------------------------------------------------------------------
 */

void FARP *
SVGA_FIFOReserveCmd(uint32 type,   // IN
                    uint32 bytes)  // IN
{
   uint32 FARP *cmd = SVGA_FIFOReserve(bytes + sizeof type);
   cmd[0] = type;
   return cmd + 1;
}


/*
 *-----------------------------------------------------------------------------
 *
 * SVGA_FIFOReserveEscape --
 *
 *      This is a convenience wrapper around SVGA_FIFOReserve, which
 *      prefixes the reserved memory block with an ESCAPE command header.
 *
 *      ESCAPE commands are a way of encoding extensible and
 *      variable-length packets within the basic FIFO protocol
 *      itself. ESCAPEs are used for some SVGA device functionality,
 *      like video overlays, for VMware's internal debugging tools,
 *      and for communicating with third party code that can load into
 *      the SVGA device.
 *
 * Results:
 *      Always returns a pointer to 'bytes' bytes of reserved space in the FIFO.
 *
 * Side effects:
 *      Begins a FIFO command, reserves space in the FIFO. Writes a
 *      3-word header into the FIFO.  May block (in SVGAFIFOFull) if
 *      the FIFO is full.
 *
 *-----------------------------------------------------------------------------
 */

void FARP *
SVGA_FIFOReserveEscape(uint32 nsid,   // IN
                       uint32 bytes)  // IN
{
   uint32 paddedBytes = (bytes + 3) & ~3UL;
#pragma pack(push)
#pragma pack(1)
   struct {
      uint32 cmd;
      uint32 nsid;
      uint32 size;
   } FARP *header;
#pragma pack(pop)
   
   header = SVGA_FIFOReserve(paddedBytes + sizeof(*header));

   header->cmd = SVGA_CMD_ESCAPE;
   header->nsid = nsid;
   header->size = bytes;

   return header + 1;
}


/*
 *-----------------------------------------------------------------------------
 *
 * SVGAFIFOFull --
 *
 *      This function is called repeatedly as long as the FIFO has too
 *      little free space for us to continue.
 *
 *      The simplest implementation of this function is a no-op.  This
 *      will just burn guest CPU until space is available. (That's a
 *      bad idea, since the host probably needs that CPU in order to
 *      make progress on emptying the FIFO.)
 *
 *      A better implementation would sleep until a FIFO progress
 *      interrupt occurs. Depending on the OS you're writing drivers
 *      for, this may deschedule the calling task or it may simply put
 *      the CPU to sleep.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      None.
 *
 *-----------------------------------------------------------------------------
 */

#if 0
void
SVGAFIFOFull(void)
{
#ifndef REALLY_TINY
   if (SVGA_IsFIFORegValid(SVGA_FIFO_FENCE_GOAL) &&
       (gSVGA.capabilities & SVGA_CAP_IRQMASK)) {

      /*
       * On hosts which support interrupts, we can sleep until the
       * FIFO_PROGRESS interrupt occurs. This is the most efficient
       * thing to do when the FIFO fills up.
       *
       * As with the IRQ-based SVGA_SyncToFence(), this will only work
       * on Workstation 6.5 virtual machines and later.
       */

      SVGA_WriteReg(SVGA_REG_IRQMASK, SVGA_IRQFLAG_FIFO_PROGRESS);
      SVGA_ClearIRQ();
      SVGA_RingDoorbell();
      SVGA_WaitForIRQ();
      SVGA_WriteReg(SVGA_REG_IRQMASK, 0);

   } else {

      /*
       * Fallback implementation: Perform one iteration of the
       * legacy-style sync. This synchronously processes FIFO commands
       * for an arbitrary amount of time, then returns control back to
       * the guest CPU.
       */

      SVGA_WriteReg(SVGA_REG_SYNC, 1);
      SVGA_ReadReg(SVGA_REG_BUSY);
   }
#endif
}
#endif


/*
 *-----------------------------------------------------------------------------
 *
 * SVGA_InsertFence --
 *
 *      Write a new fence value to the FIFO.
 *
 *      Fences are the basis of the SVGA device's synchronization
 *      model.  A fence is a marker inserted into the FIFO by the
 *      guest. The host processes all FIFO commands in order. Once the
 *      fence is reached, the host does two things:
 *
 *       - The fence value is written to the SVGA_FIFO_FENCE register.
 *       - Optionally, an interrupt is raised.
 *
 *      There are multiple ways to use fences for synchronization. See
 *      SVGA_SyncToFence and SVGA_HasFencePassed.
 *
 * Results:
 *
 *      Returns the value of the fence we inserted. Fence values
 *      increment, wrapping around at 32 bits. Fence value zero is
 *      reserved to mean "no fence". This function never returns zero.
 *
 *      Certain very old versions of the VMware SVGA device do not
 *      support fences. On these devices, we always return 1. On these
 *      devices, SyncToFence will always do a full Sync and
 *      SyncToFence will always return FALSE, so the actual fence
 *      value we use is unimportant. Code written to use fences will
 *      run inefficiently, but it will still be correct.
 *
 * Side effects:
 *      Writes to the FIFO. Increments our nextFence.
 *
 *-----------------------------------------------------------------------------
 */

uint32
SVGA_InsertFence(void)
{
   uint32 fence;

#pragma pack(push)
#pragma pack(1)
   struct {
      uint32 id;
      uint32 fence;
   } FARP *cmd;
#pragma pack(pop)

   if (!SVGA_HasFIFOCap(SVGA_FIFO_CAP_FENCE)) {
      return 1;
   }

   if (gSVGA.fifo.nextFence == 0) {
      gSVGA.fifo.nextFence = 1;
   }
   fence = gSVGA.fifo.nextFence++;

   cmd = SVGA_FIFOReserve(sizeof *cmd);
   cmd->id = SVGA_CMD_FENCE;
   cmd->fence = fence;
   SVGA_FIFOCommitAll();

   return fence;
}


/*
 *-----------------------------------------------------------------------------
 *
 * SVGA_SyncToFence --
 *
 *      Sleep until the SVGA device has processed all FIFO commands
 *      prior to the insertion point of the specified fence.
 *
 *      This is the most important way to maintain synchronization
 *      between the driver and the SVGA3D device itself. It can be
 *      used to provide flow control between the host and the guest,
 *      or to ensure that DMA operations have completed before reusing
 *      guest memory.
 *
 *      If the provided fence is zero or it has already passed,
 *      this is a no-op.
 *
 *      If the SVGA device and virtual machine hardware version are
 *      both new enough (Workstation 6.5 or later), this will use an
 *      efficient interrupt-driven mechanism to sleep until just after
 *      the host processes the fence.
 *
 *      If not, this will use a less efficient synchronization
 *      mechanism which may require the host to process significantly
 *      more of the FIFO than is necessary.
 *
 * Results:
 *      void.
 *
 * Side effects:
 *      None.
 *
 *-----------------------------------------------------------------------------
 */

void
SVGA_SyncToFence(uint32 fence)  // IN
{
   if (!fence) {
      return;
   }

   if (!SVGA_HasFIFOCap(SVGA_FIFO_CAP_FENCE)) {
      /*
       * Fall back on the legacy sync if the host does not support
       * fences.  This is the old sync mechanism that has been
       * supported in the SVGA device pretty much since the dawn of
       * time: write to the SYNC register, then read from BUSY until
       * it's nonzero. This will drain the entire FIFO.
       *
       * The parameter we write to SVGA_REG_SYNC is an arbitrary
       * nonzero value which can be used for debugging, but which is
       * ignored by release builds of VMware products.
       */

      SVGA_WriteReg(SVGA_REG_SYNC, 1);
      while (SVGA_ReadReg(SVGA_REG_BUSY) != FALSE);
      return;
   }

   if (SVGA_HasFencePassed(fence)) {
      /*
       * Nothing to do
       */

      return;
   }

#ifndef REALLY_TINY
   if (SVGA_IsFIFORegValid(SVGA_FIFO_FENCE_GOAL) &&
       (gSVGA.capabilities & SVGA_CAP_IRQMASK)) {

      /*
       * On hosts which support interrupts and which support the
       * FENCE_GOAL interrupt, we can use our preferred
       * synchronization mechanism.
       *
       * This provides low latency notification both from guest to
       * host and from host to guest, and it doesn't block the guest
       * while we're waiting.
       *
       * This will only work on Workstation 6.5 virtual machines
       * or later. Older virtual machines did not allocate an IRQ
       * for the SVGA device, and the IRQMASK capability will be
       * unset.
       */

      /*
       * Set the fence goal. This asks the host to send an interrupt
       * when this specific fence has been reached.
       */

      gSVGA.fifoMem[SVGA_FIFO_FENCE_GOAL] = fence;
      SVGA_WriteReg(SVGA_REG_IRQMASK, SVGA_IRQFLAG_FENCE_GOAL);

      SVGA_ClearIRQ();

      /*
       * Must check again, in case we reached the fence between the
       * first HasFencePassed and when we set up the IRQ.
       *
       * As a small performance optimization, we check yet again after
       * RingDoorbell, since there's a chance that RingDoorbell will
       * deschedule this VM and process some SVGA FIFO commands in a
       * way that appears synchronous from the VM's point of view.
       */

      if (!SVGA_HasFencePassed(fence)) {
         /*
          * We're about to go to sleep. Make sure the host is awake.
          */
         SVGA_RingDoorbell();

         if (!SVGA_HasFencePassed(fence)) {
            SVGA_WaitForIRQ();
         }
      }

      SVGA_WriteReg(SVGA_REG_IRQMASK, 0);

   } else
#endif // REALLY_TINY
   {
      /*
       * Sync-to-fence mechanism for older hosts. Wake up the host,
       * and spin on BUSY until we've reached the fence. This
       * processes FIFO commands synchronously, blocking the VM's
       * execution entirely until it's done.
       */

      Bool busy = TRUE;

      SVGA_WriteReg(SVGA_REG_SYNC, 1);

      while (!SVGA_HasFencePassed(fence) && busy) {
         busy = (SVGA_ReadReg(SVGA_REG_BUSY) != 0);
      }
   }

#ifndef REALLY_TINY
   if (!SVGA_HasFencePassed(fence)) {
      /*
       * This shouldn't happen. If it does, there might be a bug in
       * the SVGA device.
       */
      SVGA_Panic("SyncToFence failed!");
   }
#endif
}


/*
 *-----------------------------------------------------------------------------
 *
 * SVGA_HasFencePassed --
 *
 *      Test whether the host has processed all FIFO commands prior to
 *      the insertion point of the specified fence.
 *
 *      This function tolerates fence wrap-around, but it will return
 *      the wrong result if 'fence' is more than 2^31 fences old. It
 *      is recommended that callers don't allow fence values to
 *      persist indefinitely. Once we notice that a fence has been
 *      passed, that fence variable should be set to zero so we don't
 *      test it in the future.
 *
 * Results:
 *      TRUE if the fence has been passed,
 *      TRUE if fence==0 (no fence),
 *      FALSE if the fence has not been passed,
 *      FALSE if the SVGA device does not support fences.
 *
 * Side effects:
 *      None.
 *
 *-----------------------------------------------------------------------------
 */

Bool
SVGA_HasFencePassed(uint32 fence)  // IN
{
   if (!fence) {
      return TRUE;
   }

   if (!SVGA_HasFIFOCap(SVGA_FIFO_CAP_FENCE)) {
      return FALSE;
   }

   return ((int32)(gSVGA.fifoMem[SVGA_FIFO_FENCE] - fence)) >= 0;
}


/*
 *-----------------------------------------------------------------------------
 *
 * SVGA_ClearIRQ --
 *
 *      Clear all pending IRQs. Any IRQs which occurred prior to this
 *      function call will be ignored by the next SVGA_WaitForIRQ()
 *      call.
 *
 *      Does not affect the current IRQ mask. This function is not
 *      useful unless the SVGA device has IRQ support.
 *
 * Results:
 *      Returns a mask of all the interrupt flags that were set prior
 *      to the clear.
 *
 * Side effects:
 *      None.
 *
 *-----------------------------------------------------------------------------
 */
#ifndef REALLY_TINY
/*
uint32
SVGA_ClearIRQ(void)
{
   uint32 flags = 0;
   Atomic_Exchange(gSVGA.irq.pending, flags);
   
   return flags;
}*/

uint32 SVGA_ClearIRQ(void); 
#pragma aux SVGA_ClearIRQ = \
    ".386"              \
    "push   eax"        \
    "xor    eax, eax"    \
    "xchg   ptr gSVGA.irq.pending, eax"     \
    "mov    dx, ax"     \
    "shr    eax, 16"    \
    "xchg   dx, ax"     \
    "xchg   bx, ax"     \
    "pop    eax"        \
    "xchg   bx, ax"     \
    value [dx ax] modify [bx];

#endif

/*
 *-----------------------------------------------------------------------------
 *
 * SVGA_WaitForIRQ --
 *
 *      Sleep until an SVGA interrupt occurs. When we wake up, return
 *      a mask of all SVGA IRQs which occurred while we were asleep.
 *
 *      The design of this function will be different for every OS,
 *      but it is imperative that you ensure it will work properly
 *      no matter when the interrupt actually occurs. The IRQ could have
 *      already happened (any time since the last ClearIRQ, it could
 *      happen while this function is running, or it could happen
 *      after we decide to sleep.
 *
 *      In this example:
 *
 *        1. If the IRQ occurred before this function was called,
 *           the bit will already be set in irq.pending. We can return.
 *
 *        2. If the IRQ occurs while this function is executing, we
 *           ask our IRQ handler to do a light-weight context switch
 *           back to the beginning of this function, so we can
 *           re-examine irq.pending before sleeping.
 *
 *        3. If the IRQ occurs while we're sleeping, we wake up
 *           from the HLT instruction and re-test irq.pending.
 *
 * Results:
 *      Returns a mask of all the interrupt flags that were set prior
 *      to the clear. This will always be nonzero.
 *
 * Side effects:
 *      Clears the irq.pending flags for exactly the set of IRQs we return.
 *
 *-----------------------------------------------------------------------------
 */
#ifndef REALLY_TINY
static __attribute__ ((noinline)) uint32
SVGAWaitForIRQInternal(void *arg, ...)
{
   uint32 flags = 0;

   /*
    * switchContext must be set to TRUE before we halt, or we'll deadlock.
    * This variable is marked as volatile, plus we use a compiler memory barrier.
    */
   gSVGA.irq.switchContext = TRUE;
   asm volatile ("" ::: "memory");

   Atomic_Exchange(gSVGA.irq.pending, flags);

   if (!flags) {
      Intr_Halt();
   }

   /*
    * Must set switchContext to FALSE before this stack frame is deallocated.
    */
   gSVGA.irq.switchContext = FALSE;
   asm volatile ("" ::: "memory");

   return flags;
}

uint32
SVGA_WaitForIRQ(void)
{
   uint32 flags;

   /*
    * Store this CPU context, and tell the IRQ handler to return
    * here if an IRQ occurs. We use IntrContext here just like a
    * setjmp/longjmp that works in ISRs.
    *
    * This must be above the irq.pending check, so that we re-test
    * irq.pending if an IRQ occurs after testing irq.pending but
    * before we halt. Without this, we could deadlock if an IRQ
    * comes in before we actually halt but after we've decided to
    * halt.
    */

   Intr_SaveContext((IntrContext*) &gSVGA.irq.newContext);

   do {
      /*
       * XXX: The arguments here are a kludge to prevent the interrupt
       *      stack frame from overlapping the stack frame referenced
       *      by Intr_SaveContext().
       *
       *      Be sure to look at the generated assembly and compare the
       *      size of this argument list to the amount of stack space
       *      allocated for Intr_SaveContext.
       */
      flags = SVGAWaitForIRQInternal(NULL, NULL, NULL, NULL, NULL, NULL);

   } while (flags == 0);

   return flags;
}

#endif

/*
 *-----------------------------------------------------------------------------
 *
 * SVGAInterruptHandler --
 *
 *      This is the ISR for the SVGA device's interrupt. We ask the
 *      SVGA device which interrupt occurred, and clear its flag.
 *
 *      To report this IRQ to the rest of the driver, we:
 *
 *        1. Atomically remember the IRQ in the irq.pending bitmask.
 *
 *        2. Optionally switch to a different light-weight thread
 *           or a different place in this thread upon returning.
 *           This is analogous to waking up a sleeping thread in
 *           a driver for a real operating system. See SVGA_WaitForIRQ
 *           for details.
 *
 * Results:
 *      void.
 *
 * Side effects:
 *      Sets bits in pendingIRQs. Reads and clears the device's IRQ flags.
 *      May switch execution contexts.
 *
 *-----------------------------------------------------------------------------
 */

#ifndef REALLY_TINY
void
SVGAInterruptHandler(int vector)  // IN (unused)
{
   IntrContext *context = Intr_GetContext(vector);

   /*
    * The SVGA_IRQSTATUS_PORT is a separate I/O port, not a register.
    * Reading from it gives us the set of IRQ flags which are
    * set. Writing a '1' to any bit in this register will clear the
    * corresponding flag.
    *
    * Here, we read then clear all flags.
    */

   uint16 port = gSVGA.ioBase + SVGA_IRQSTATUS_PORT;
   uint32 irqFlags = IO_In32(port);
   IO_Out32(port, irqFlags);

   gSVGA.irq.count++;

   if (!irqFlags) {
      SVGA_Panic("Spurious SVGA IRQ");
   }

   Atomic_Or(gSVGA.irq.pending, irqFlags);

   if (gSVGA.irq.switchContext) {
      drv_memcpy((void*) &gSVGA.irq.oldContext, context, sizeof *context);
      drv_memcpy(context, (void*) &gSVGA.irq.newContext, sizeof *context);
      gSVGA.irq.switchContext = FALSE;
   }
}
#endif


/*
 *----------------------------------------------------------------------
 *
 * SVGA_AllocGMR --
 *
 *      Allocate a buffer from the framebuffer GMR for screen/DMA operations.
 *      Returns both a pointer (for us to use) and an SVGAGuestPtr (for the
 *      SVGA device to use).
 *
 *      XXX: This is a trivial implementation which just returns
 *           consecutive addresses in the framebuffer.
 *
 * Results:
 *      Returns a local pointer and an SVGAGuestPtr to unused memory.
 *
 * Side effects:
 *      Allocates memory.
 *
 *----------------------------------------------------------------------
 */

void FARP *
SVGA_AllocGMR(uint32 size,        // IN
              SVGAGuestPtr FARP *ptr)  // OUT
{
   static SVGAGuestPtr nextPtr = { SVGA_GMR_FRAMEBUFFER, 0 };
   *ptr = nextPtr;
   nextPtr.offset += size;
   return gSVGA.fbMem + ptr->offset;
}


/*
 *-----------------------------------------------------------------------------
 *
 * SVGA_Update --
 *
 *      Send a 2D update rectangle through the FIFO.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      None.
 *
 *-----------------------------------------------------------------------------
 */

void
SVGA_Update(uint32 x,       // IN
            uint32 y,       // IN
            uint32 width,   // IN
            uint32 height)  // IN
{
   SVGAFifoCmdUpdate FARP *cmd = SVGA_FIFOReserveCmd(SVGA_CMD_UPDATE, sizeof *cmd);
   cmd->x = x;
   cmd->y = y;
   cmd->width = width;
   cmd->height = height;
   SVGA_FIFOCommitAll();
}


/*
 *-----------------------------------------------------------------------------
 *
 * SVGA_BeginDefineCursor --
 *
 *      Begin an SVGA_CMD_DEFINE_CURSOR command. This copies the command header
 *      into the FIFO, and reserves enough space for the cursor image itself.
 *      We return pointers to FIFO memory where the AND/XOR masks can be written.
 *      When finished, the caller must invoke SVGA_FIFOCommitAll().
 *
 * Results:
 *      Returns pointers to memory where the caller can store the AND/XOR masks.
 *
 * Side effects:
 *      Reserves space in the FIFO.
 *
 *-----------------------------------------------------------------------------
 */

void
SVGA_BeginDefineCursor(const SVGAFifoCmdDefineCursor FARP *cursorInfo,  // IN
                       void FARP * FARP *andMask,                      // OUT
                       void FARP * FARP *xorMask)                      // OUT
{
   uint32 andPitch = ((cursorInfo->andMaskDepth * cursorInfo->width + 31) >> 5) << 2;
   uint32 andSize = andPitch * cursorInfo->height;
   uint32 xorPitch = ((cursorInfo->xorMaskDepth * cursorInfo->width + 31) >> 5) << 2;
   uint32 xorSize = xorPitch * cursorInfo->height;

   SVGAFifoCmdDefineCursor FARP *cmd = SVGA_FIFOReserveCmd(SVGA_CMD_DEFINE_CURSOR,
                                                      sizeof *cmd + andSize + xorSize);
   *cmd = *cursorInfo;
   *andMask = (void FARP*)(cmd + 1);
   *xorMask = (void FARP*)(andSize + (uint8 FARP*)(*andMask));
}


/*
 *-----------------------------------------------------------------------------
 *
 * SVGA_BeginDefineAlphaCursor --
 *
 *      Begin an SVGA_CMD_DEFINE_ALPHA_CURSOR command. This copies the
 *      command header into the FIFO, and reserves enough space for
 *      the cursor image itself.  We return a pointer to the FIFO
 *      memory where the caller should write a pre-multiplied BGRA
 *      image. When finished, the caller must invoke SVGA_FIFOCommitAll().
 *
 * Results:
 *      Returns pointers to memory where the caller can store the image.
 *
 * Side effects:
 *      Reserves space in the FIFO.
 *
 *-----------------------------------------------------------------------------
 */

void
SVGA_BeginDefineAlphaCursor(const SVGAFifoCmdDefineAlphaCursor FARP *cursorInfo,  // IN
                            void FARP * FARP * data)                             // OUT
{
   uint32 imageSize = cursorInfo->width * cursorInfo->height * sizeof(uint32);
   SVGAFifoCmdDefineAlphaCursor FARP *cmd = SVGA_FIFOReserveCmd(SVGA_CMD_DEFINE_ALPHA_CURSOR,
                                                           sizeof *cmd + imageSize);
   *cmd = *cursorInfo;
   *data = (void FARP*) (cmd + 1);
}


/*
 *-----------------------------------------------------------------------------
 *
 * SVGA_MoveCursor --
 *
 *      Change the position or visibility of the hardware cursor, using
 *      the Cursor Bypass 3 protocol.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Modifies FIFO registers.
 *
 *-----------------------------------------------------------------------------
 */

void
SVGA_MoveCursor(uint32 visible,   // IN
                uint32 x,         // IN
                uint32 y,         // IN
                uint32 screenId)  // IN
{
   if (SVGA_HasFIFOCap(SVGA_FIFO_CAP_SCREEN_OBJECT)) {
      gSVGA.fifoMem[SVGA_FIFO_CURSOR_SCREEN_ID] = screenId;
   }

   if (SVGA_HasFIFOCap(SVGA_FIFO_CAP_CURSOR_BYPASS_3)) {
      gSVGA.fifoMem[SVGA_FIFO_CURSOR_ON] = visible;
      gSVGA.fifoMem[SVGA_FIFO_CURSOR_X] = x;
      gSVGA.fifoMem[SVGA_FIFO_CURSOR_Y] = y;
      gSVGA.fifoMem[SVGA_FIFO_CURSOR_COUNT]++;
   }
}


/*
 *-----------------------------------------------------------------------------
 *
 * SVGA_BeginVideoSetRegs --
 *
 *      Begin an SVGA_ESCAPE_VMWARE_VIDEO_SET_REGS command. This can be
 *      used to set an arbitrary number of video registers atomically.
 *
 * Results:
 *      Returns a pointer to the command. The caller must fill in (*setRegs)->items.
 *
 * Side effects:
 *      Reserves space in the FIFO.
 *
 *-----------------------------------------------------------------------------
 */

void
SVGA_BeginVideoSetRegs(uint32 streamId,                   // IN
                       uint32 numItems,                   // IN
                       SVGAEscapeVideoSetRegs FARP * FARP * setRegs)  // OUT
{
   SVGAEscapeVideoSetRegs FARP *cmd;
   uint32 cmdSize = (sizeof *cmd
                     - sizeof cmd->items
                     + numItems * sizeof cmd->items[0]);

   cmd = SVGA_FIFOReserveEscape(SVGA_ESCAPE_NSID_VMWARE, cmdSize);
   cmd->header.cmdType = SVGA_ESCAPE_VMWARE_VIDEO_SET_REGS;
   cmd->header.streamId = streamId;

   *setRegs = cmd;
}


/*
 *-----------------------------------------------------------------------------
 *
 * SVGA_VideoSetReg --
 *
 *      Atomically set all registers in a video overlay unit.
 *
 *      'maxReg' should be the highest video overlay register that the
 *      caller understands. We will not set any registers with IDs
 *      higher than maxReg. This prevents older applications compiled
 *      with newer headers from sending improper values to new overlay
 *      registers.
 *
 *      Each video overlay unit (specified by streamId) has a collection
 *      of 32-bit registers which control the operation of that overlay.
 *      Changes to these registers take effect at the next Flush.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Writes a FIFO command.
 *
 *-----------------------------------------------------------------------------
 */

void
SVGA_VideoSetAllRegs(uint32 streamId,        // IN
                     SVGAOverlayUnit FARP *regs,  // IN
                     uint32 maxReg)          // IN
{
   uint32 FARP *regArray = (uint32 FARP*) regs;
   const uint32 numRegs = maxReg + 1;
   SVGAEscapeVideoSetRegs FARP *setRegs;
   uint32 i;

   SVGA_BeginVideoSetRegs(streamId, numRegs, &setRegs);

   for (i = 0; i < numRegs; i++) {
      setRegs->items[i].registerId = i;
      setRegs->items[i].value = regArray[i];
   }

   SVGA_FIFOCommitAll();
}


/*
 *-----------------------------------------------------------------------------
 *
 * SVGA_VideoSetReg --
 *
 *      Set a single video overlay register.
 *
 *      Each video overlay unit (specified by streamId) has a collection
 *      of 32-bit registers which control the operation of that overlay.
 *      Changes to these registers take effect at the next Flush.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Writes a FIFO command.
 *
 *-----------------------------------------------------------------------------
 */

void
SVGA_VideoSetReg(uint32 streamId,    // IN
                 uint32 registerId,  // IN
                 uint32 value)       // IN
{
   SVGAEscapeVideoSetRegs FARP *setRegs;

   SVGA_BeginVideoSetRegs(streamId, 1, &setRegs);
   setRegs->items[0].registerId = registerId;
   setRegs->items[0].value = value;
   SVGA_FIFOCommitAll();
}


/*
 *-----------------------------------------------------------------------------
 *
 * SVGA_VideoFlush --
 *
 *      Update a video overlay. On a VideoFlush, all register changes
 *      take effect, and the device will perform a DMA transfer to
 *      copy the next frame of video from guest memory.
 *
 *      The SVGA device will generally perform a DMA transfer from
 *      overlay memory only during a VideoFlush. The DMA operation
 *      will usually be completed before the host moves to the next
 *      FIFO command. However, this is not guaranteed. In unusual
 *      conditions, the device may re-perform the DMA operation at any
 *      time. This means that, as long as an overlay is enabled, its
 *      corresponding guest memory must be valid.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Writes a FIFO command. May cause an asynchronous DMA transfer.
 *
 *-----------------------------------------------------------------------------
 */

void
SVGA_VideoFlush(uint32 streamId)  // IN
{
   SVGAEscapeVideoFlush FARP *cmd;

   cmd = SVGA_FIFOReserveEscape(SVGA_ESCAPE_NSID_VMWARE, sizeof *cmd);
   cmd->cmdType = SVGA_ESCAPE_VMWARE_VIDEO_FLUSH;
   cmd->streamId = streamId;
   SVGA_FIFOCommitAll();
}

#endif /* 0 */
