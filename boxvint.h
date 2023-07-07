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

/* Internal definitions used by the boxv library. */

/*-------------- VGA Specific ----------------*/

/* VGA I/O port addresses. */
#define VGA_CRTC            0x3D4   /* Color only! */
#define VGA_ATTR_W          0x3C0
#define VGA_ATTR_R          0x3C1
#define VGA_MISC_OUT_W      0x3C2
#define VGA_SEQUENCER       0x3C4
#define VGA_SEQUENCER_DATA  0x3C5
#define VGA_PIXEL_MASK      0x3C6
#define VGA_DAC_W_INDEX     0x3C8
#define VGA_DAC_DATA        0x3C9
#define VGA_MISC_OUT_R      0x3CC
#define VGA_GRAPH_CNTL      0x3CE
#define VGA_GRAPH_CNTL_DATA 0x3CF
#define VGA_STAT_ADDR       0x3DA   /* Color only! */

/* VGA Attribute Controller register indexes. */
#define VGA_AR_MODE         0x10
#define VGA_AR_OVERSCAN     0x11
#define VGA_AR_PLANE_EN     0x12
#define VGA_AR_PIX_PAN      0x13
#define VGA_AR_COLOR_SEL    0x14

/* VGA Graphics Controller register indexes. */
#define VGA_GR_SET_RESET    0x00
#define VGA_GR_DATA_ROTATE  0x03
#define VGA_GR_READ_MAP_SEL 0x04
#define VGA_GR_MODE         0x05
#define VGA_GR_MISC         0x06
#define VGA_GR_BIT_MASK     0x08

/* VGA Sequencer register indexes. */
#define VGA_SR_RESET        0x00
#define VGA_SR_CLK_MODE     0x01
#define VGA_SR_PLANE_MASK   0x02
#define VGA_SR_MEM_MODE     0x04

/* Sequencer constants. */
#define VGA_SR0_NORESET     0x03
#define VGA_SR0_RESET       0x00
#define VGA_SR1_BLANK       0x20

/* VGA CRTC register indexes. */
#define VGA_CR_HORZ_TOTAL   0x00
#define VGA_CR_CUR_START    0x0A
#define VGA_CR_CUR_END      0x0B
#define VGA_CR_START_HI     0x0C
#define VGA_CR_START_LO     0x0D
#define VGA_CR_CUR_POS_HI   0x0E
#define VGA_CR_CUR_POS_LO   0x0F
#define VGA_CR_VSYNC_START  0x10
#define VGA_CR_VSYNC_END    0x11

/* VGA Input Status Register 1 constants. */
#define VGA_STAT_VSYNC      0x08

/*------------ End VGA Specific --------------*/

/*------------- bochs Specific ---------------*/

#define VBE_DISPI_BANK_ADDRESS          0xA0000
#define VBE_DISPI_BANK_SIZE_KB          64

#define VBE_DISPI_MAX_XRES              1024
#define VBE_DISPI_MAX_YRES              768

#define VBE_DISPI_IOPORT_INDEX          0x01CE
#define VBE_DISPI_IOPORT_DATA           0x01CF

#define VBE_DISPI_INDEX_ID              0x0
#define VBE_DISPI_INDEX_XRES            0x1
#define VBE_DISPI_INDEX_YRES            0x2
#define VBE_DISPI_INDEX_BPP             0x3
#define VBE_DISPI_INDEX_ENABLE          0x4
#define VBE_DISPI_INDEX_BANK            0x5
#define VBE_DISPI_INDEX_VIRT_WIDTH      0x6
#define VBE_DISPI_INDEX_VIRT_HEIGHT     0x7
#define VBE_DISPI_INDEX_X_OFFSET        0x8
#define VBE_DISPI_INDEX_Y_OFFSET        0x9
#define VBE_DISPI_INDEX_VIDEO_MEMORY_64K  0xa
#define VBE_DISPI_INDEX_FB_BASE_HI      0xb

#define VBE_DISPI_ID0                   0xB0C0
#define VBE_DISPI_ID1                   0xB0C1
#define VBE_DISPI_ID2                   0xB0C2
#define VBE_DISPI_ID3                   0xB0C3
#define VBE_DISPI_ID4                   0xB0C4
#define VBE_DISPI_ID5                   0xB0C5
#define VBE_DISPI_ID6                   0xB0C6

#define VBE_DISPI_DISABLED              0x00
#define VBE_DISPI_ENABLED               0x01
#define VBE_DISPI_GETCAPS               0x02
#define VBE_DISPI_8BIT_DAC              0x20
#define VBE_DISPI_LFB_ENABLED           0x40
#define VBE_DISPI_NOCLEARMEM            0x80

/* Default LFB base. Might be different! */
#define VBE_DISPI_LFB_PHYSICAL_ADDRESS  0xE0000000

/*------------ End bochs Specific -------------*/


typedef unsigned char   v_byte;
typedef unsigned short  v_word;

/* A structure describing the contents of VGA registers for a mode set. */
typedef struct {
    v_byte          misc;           /* Miscellaneous register. */
    v_byte          seq[5];         /* Sequencer registers. */
    v_byte          crtc[25];       /* CRTC registers. */
    v_byte          gctl[9];        /* Graphics controller registers. */
    v_byte          atr[21];        /* Attribute registers. */
} v_vgaregs;

/* A structure fully describing a graphics or text mode. */
typedef struct {
    int             mode_no;        /* Internal mode number. */
    int             xres;           /* Horizontal (X) resolution. */
    int             yres;           /* Vertical (Y) resolution. */
    int             bpp;            /* Bits per pixel. */
    int             ext;            /* Non-zero for extended modes. */
    v_vgaregs      *vgaregs;       /* Contents of VGA registers. */
} v_mode;
