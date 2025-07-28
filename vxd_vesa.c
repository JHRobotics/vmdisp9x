/**************************************************************************

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
#define VESA

#include "winhack.h"
#include "vmm.h"
#include "vxd.h"

#include "vxd_lib.h"
#include "3d_accel.h"

#include "boxvint.h" /* VGA regitry */

#include "pci.h" /* re-use PCI functions from SVGA */

#include "vesa.h"
#include "mtrr.h"

#define IO_IN8
#define IO_OUT8
#include "io32.h"

#include "code32.h"

#define ISA_LFB 0xE0000000UL

extern FBHDA_t *hda;
extern LONG fb_lock_cnt;
extern DWORD ThisVM;

static char vesa_vxd_name[] = "vesamini.vxd";

typedef struct vesa_mode
{
	DWORD width;
	DWORD height;
	DWORD bpp;
	DWORD pitch;
	DWORD phy;
	WORD  mode_id;
	WORD  flags;
} vesa_mode_t;

static vesa_mode_t *vesa_modes = NULL;
static DWORD vesa_modes_cnt = 0;
WORD vesa_version = 0;
static DWORD vesa_caps = 0;
static int act_mode = -1;

/* access rect */
static DWORD rect_left;
static DWORD rect_top;
static DWORD rect_right;
static DWORD rect_bottom;

extern BOOL vram_heap_in_ram;

/* vxd_mouse.vxd */
BOOL mouse_get_rect(DWORD *ptr_left, DWORD *ptr_top,
	DWORD *ptr_right, DWORD *ptr_bottom);

#include "vxd_strings.h"

void vesa_bios(CRS_32 *V86regs)
{
	// sizeof(CRS_32) = 108
	_asm    mov       ebx, [ThisVM]
	_asm    mov       ecx, [V86regs]
  /* save state */
	_asm    sub       esp, 108
	_asm    mov       edi, esp
	VMMCall(Save_Client_State);
	/* switch to V86regs */
	_asm   mov        esi, ecx
	VMMCall(Restore_Client_State)
	/* V86 INT */
	VMMCall(Begin_Nest_V86_Exec);
	_asm    mov       eax, 10h
	VMMCall(Exec_Int);
	VMMCall(End_Nest_Exec);
	/* copy result state to V86regs */
	_asm   mov       edi, ecx
	VMMCall(Save_Client_State);
	/* restore state regs */
	_asm    mov       esi, esp
	VMMCall(Restore_Client_State)
	_asm    add       esp, 108
}

void load_client_state(CRS_32 *V86regs)
{
//	_asm   mov       edi, [V86regs]
//	VMMCall(Save_Client_State);
	VMCB_t *vm = (VMCB_t*)ThisVM;
	memcpy(V86regs, vm->CB_Client_Pointer, sizeof(CRS_32));
}

static DWORD vesa_buf_v86;
static void *vesa_buf;
static DWORD vesa_buf_phy;
static vesa_palette_entry_t *vesa_pal;
static int vesa_pal_bits = 6;

static BOOL vesa_valid = FALSE;

static DWORD conf_dos_window = 0;
static DWORD conf_hw_double_buf = 0;

#define SCREEN_EMULATED_CENTER 0
#define SCREEN_EMULATED_COPY   1
#define SCREEN_FLIP            2
#define SCREEN_FLIP_VSYNC      3

static DWORD screen_mode = SCREEN_EMULATED_COPY;

#define MODE_OFFSET 1024
#define CRTC_OFFSET 2048
#define PAL_OFFSET  3072 // pal size = 4*256

#define V86_SEG(_lin) ((_lin) >> 4)
#define V86_OFF(_lin) ((_lin) & 0xF)

#define LIN_FROM_V86(_flatptr) ((((_flatptr) >> 12) & 0xFFFF0UL) + ((_flatptr) & 0xFFFFUL))

#define VESA_SUCC(_r) (((_r).Client_EAX & 0xFFFF)==0x004F)

