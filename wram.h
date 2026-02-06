/*****************************************************************************

Copyright (c) 2025 Jaroslav Hensl <emulator@emulace.cz>

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
         Memory layout
+-----------------------------+ <-- WRAM top
| Palette (256 * 4)           |
|-----------------------------| <--  1024 (P=0)
| Gamma table (256 * 3)       |
|-----------------------------| <--  1792 (P=0)
| Registry (64*4)             |
|-----------------------------| <--  2048 (P=0)
| Pad (2048)                  |
|-----------------------------| <--  4096 (P=0)
| Cursor                      |
| AND (32 x 32 x 4)           | <--  8192 (P=1)
| XOR (32 x 32 x 4)           | <-- 12288 (P=2)
|-----------------------------| <-- FB top (0x3000)
| Frame buffer 0              |
|                             |
|                             |
|                             |
|-----------------------------|
| Another framebuffer /       |
| textures                    |
|                             |
|                             |
|-----------------------------|
| ....                        |
|-----------------------------| <-- SYSTEM max
| Alternate screen 1-8 (opt)  |
|                             |
|                             |
+-----------------------------+ <-- WRAM max

*/
#ifndef __WRAM_H__INCLUDED__
#define __WRAM_H__INCLUDED__

#pragma pack(push)
#pragma pack(1)

typedef union _flat_t
{
	DWORD  dw;
	void   *ptr;
	BYTE   *u8;
} flat_t;

typedef union _pixel_32_t
{
	DWORD dw;
	BYTE  b[4];
	struct
	{
		BYTE b;
		BYTE g;
		BYTE r;
		BYTE x;
	} c;
} pixel_32_t;

typedef DWORD pixel_16_t;
typedef BYTE pixel_8_t;

typedef union _bitmap_ptr_t
{
	pixel_32_t *px32;
	pixel_16_t *px16;
	pixel_8_t *px8;
	flat_t flat;
} bitmap_ptr_t;

#define CURSOR_DMAX 32
#define CURSOR_CB (CURSOR_DMAX*CURSOR_DMAX*sizeof(DWORD))

#define WRAM_FB_TOP 0x3000

#define MODE_32 0
#define MODE_24 1
#define MODE_16 2
#define MODE_15 3
#define MODE_8  4

typedef struct _wram_t
{
	pixel_32_t palette[256];
	struct
	{
		BYTE red[256];
		BYTE green[256];
		BYTE blue[256];
	} gamma;
	union
	{
		DWORD dw[64];
		struct
		{
			DWORD fbstart; /* byte offset from WRAM top */
			DWORD size; /* size of entire memory */
			DWORD width;
			DWORD height;
			DWORD mode;
			DWORD pitch;
			DWORD cursor_visible;
			DWORD cursor_empty;
			LONG  cursor_x;
			LONG  cursor_y;
			LONG  cursor_w;
			LONG  cursor_h;
			LONG  cursor_spotx;
			LONG  cursor_spoty;
			DWORD gamma_enable;
			DWORD fbmin;
			DWORD fbmax;
		} s;
	} regs;
	BYTE extradata[2048]; // current used for fbhda
	struct
	{
		DWORD andmask[CURSOR_DMAX*CURSOR_DMAX];
		DWORD xormask[CURSOR_DMAX*CURSOR_DMAX];
	} cursor;
} wram_t;

#define BLIT_MAX_BUFS 3

typedef struct _blit_t
{
	flat_t base;
	bitmap_ptr_t dst[BLIT_MAX_BUFS];
	DWORD dst_mode;
	DWORD dst_w;
	DWORD dst_h;
	DWORD dst_padx;
	DWORD dst_pady;
	DWORD dst_pitch;
	DWORD dst_scans;
	DWORD src_sx;
	DWORD src_ex;
	DWORD src_sy;
	DWORD src_ey;
	DWORD num_changes;
} blit_t;

#pragma pack(pop)

extern wram_t *wram;

BOOL wram_init(DWORD bytes);
void wram_blit(blit_t *blit);
void wram_clear_target(blit_t *blit);
void wram_clear();
BOOL wram_swap(void *virtptr, blit_t *blit);
void wram_changes(blit_t *blit, DWORD sx, DWORD sy, DWORD ex, DWORD ey);

#endif /* __WRAM_H__INCLUDED__ */
