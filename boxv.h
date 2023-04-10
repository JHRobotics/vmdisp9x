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

/*
 * Public interface to the boxv library.
 */

/* PCI vendor and device IDs. */
#define BOXV_PCI_VEN    0x80EE
#define BOXV_PCI_DEV    0xBEEF

/* A structure describing mode information. Note: The mode numbers
 * usually match VBE, but this should not be relied upon.
 */
typedef struct {
    int     mode_no;        /* Mode number */
    int     xres;           /* Horizontal resolution */
    int     yres;           /* Vertical resolution */
    int     bpp;            /* Color depth */
} BOXV_mode_t;

extern void BOXV_mode_enumerate( void *cx, int (cb)( void *cx, BOXV_mode_t *mode ) );
extern int  BOXV_detect( void *cx, unsigned long *vram_size );
extern int  BOXV_ext_mode_set( void *cx, int xres, int yres, int bpp, int v_xres, int v_yres );
extern int  BOXV_mode_set( void *cx, int mode_no );
extern int  BOXV_dac_set( void *cx, unsigned start, unsigned count, void *pal );
extern int  BOXV_ext_disable( void *cx );
extern unsigned long BOXV_get_lfb_base( void *cx );

#define PCI_VENDOR_ID_VMWARE            0x15AD
#define PCI_DEVICE_ID_VMWARE_SVGA2      0x0405