static void offset_calc(DWORD offset, DWORD *ox, DWORD *oy)
{
	DWORD ps = ((hda->bpp+7)/8);
	*oy = offset/hda->pitch;
	*ox = (offset % hda->pitch)/ps;
}

static void alloc_modes_info(DWORD cnt)
{
	DWORD vesa_modes_pages = ((sizeof(vesa_mode_t) * cnt) + P_SIZE - 1) / P_SIZE;
	vesa_modes = (vesa_mode_t*)_PageAllocate(vesa_modes_pages, PG_SYS, 0, 0x0, 0, 0x100000, NULL, PAGEZEROINIT);
	dbg_printf("vesa_modes = %lX\n", vesa_modes);
	vesa_modes_cnt = 0;
}

static char VESA_conf_path[] = "Software\\vmdisp9x\\vesa";

BOOL VESA_init_hw()
{
	DWORD flat;
	DWORD conf_vram_limit = 128;
	DWORD conf_mtrr = 1;
	DWORD conf_no_memtest = 0;
	
	dbg_printf("VESA init begin...\n");

	RegReadConf(HKEY_LOCAL_MACHINE, VESA_conf_path, "VRAMLimit",        &conf_vram_limit);
	RegReadConf(HKEY_LOCAL_MACHINE, VESA_conf_path, "MTRR",             &conf_mtrr);
	RegReadConf(HKEY_LOCAL_MACHINE, VESA_conf_path, "DosWindowSetMode", &conf_dos_window);
	RegReadConf(HKEY_LOCAL_MACHINE, VESA_conf_path, "HWDoubleBuffer",   &conf_hw_double_buf);
	RegReadConf(HKEY_LOCAL_MACHINE, VESA_conf_path, "NoMemTest",        &conf_no_memtest);

	flat = _PageAllocate(1, PG_SYS, 0, 0x0, 0, 0x100000, &vesa_buf_phy, PAGEUSEALIGN | PAGECONTIG | PAGEFIXED);
	vesa_buf = (void*)flat;

	if(vesa_buf)
	{
		DWORD modes_count = 0;
		DWORD v86_page = 0xB0; /* Is this OK? Any collisions? */

		if(_PhysIntoV86(vesa_buf_phy >> 12, ThisVM, v86_page, 1, 0))
		{
			vesa_buf_v86 = v86_page*4096;
		}
		dbg_printf("VESA V86 addr=0x%lX, phy=0x%lX\n", vesa_buf_v86, vesa_buf_phy);

		if(vesa_buf_v86)
		{
			DWORD i;
			DWORD fb_phy = 0xFFFFFFFFF;
			CRS_32 regs;
			vesa_info_block_t *info = vesa_buf;
			vesa_mode_info_t *modeinfo = (vesa_mode_info_t*)((char*)vesa_buf + MODE_OFFSET);

			memset(info,     0, sizeof(vesa_info_block_t));
			memset(modeinfo, 0, sizeof(vesa_mode_info_t));
			memcpy(&info->VESASignature[0], "VBE2", 4);

			load_client_state(&regs);
			regs.Client_EAX = VESA_CMD_ADAPTER_INFO;
			regs.Client_ES  = V86_SEG(vesa_buf_v86);
			regs.Client_EDI = V86_OFF(vesa_buf_v86);

			vesa_bios(&regs);

			if((regs.Client_EAX & 0xFFFF) == 0x004F &&
				memcmp(info->VESASignature, "VESA", 4) == 0)
			{
				WORD *modes = NULL;

				dbg_printf("VESA bios result=%X, vesa_version=%X, modes_ptr=%lX vesa_caps=%lX\n",
					regs.Client_EAX & 0xFFFF, info->VESAVersion, info->VideoModePtr, info->Capabilities);

				if(info->VESAVersion < VESA_VBE_2_0)
				{
					dbg_printf("We need VBE 2.0 as minimum, abort\n");
					return FALSE;
				}

				modes = (WORD*)LIN_FROM_V86(info->VideoModePtr);
				if(modes != NULL)
				{
					while(*modes != 0xFFFF)
					{
						//dbg_printf("modes: 0x%04X\n", *modes);
						modes++;
						modes_count++;
					}
				}

				alloc_modes_info(modes_count);
				modes = (WORD*)LIN_FROM_V86(info->VideoModePtr);

				for(i = 0; i < modes_count; i++)
				{
					regs.Client_EAX = VESA_CMD_MODE_INFO;
					regs.Client_ECX = modes[i];
					regs.Client_ES  = V86_SEG(vesa_buf_v86+MODE_OFFSET);
					regs.Client_EDI = V86_OFF(vesa_buf_v86+MODE_OFFSET);

					vesa_bios(&regs);

					//dbg_printf("mode=%X atrs=0x%lX eax=0x%lX\n", modes[i], modeinfo->ModeAttributes, regs.Client_EAX);
					if(VESA_SUCC(regs))
					{
						if((modeinfo->ModeAttributes &
							(VESA_MODE_HW_SUPPORTED | VESA_MODE_COLOR | VESA_MODE_GRAPHICS | VESA_MODE_LFB)) ==
							(VESA_MODE_HW_SUPPORTED | VESA_MODE_COLOR | VESA_MODE_GRAPHICS | VESA_MODE_LFB))
						{
							vesa_mode_t *m = &vesa_modes[vesa_modes_cnt];
							m->width   = modeinfo->XResolution;
							m->height  = modeinfo->YResolution;
							m->bpp     = modeinfo->BitsPerPixel;
							m->pitch   = modeinfo->BytesPerScanLine;
							m->phy     = modeinfo->PhysBasePtr;
							m->mode_id = modes[i];
							m->flags   = modeinfo->ModeAttributes;
							vesa_modes_cnt++;

							if(m->phy)
							{
								if(m->phy < fb_phy)
								{
									fb_phy = m->phy;
								}
							}

							dbg_printf("Mode 0x%X = (%ld x %ld x %ld) = phy:%lX\n",
								m->mode_id, m->width, m->height, m->bpp, m->phy);
						}
					}
				} // for

				if(vesa_modes_cnt == 0)
				{
					return FALSE;
				}

			 	if(!FBHDA_init_hw())
			 	{
			 		return FALSE;
			 	}

				vesa_version = info->VESAVersion;
				vesa_caps = info->Capabilities;

		 		memcpy(hda->vxdname, vesa_vxd_name, sizeof(vesa_vxd_name));
				hda->vram_size = info->TotalMemory * 0x10000UL;
				if(hda->vram_size > conf_vram_limit*1024*1024)
				{
					hda->vram_size = conf_vram_limit*1024*1024;
				}

				hda->vram_bar_size = hda->vram_size;

				if(fb_phy == 0xFFFFFFFF)
				{
					fb_phy = ISA_LFB;
				}

				hda->vram_pm32 = (void*)_MapPhysToLinear(fb_phy, hda->vram_size, 0);

				if(conf_mtrr)
				{
					if(fb_phy > 1*1024*1024)
					{
						if(MTRR_GetVersion())
						{
							dbg_printf("MTRR supported\n");
							MTRR_SetPhysicalCacheTypeRange(fb_phy, 0, hda->vram_size, MTRR_FRAMEBUFFERCACHED);
						}
						else
						{
							dbg_printf("MTRR unsupported\n");
						}
					}
				}

				hda->flags |= FB_SUPPORT_FLIPING | FB_VESA_MODES;

				vesa_pal = (vesa_palette_entry_t*)(((BYTE*)vesa_buf)+PAL_OFFSET);

				vesa_valid  = TRUE;

				//VESA_load_vbios_pm();
				
				if(!conf_no_memtest)
				{
					FBHDA_memtest();
				}

				dbg_printf("VESA_init_hw(vram_size=%ld) = TRUE\n", hda->vram_size);
				
				return TRUE;
			}
		}
	}
	return FALSE;
}

