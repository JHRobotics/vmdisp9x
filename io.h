/*****************************************************************************

Copyright (c) 2012-2022  Michal Necasek

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

#include <conio.h>  /* For port I/O prototypes. */

/* The 16-bit compiler does not do inpd(). We have to do it ourselves.
 * NB: It might be OK to trash the high bits of EAX but better be safe.
 */
/* Warning: Destroys high bits of EAX. */
unsigned long inpd_asm( unsigned port );
#pragma aux inpd_asm =      \
    ".386"              \
    "push   eax"        \
    "in     eax, dx"    \
    "mov    dx, ax"     \
    "shr    eax, 16"    \
    "xchg   bx, ax"     \
    "pop    eax"        \
    "xchg   bx, ax"     \
    "xchg   ax, dx"     \
    parm [dx] value [dx ax] modify [bx] nomemory;
    
void outpd_asm( unsigned port, unsigned long val );
#pragma aux outpd_asm =      \
    ".386"              \
    "push   eax"        \
    "xchg   bx, ax"     \
    "shl    eax, 16"    \
    "mov    ax, bx"     \
    "out    dx, eax"    \
    "pop    eax"        \
    parm [dx] [bx ax] nomemory;

#ifdef NEEDVIDEO

static void vid_outb( void *cx, unsigned port, unsigned val )
{
    outp( port, val );
}

static void vid_outw( void *cx, unsigned port, unsigned val )
{
    outpw( port, val );
}

static unsigned vid_inb( void *cx, unsigned port )
{
    return( inp( port ) );
}

static unsigned vid_inw( void *cx, unsigned port )
{
    return( inpw( port ) );
}

#ifndef QEMU
static unsigned long vid_ind( void *cx, unsigned port )
{
    return( inpd_asm( port ) );
}
#endif

#else

static unsigned long inpd(unsigned port )
{
  return( inpd_asm( port ) );
}

static void outpd( unsigned port, unsigned long val )
{
  outpd_asm( port, val );
}

#endif
