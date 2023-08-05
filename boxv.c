/*****************************************************************************

Copyright (c) 2012-2022  Michal Necasek
                   2023  Philip Kelley

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

/* Core boxv implementation. */

#include "boxv.h"       /* Public interface. */
#include "boxvint.h"    /* Implementation internals. */
#define NEEDVIDEO
#include "io.h"    /* I/O access layer, host specific. */

#pragma code_seg( _INIT )

/* Write a single value to an indexed register at a specified
 * index. Suitable for the CRTC or graphics controller.
 */
inline void vid_wridx( void *cx, int idx_reg, int idx, v_byte data )
{
    vid_outw( cx, idx_reg, idx | (data << 8) );
}

/* Set an extended non-VGA mode with given parameters. 8bpp and higher only.
 * Returns non-zero value on failure.
 */
int BOXV_ext_mode_set( void *cx, int xres, int yres, int bpp, int v_xres, int v_yres )
{
    /* Do basic parameter validation. */
    if( v_xres < xres || v_yres < yres )
        return( -1 );

    /* Put the hardware into a state where the mode can be safely set. */
    vid_inb( cx, VGA_STAT_ADDR );                   /* Reset flip-flop. */
    vid_outb( cx, VGA_ATTR_W, 0 );                  /* Disable palette. */
    vid_wridx( cx, VGA_SEQUENCER, VGA_SR_RESET, VGA_SR_RESET );

    /* Disable the extended display registers. */
    vid_outw( cx, VBE_DISPI_IOPORT_INDEX, VBE_DISPI_INDEX_ENABLE );
    vid_outw( cx, VBE_DISPI_IOPORT_DATA, VBE_DISPI_DISABLED );

    /* Program the extended non-VGA registers. */

    /* Set X resoultion. */
    vid_outw( cx, VBE_DISPI_IOPORT_INDEX, VBE_DISPI_INDEX_XRES );
    vid_outw( cx, VBE_DISPI_IOPORT_DATA, xres );
    /* Set Y resoultion. */
    vid_outw( cx, VBE_DISPI_IOPORT_INDEX, VBE_DISPI_INDEX_YRES );
    vid_outw( cx, VBE_DISPI_IOPORT_DATA, yres );
    /* Set bits per pixel. */
    vid_outw( cx, VBE_DISPI_IOPORT_INDEX, VBE_DISPI_INDEX_BPP );
    vid_outw( cx, VBE_DISPI_IOPORT_DATA, bpp );
    /* Set the virtual resolution. */
    vid_outw( cx, VBE_DISPI_IOPORT_INDEX, VBE_DISPI_INDEX_VIRT_WIDTH );
    vid_outw( cx, VBE_DISPI_IOPORT_DATA, v_xres );
    vid_outw( cx, VBE_DISPI_IOPORT_INDEX, VBE_DISPI_INDEX_VIRT_HEIGHT );
    vid_outw( cx, VBE_DISPI_IOPORT_DATA, v_yres );
    /* Reset the current bank. */
    vid_outw( cx, VBE_DISPI_IOPORT_INDEX, VBE_DISPI_INDEX_BANK );
    vid_outw( cx, VBE_DISPI_IOPORT_DATA, 0 );
    /* Set the X and Y display offset to 0. */
    vid_outw( cx, VBE_DISPI_IOPORT_INDEX, VBE_DISPI_INDEX_X_OFFSET );
    vid_outw( cx, VBE_DISPI_IOPORT_DATA, 0 );
    vid_outw( cx, VBE_DISPI_IOPORT_INDEX, VBE_DISPI_INDEX_Y_OFFSET );
    vid_outw( cx, VBE_DISPI_IOPORT_DATA, 0 );
    /* Enable the extended display registers. */
    vid_outw( cx, VBE_DISPI_IOPORT_INDEX, VBE_DISPI_INDEX_ENABLE );
#ifdef QEMU
    vid_outw( cx, VBE_DISPI_IOPORT_DATA, VBE_DISPI_ENABLED | VBE_DISPI_8BIT_DAC | VBE_DISPI_LFB_ENABLED | VBE_DISPI_NOCLEARMEM  );
#else
    vid_outw( cx, VBE_DISPI_IOPORT_DATA, VBE_DISPI_ENABLED | VBE_DISPI_8BIT_DAC );
#endif

    /* Re-enable the sequencer. */
    vid_wridx( cx, VGA_SEQUENCER, VGA_SR_RESET, VGA_SR0_NORESET );

    /* Reset flip-flop again and re-enable palette. */
    vid_inb( cx, VGA_STAT_ADDR );
    vid_outb( cx, VGA_ATTR_W, 0x20 );

    return( 0 );
}