BOOL VESA_valid()
{
	return vesa_valid;
}

BOOL VESA_validmode(DWORD w, DWORD h, DWORD bpp)
{
	DWORD i;

	for(i = 0; i < vesa_modes_cnt; i++)
	{
		if(vesa_modes[i].width == w && vesa_modes[i].height == h && vesa_modes[i].bpp == bpp)
		{
			return TRUE;
		}
	}
	//dbg_printf("fail to valid mode: %ld %ld %ld\n", w, h, bpp);
	return FALSE;
}

void VESA_clear()
{
	memset((BYTE*)hda->vram_pm32+hda->system_surface, 0, hda->pitch*hda->height);
}

BOOL VESA_setmode_phy(DWORD w, DWORD h, DWORD bpp, DWORD rr_min, DWORD rr_max)
{
	DWORD i;

	for(i = 0; i < vesa_modes_cnt; i++)
	{
		if(vesa_modes[i].width == w && vesa_modes[i].height == h && vesa_modes[i].bpp == bpp)
		{
			CRS_32 regs;
			VESA_HIRES_enable();

			load_client_state(&regs);

			// set mode
			regs.Client_EAX = VESA_CMD_MODE_SET;
			regs.Client_EBX = vesa_modes[i].mode_id | VESA_SETMODE_LFB | VESA_SETMODE_NOCLEAR;
			regs.Client_ES = 0;
			regs.Client_EDI = 0;

			vesa_bios(&regs);
			if(VESA_SUCC(regs))
			{
				DWORD test_x;
				DWORD test_y;

				dbg_printf("SET success: %d %d %d\n", w, h, bpp);
				hda->width  = w;
				hda->height = h;
				hda->bpp    = bpp;
				hda->pitch  = vesa_modes[i].pitch;
				hda->stride = h * hda->pitch;
				hda->surface = 0;

				/* set DAC to 8BIT if supported */
				if(bpp <= 8)
				{
					if((vesa_caps & VESA_CAP_DAC8BIT) != 0)
					{
						/* set DAC to 8 bpp */
						regs.Client_EAX = VESA_CMD_PALETTE_FORMAT;
						regs.Client_EBX = VESA_DAC_SETFORMAT | VESA_DAC_SET_8BIT;
						vesa_bios(&regs);
						if(VESA_SUCC(regs))
						{
							vesa_pal_bits = (regs.Client_EBX >> 8) & 0xFF;
						}
						else
						{
							vesa_pal_bits = 6;
						}
					}
					else
					{
						vesa_pal_bits = 6;
					}
				}
				screen_mode = SCREEN_EMULATED_COPY;

				if(conf_hw_double_buf > 0)
				{
					if(conf_hw_double_buf >= 2)
					{
						/* test if HW flip and vtrace supported */
						offset_calc(hda->stride, &test_x, &test_y);
						regs.Client_EAX = VESA_CMD_DISPLAY_START;
						regs.Client_EBX = VESA_DISPLAYSTART_VTRACE;
						regs.Client_ECX = test_x;
						regs.Client_EDX = test_y;
						vesa_bios(&regs);
						if(VESA_SUCC(regs))
						{
							screen_mode = SCREEN_FLIP_VSYNC;
		
							regs.Client_EAX = VESA_CMD_DISPLAY_START;
							regs.Client_EBX = VESA_DISPLAYSTART_SET;
							regs.Client_ECX = 0;
							regs.Client_EDX = 0;
							vesa_bios(&regs);
						}
					}

					if(screen_mode <= SCREEN_EMULATED_COPY)
					{
						/* test if HW flip supported */
						offset_calc(hda->stride, &test_x, &test_y);
						regs.Client_EAX = VESA_CMD_DISPLAY_START;
						regs.Client_EBX = VESA_DISPLAYSTART_SET;
						regs.Client_ECX = test_x;
						regs.Client_EDX = test_y;
						vesa_bios(&regs);
	
						if(VESA_SUCC(regs))
						{
							screen_mode = SCREEN_FLIP;
	
							regs.Client_EAX = VESA_CMD_DISPLAY_START;
							regs.Client_EBX = VESA_DISPLAYSTART_SET;
							regs.Client_ECX = 0;
							regs.Client_EDX = 0;
							vesa_bios(&regs);
						}
					}
				}

				if(screen_mode <= SCREEN_EMULATED_COPY)
				{
					hda->system_surface = hda->stride;
					hda->surface = hda->stride;
					//hda->system_surface = 0;
					//hda->flags &= ~FB_SUPPORT_VSYNC;
				}
				else
				{
					hda->system_surface = 0;
					if(screen_mode == SCREEN_FLIP_VSYNC)
					{
						hda->flags |= FB_SUPPORT_VSYNC;
					}
				}

				FBHDA_update_heap_size(FALSE, vram_heap_in_ram);

				act_mode = i;

				dbg_printf("mode set, mode_id=%d, screen_mode=%d\n", i, screen_mode);
				return TRUE;
			}
			else
			{
				dbg_printf("vbios fail: eax=0x%lX\n", regs.Client_EAX);
			}
		}
	} // for

	dbg_printf("fail to set %ld %ld %ld\n", w, h, bpp);
	return FALSE;
}

