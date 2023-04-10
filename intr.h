/* -*- Mode: C; c-basic-offset: 3 -*-
 *
 * intr.h - Interrupt vector management and interrupt routing.
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

#ifndef __INTR_H__
#define __INTR_H__

#include "types.h"
#include "io.h"

#define NUM_INTR_VECTORS    256
#define NUM_FAULT_VECTORS   0x20
#define NUM_IRQ_VECTORS     0x10
#define IRQ_VECTOR_BASE     NUM_FAULT_VECTORS
#define IRQ_VECTOR(irq)     ((irq) + IRQ_VECTOR_BASE)
#define USER_VECTOR_BASE    (IRQ_VECTOR_BASE + NUM_IRQ_VECTORS)
#define USER_VECTOR(n)      ((n) + USER_VECTOR_BASE)

#define IRQ_TIMER           0
#define IRQ_KEYBOARD        1

#define FAULT_DE            0x00    // Divide error
#define FAULT_NMI           0x02    // Non-maskable interrupt
#define FAULT_BP            0x03    // Breakpoint
#define FAULT_OF            0x04    // Overflow
#define FAULT_BR            0x05    // Bound range
#define FAULT_UD            0x06    // Undefined opcode
#define FAULT_NM            0x07    // No FPU
#define FAULT_DF            0x08    // Double Fault
#define FAULT_TS            0x0A    // Invalid TSS
#define FAULT_NP            0x0B    // Segment not present
#define FAULT_SS            0x0C    // Stack-segment fault
#define FAULT_GP            0x0D    // General Protection Fault
#define FAULT_PF            0x0E    // Page fault
#define FAULT_MF            0x10    // Math fault
#define FAULT_AC            0x11    // Alignment check
#define FAULT_MC            0x12    // Machine check
#define FAULT_XM            0x13    // SIMD floating point exception

#define PIC1_COMMAND_PORT  0x20
#define PIC1_DATA_PORT     0x21
#define PIC2_COMMAND_PORT  0xA0
#define PIC2_DATA_PORT     0xA1

typedef void (*IntrHandler)(int vector);
typedef void (*IntrContextFn)(void);

fastcall void Intr_Init(void);
fastcall void Intr_SetFaultHandlers(IntrHandler handler);

static inline void
Intr_Enable(void) {
   asm volatile ("sti");
}

static inline void
Intr_Disable(void) {
   asm volatile ("cli");
}

static inline Bool
Intr_Save(void) {
   uint32 eflags;
   asm volatile ("pushf; pop %0" : "=r" (eflags));
   return (eflags & 0x200) != 0;
}

static inline void
Intr_Restore(Bool flag) {
   if (flag) {
      Intr_Enable();
   } else {
      Intr_Disable();
   }
}

static inline void
Intr_Halt(void) {
   asm volatile ("hlt");
}

static inline void
Intr_Break(void) {
   asm volatile ("int3");
}

/*
 * This structure describes all execution state that's saved when an
 * interrupt or a setjmp occurs. In the case of an interrupt, this
 * structure actually describes the stack frame of the interrupt
 * trampoline.
 *
 * An interrupt handler can get a pointer to its IntrContext by
 * passing its first argument to the Intr_GetContext macro. This
 * allows an interrupt handler to examine the execution context in
 * which the interrupt occurred, to modify the interrupt's return
 * address, or even to implement input and output for OS traps.
 *
 * This module also provides functions for directly saving and
 * restoring IntrContext structures. This can be used much like
 * setjmp/longjmp, or it can even be used for simple cooperative or
 * preemptive multithreading. An interrupt handler can perform a
 * context switch by overwriting its IntrContext with a saved context.
 *
 * The definition of this structure must be kept in sync with the
 * machine code in our interrupt trampolines, and with the
 * assembly-language implementation of SaveContext and RestoreContext.
 */

typedef struct IntrContext {
   /*
    * General purpose registers. These are all saved after the value
    * of %esp is captured.
    */

   uint32  edi;
   uint32  esi;
   uint32  ebp;
   uint32  esp;
   uint32  ebx;
   uint32  edx;
   uint32  ecx;
   uint32  eax;

   /*
    * These values are save by the CPU during an interrupt.  By
    * convention, these values are at the top of the stack when %esp
    * was saved.
    *
    * The values of cs and eflags are ignored by Intr_SaveContext
    * and Intr_RestoreContext.
    */

   uint32  eip;
   uint32  cs;
   uint32  eflags;
} IntrContext;

/*
 * Always use the 'volatile' keyword when storing the result
 * of Intr_GetContext. GCC can erroneously decide to optimize
 * out any copies to this pointer, because it doesn't know the
 * values will be used by our trampoline.
 */

#define Intr_GetContext(arg)  ((IntrContext*) &(&arg)[1])

uint32 Intr_SaveContext(IntrContext *ctx);
void Intr_RestoreContext(IntrContext *ctx);
fastcall void Intr_InitContext(IntrContext *ctx, uint32 *stack, IntrContextFn main);

/*
 * To save space, we don't include assembly-language trampolines for
 * each interrupt vector. Instead, we allocate a table in the BSS
 * segment which we can fill in at runtime with simple trampoline
 * functions. This structure actually describes executable 32-bit
 * code.
 */

typedef struct {
   uint16      code1;
   uint32      arg;
   uint8       code2;
   IntrHandler handler;
   uint32      code3;
   uint32      code4;
   uint32      code5;
   uint32      code6;
   uint32      code7;
   uint32      code8;
} PACKED IntrTrampolineType;

extern IntrTrampolineType ALIGNED(4) IntrTrampoline[NUM_INTR_VECTORS];

/*
 * Intr_SetHandler --
 *
 *    Set a C-language interrupt handler for a particular vector.
 *    Note that the argument is a vector number, not an IRQ.
 */

static inline void
Intr_SetHandler(int vector, IntrHandler handler)
{
   IntrTrampoline[vector].handler = handler;
}

/*
 * Intr_SetMask --
 *
 *    (Un)mask a particular IRQ.
 */

static inline void
Intr_SetMask(int irq, Bool enable)
{
   uint8 port, bit, mask;

   if (irq >= 8) {
      bit = 1 << (irq - 8);
      port = PIC2_DATA_PORT;
   } else {
      bit = 1 << irq;
      port = PIC1_DATA_PORT;
   }

   mask = IO_In8(port);

   /* A '1' bit in the mask inhibits the interrupt. */
   if (enable) {
      mask &= ~bit;
   } else {
      mask |= bit;
   }

   IO_Out8(port, mask);
}


#endif /* __INTR_H__ */