/* Program the DAC. Each of the 'count' entries is 4 bytes in size,
 * red/green/blue/unused.
 * Returns non-zero on failure.
 */
int BOXV_dac_set( void *cx, unsigned start, unsigned count, void *pal )
{
    v_byte      *prgbu = pal;

    /* Basic argument validation. */
    if( start + count > 256 )
        return( -1 );

    /* Write the starting index. */
    vid_outb( cx, VGA_DAC_W_INDEX, start );
    /* Load the RGB data. */
    while( count-- ) {
        vid_outb( cx, VGA_DAC_DATA, *prgbu++ );
        vid_outb( cx, VGA_DAC_DATA, *prgbu++ );
        vid_outb( cx, VGA_DAC_DATA, *prgbu++ );
        ++prgbu;
    }
    return( 0 );
}

/* Detect the presence of a supported adapter and amount of installed
 * video memory. Returns zero if not found.
 */
int BOXV_detect( void *cx, unsigned long *vram_size )
{
    v_word      boxv_id;

    vid_outw( cx, VBE_DISPI_IOPORT_INDEX, VBE_DISPI_INDEX_ID );

    boxv_id = vid_inw( cx, VBE_DISPI_IOPORT_DATA );
#ifdef QEMU
    if( boxv_id < VBE_DISPI_ID0 || boxv_id > VBE_DISPI_ID6 )
        return( 0 );

    if( vram_size ) {
        vid_outw( cx, VBE_DISPI_IOPORT_INDEX, VBE_DISPI_INDEX_VIDEO_MEMORY_64K );
        *vram_size = (unsigned long)vid_inw( cx, VBE_DISPI_IOPORT_DATA ) << 16;
    }

    return( boxv_id );
#else
    if( vram_size ) {
        *vram_size = vid_ind( cx, VBE_DISPI_IOPORT_DATA );
    }

    if( boxv_id >= VBE_DISPI_ID0 && boxv_id <= VBE_DISPI_ID4 )
        return( boxv_id );
    else
        return( 0 );
#endif
}

/* Disable extended mode and place the hardware into a VGA compatible state.
 * Returns non-zero on failure.
 */
int BOXV_ext_disable( void *cx )
{
    /* Disable the extended display registers. */
    vid_outw( cx, VBE_DISPI_IOPORT_INDEX, VBE_DISPI_INDEX_ENABLE );
    vid_outw( cx, VBE_DISPI_IOPORT_DATA, VBE_DISPI_DISABLED );
    return( 0 );
}

/* Return the physical base address of the framebuffer. Needed in environments
 * that do not query the base through PCI.
 */
unsigned long BOXV_get_lfb_base( void *cx )
{
    unsigned long   fb_base;

    /* Ask the virtual hardware for the high 16 bits. */
    vid_outw( cx, VBE_DISPI_IOPORT_INDEX, VBE_DISPI_INDEX_FB_BASE_HI );
    fb_base = vid_inw( cx, VBE_DISPI_IOPORT_DATA );

    /* Old versions didn't support that, so use the default
     * if the value looks like garbage.
     */
    if( fb_base != 0 && fb_base != 0xffff )
        fb_base = fb_base << 16;
    else
        fb_base = VBE_DISPI_LFB_PHYSICAL_ADDRESS;

    return( fb_base );
}