BOOL VESA_setmode(DWORD w, DWORD h, DWORD bpp, DWORD rr_min, DWORD rr_max)
{
	if(VESA_setmode_phy(w, h, bpp, rr_min, rr_max))
	{
		VESA_clear();
		mouse_invalidate();
		return TRUE;
	}
	return FALSE;
}

void FBHDA_palette_set(unsigned char index, DWORD rgb)
{
//	dbg_printf("PAL: %d:0x%lX,bpp:%d\n", index, rgb, vesa_pal_bits);

	if((vesa_caps & VESA_CAP_NONVGA) == 0)
	{
		/* if mode is VGA compatible, we can use VGA regitry */
		outp(VGA_DAC_W_INDEX, index);    /* Set starting index. */
		if(vesa_pal_bits == 8)
		{
			outp(VGA_DAC_DATA,   (rgb >> 16) & 0xFF);
			outp(VGA_DAC_DATA,   (rgb >>  8) & 0xFF);
			outp(VGA_DAC_DATA,    rgb        & 0xFF);
		}
		else
		{
			outp(VGA_DAC_DATA,   (rgb >> 18) & 0x3F);
			outp(VGA_DAC_DATA,   (rgb >> 10) & 0x3F);
			outp(VGA_DAC_DATA,   (rgb >>  2) & 0x3F);
		}
	}
	else
	{
		CRS_32 regs;
		DWORD v86_ptr = vesa_buf_v86+PAL_OFFSET+4*index;

		if(vesa_pal_bits == 8)
		{
			vesa_pal[index].Red     = (rgb >> 16) & 0xFF;
			vesa_pal[index].Green   = (rgb >>  8) & 0xFF;
			vesa_pal[index].Blue    =  rgb        & 0xFF;
		}
		else
		{
			vesa_pal[index].Red     = (rgb >> 18) & 0x3F;
			vesa_pal[index].Green   = (rgb >> 10) & 0x3F;
			vesa_pal[index].Blue    = (rgb >>  2) & 0x3F;
		}

		load_client_state(&regs);
		regs.Client_EAX = VESA_CMD_PALETTE_DATA;
		regs.Client_EBX = VESA_RAMDAC_DATA_SET;
		regs.Client_ECX = 1;
		regs.Client_EDX = index;
		regs.Client_ES  = V86_SEG(v86_ptr);
		regs.Client_EDI = V86_OFF(v86_ptr);
		vesa_bios(&regs);
	}
}

