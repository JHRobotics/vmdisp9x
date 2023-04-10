/*****************************************************************************

Copyright (c) 2023  Jaroslav Hensl

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

unsigned long inpd_asm(unsigned short port);
#pragma aux inpd_asm =      \
    ".386"                  \
    "in eax, dx"            \
    parm [dx] value [eax];
    
void outpd_asm(unsigned short port, unsigned long val);
#pragma aux outpd_asm =     \
    ".386"                  \
    "out dx, eax"           \
    parm [dx] [eax];

unsigned short inpw_asm(unsigned short port);
#pragma aux inpw_asm =      \
    ".386"                  \
    "in ax, dx"             \
    parm [dx] value [ax];

void outpw_asm(unsigned short port, unsigned short val);
#pragma aux outpw_asm =     \
    ".386"                  \
    "out dx, ax"            \
    parm [dx] [ax];

unsigned char inpb_asm(unsigned short port);
#pragma aux inpb_asm =      \
    ".386"                  \
    "in al, dx"             \
    parm [dx] value [al];

void outp_asm(unsigned short port, unsigned char val);
#pragma aux outp_asm  =     \
    ".386"                  \
    "out dx, al"            \
    parm [dx] [al];

/*
 * Watcom complaining if have unused static function so, before include
 * this file define one or more folowing defines to specify which function
 * your code using:
 *   IO_IN32
 *   IN_OUT32
 *   IO_IN16
 *   IN_OUT16
 *   IO_IN8
 *   IN_OUT8
 *
 */
#ifdef IO_IN32
static unsigned long inpd(unsigned short port)
{
  return inpd_asm(port);
}
#endif

#ifdef IO_OUT32
static void outpd( unsigned short port, unsigned long val)
{
  outpd_asm(port, val);
}
#endif

#ifdef IO_IN16
static unsigned short inpw(unsigned short port)
{
  return inpw_asm(port);
}
#endif

#ifdef IO_OUT16
static void outpw( unsigned short port, unsigned short val)
{
  outpw_asm(port, val);
}
#endif

#ifdef IO_IN8
static unsigned char inp(unsigned short port)
{
  return inpb_asm(port);
}
#endif

#ifdef IO_OUT8
static void outp(unsigned short port, unsigned char val)
{
  outp_asm(port, val);
}
#endif