DWORD FBHDA_palette_get(unsigned char index)
{
	if((vesa_caps & VESA_CAP_NONVGA) == 0)
	{
		DWORD r, g, b;
		outp(VGA_DAC_W_INDEX, index);
		r = inp(VGA_DAC_DATA);
		g = inp(VGA_DAC_DATA);
		b = inp(VGA_DAC_DATA);

		if(vesa_pal_bits == 8)
		{
			return (r << 16) | (g << 8) | b;
		}
		else
		{
			return (r << 18) | (g << 10) | (b << 2);
		}
	}
	else
	{
		if(vesa_pal_bits == 8)
		{
			return
				((DWORD)(vesa_pal[index].Red)   << 16) |
				((DWORD)(vesa_pal[index].Green) <<  8) |
				 (DWORD)(vesa_pal[index].Blue);
		}
		else
		{
			return
				((DWORD)(vesa_pal[index].Red)   << 18) |
				((DWORD)(vesa_pal[index].Green) << 10) |
				((DWORD)(vesa_pal[index].Blue)  << 2);
		}
	}

	return 0;
}

BOOL FBHDA_swap(DWORD offset)
{
	if((offset + hda->stride) < hda->vram_size && offset >= hda->system_surface)
	{
		switch(screen_mode)
		{
			case SCREEN_EMULATED_CENTER:
			case SCREEN_EMULATED_COPY:
				FBHDA_access_begin(0);
				hda->surface = offset;
				FBHDA_access_end(0);
				break;
			case SCREEN_FLIP:
			case SCREEN_FLIP_VSYNC:
			{
				CRS_32 regs;
				DWORD off_x;
				DWORD off_y;
				if(fb_lock_cnt == 0)
				{
					mouse_erase();
				}
				offset_calc(offset, &off_x, &off_y);
				load_client_state(&regs);

				regs.Client_EAX = VESA_CMD_DISPLAY_START;
				regs.Client_EBX =
					(screen_mode == SCREEN_FLIP_VSYNC) ? VESA_DISPLAYSTART_VTRACE : VESA_DISPLAYSTART_SET;
				regs.Client_ECX = off_x;
				regs.Client_EDX = off_y;
				hda->surface = offset;
				vesa_bios(&regs);

				if(fb_lock_cnt == 0)
				{
					mouse_blit();
				}
				break;
			}
		}
		return TRUE;
	}
	return FALSE;
}

static inline void update_rect(DWORD left, DWORD top, DWORD right, DWORD bottom)
{
	if(left < rect_left)
		rect_left = left;

	if(top < rect_top)
		rect_top = top;

	if(right > rect_right)
		rect_right = right;

	if(bottom > rect_bottom)
		rect_bottom = bottom;
}

void FBHDA_access_begin(DWORD flags)
{
	//Wait_Semaphore(hda_sem, 0);
	if(fb_lock_cnt++ == 0)
	{
		if(flags & FBHDA_ACCESS_MOUSE_MOVE)
		{
			if(!mouse_get_rect(&rect_left, &rect_top, &rect_right, &rect_bottom))
			{
				rect_left   = 0;
				rect_top    = 0;
				rect_right  = 0;
				rect_bottom = 0;
			}
		}
		else
		{
			rect_left   = 0;
			rect_top    = 0;
			rect_right  = hda->width;
			rect_bottom = hda->height;
		}
		mouse_erase();
	}
	else
	{
		DWORD l, t, r, b;

		if(mouse_get_rect(&l, &t, &r, &b))
		{
			update_rect(l, t, r, b);
		}
	}
}

void FBHDA_access_rect(DWORD left, DWORD top, DWORD right, DWORD bottom)
{
	if(fb_lock_cnt++ == 0)
	{
		mouse_erase();
		rect_left   = left;
		rect_top    = top;
		rect_right  = right;
		rect_bottom = bottom;
	}
	else
	{
		update_rect(left, top, right, bottom);
	}
}

static void VESA_copy_rect()
{
	if((rect_right > rect_left) && (rect_bottom > rect_top))
	{
		DWORD y;
		DWORD bs = (hda->bpp + 7) >> 3;
		DWORD line_size = (rect_right - rect_left) * bs;
		DWORD line_start = rect_left * bs;
		BYTE *dst = (BYTE*)hda->vram_pm32;
		BYTE *src = ((BYTE*)hda->vram_pm32) + hda->surface;

		dst += (rect_top * hda->pitch) + line_start;
		src += (rect_top * hda->pitch) + line_start;

		for(y = rect_top; y < rect_bottom; y++)
		{
			memcpy(dst, src, line_size);
			dst += hda->pitch;
			src += hda->pitch;
		}
	}
}

void FBHDA_access_end(DWORD flags)
{
	if(flags & FBHDA_ACCESS_MOUSE_MOVE)
	{
		DWORD l, t, r, b;
		
		if(mouse_get_rect(&l, &t, &r, &b))
		{
			update_rect(l, t, r, b);
		}
	}

	fb_lock_cnt--;
	if(fb_lock_cnt < 0) fb_lock_cnt = 0;

	if(fb_lock_cnt == 0)
	{
		mouse_blit();
		switch(screen_mode)
		{
			case SCREEN_EMULATED_CENTER:
			case SCREEN_EMULATED_COPY:
			{
				if(rect_left > hda->width)
					rect_left = 0;

				if(rect_top > hda->height)
					rect_top = 0;

				if(rect_right > hda->width)
					rect_right = hda->width;

				if(rect_bottom > hda->height)
					rect_bottom = hda->height;

				if(rect_left == 0 && rect_top == 0 &&
					rect_right == hda->width && rect_bottom == hda->height)
				{
					memcpy(hda->vram_pm32, ((BYTE*)hda->vram_pm32)+hda->surface, hda->stride);
				}
				else
				{
					VESA_copy_rect();
				}

				break;
			}
		}
		// cursor
	}

	//Signal_Semaphore(hda_sem);
}

DWORD FBHDA_overlay_setup(DWORD overlay, DWORD width, DWORD height, DWORD bpp)
{
	return 0;
}

void FBHDA_overlay_lock(DWORD left, DWORD top, DWORD right, DWORD bottom)
{

}

void  FBHDA_overlay_unlock(DWORD flags)
{

}

static BOOL vga_mode = TRUE;

void VESA_HIRES_enable()
{
	dbg_printf("VESA_HIRES_enable\n");
	vga_mode = FALSE;
}

void VESA_HIRES_disable()
{
	dbg_printf("VESA_HIRES_disable\n");
	vga_mode = TRUE;
}

extern BOOL mode_changing;

BOOL FBHDA_mode_query(DWORD index, FBHDA_mode_t *mode)
{
	if(index >= vesa_modes_cnt)
	{
		return FALSE;
	}

	if(mode)
	{
		mode->cb = sizeof(FBHDA_mode_t);
		mode->width = vesa_modes[index].width;
		mode->height = vesa_modes[index].height;
		mode->bpp = vesa_modes[index].bpp;
		mode->refresh = 0;
	}

	return TRUE;
}

static BYTE saved_vga_mode = 0xFF;

BOOL VESA_check_int10h(DWORD vm, PCRS_32 regs)
{
	if(vm != ThisVM)
	{
		BYTE ah = (regs->Client_EAX >> 8) & 0xFF;
		BYTE al =  regs->Client_EAX & 0xFF;
		if(ah == 0)
		{
			if(!vga_mode && conf_dos_window == 0)
			{
				saved_vga_mode = al & 0x7F;
				return FALSE;
			}
		}

		if(ah == 0x4F)
		{
			if(al == 0x02) /* mode set */
			{
				if(!vga_mode && conf_dos_window == 0)
				{
					regs->Client_EAX &= 0xFFFF;
					regs->Client_EAX |= 0x034F; /* Function call invalid in current video mode */
					return FALSE;
				}
			}
		}
	}
	return TRUE;
}

#if 0
/* NOTE: idea was forbind switch between window and full screen DOS mode,
         but this currently useless because windows only checking if can out
         but not if can in 
 */
static BOOL is_text_mode(DWORD mode)
{
	switch(mode)
	{
		case 00:
		case 01:
		case 03:
		case 07:
			return TRUE;
	}
	return FALSE;
}

BOOL VESA_check_switch(DWORD mode)
{
	if(vga_mode)
	{
		if(!is_text_mode(mode))
		{
			return FALSE;
		}
	}
	else
	{
		if(is_text_mode(mode))
		{
			return FALSE;
		}
	}
	return TRUE;
}

#else

BOOL VESA_check_switch(DWORD mode)
{
	return TRUE;
}

#endif
