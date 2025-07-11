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

#include "boxvint.h"

#include "pci.h" /* re-use PCI functions from SVGA */
#include "vesa.h"

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
} vesa_mode_t;

vesa_mode_t *vesa_modes = NULL;
DWORD vesa_modes_cnt = 0;

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
	_asm   mov       edi, [V86regs]
	VMMCall(Save_Client_State);
}

DWORD vesa_buf_flat = 0;
DWORD vesa_buf_v86;
void *vesa_buf;

#define MODE_OFFSET 1024
#define CRTC_OFFSET 2048

#define V86_SEG(_lin) ((_lin) >> 4)
#define V86_OFF(_lin) ((_lin) & 0xF)

#define LIN_FROM_V86(_flatptr) ((((_flatptr) >> 12) & 0xFFFF0UL) + ((_flatptr) & 0xFFFFUL))

static BOOL vesa_valid = FALSE;

static void alloc_modes_info(DWORD cnt)
{
	DWORD vesa_modes_pages = ((sizeof(vesa_mode_t) * cnt) + P_SIZE - 1) / P_SIZE;
	vesa_modes = (vesa_mode_t*)_PageAllocate(vesa_modes_pages, PG_SYS, 0, 0x0, 0, 0x100000, NULL, PAGEZEROINIT);
	dbg_printf("vesa_modes = %lX\n", vesa_modes);
	vesa_modes_cnt = 0;
}

BOOL VESA_init_hw()
{
	DWORD phy;
	DWORD flat;
	dbg_printf("VESA init begin...\n");

	flat = _PageAllocate(1, PG_SYS, 0, 0x0, 0, 0x100000, &phy, PAGEUSEALIGN | PAGECONTIG | PAGEFIXED);
	vesa_buf = (void*)flat;

	if(vesa_buf)
	{
		DWORD modes_count = 0;
#if 0
		/* JH: bad idea, memory must upper 640k! */
		DWORD lp = _GetLastV86Page();
		DWORD v86_page = lp+1;

		_SetLastV86Page(v86_page, 0);
		if(_PhysIntoV86(phy >> 12, ThisVM, v86_page, 1, 0))
		{
			vesa_buf_v86 = v86_page*4096;
		}

		dbg_printf("last page was=%lX\n", lp);
		dbg_printf("VESA V86 addr=0x%lX, phy=0x%lX\n", vesa_buf_v86, phy);
#else
		/* FIXME: check if pages is free! */
		DWORD v86_page = 0xB0;
		if(_PhysIntoV86(phy >> 12, ThisVM, v86_page, 1, 0))
		{
			vesa_buf_v86 = v86_page*4096;
		}
		dbg_printf("VESA V86 addr=0x%lX, phy=0x%lX\n", vesa_buf_v86, phy);
#endif

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
			regs.Client_EAX = 0x4f00;
			regs.Client_ES  = V86_SEG(vesa_buf_v86);
			regs.Client_EDI = V86_OFF(vesa_buf_v86);

			vesa_bios(&regs);

			if((regs.Client_EAX & 0xFFFF) == 0x004F &&
				memcmp(info->VESASignature, "VESA", 4) == 0)
			{
				WORD *modes = NULL;

				dbg_printf("VESA bios result=%X, vesa_version=%X, modes_ptr=%lX\n",
					regs.Client_EAX & 0xFFFF, info->VESAVersion, info->VideoModePtr);

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
					regs.Client_EAX = 0x4F01;
					regs.Client_ECX = modes[i];
					regs.Client_ES  = V86_SEG(vesa_buf_v86+MODE_OFFSET);
					regs.Client_EDI = V86_OFF(vesa_buf_v86+MODE_OFFSET);

					vesa_bios(&regs);

					//dbg_printf("mode=%X atrs=0x%lX eax=0x%lX\n", modes[i], modeinfo->ModeAttributes, regs.Client_EAX);
					if((regs.Client_EAX & 0xFFFF) == 0x004F)
					{
						if(modeinfo->ModeAttributes &
							(VESA_MODE_HW_SUPPORTED | VESA_MODE_COLOR | VESA_MODE_GRAPHICS | VESA_MODE_LINEAR_FRAMEBUFFER))
						{
							vesa_mode_t *m = &vesa_modes[vesa_modes_cnt];
							m->width   = modeinfo->XResolution;
							m->height  = modeinfo->YResolution;
							m->bpp     = modeinfo->BitsPerPixel;
							m->pitch   = modeinfo->BytesPerScanLine;
							m->phy     = modeinfo->PhysBasePtr;
							m->mode_id = modes[i];
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

		 		memcpy(hda->vxdname, vesa_vxd_name, sizeof(vesa_vxd_name));
				hda->vram_size = info->TotalMemory * 0x10000UL;
				//hda->vram_size = 16*1024*1024;
				hda->vram_bar_size = hda->vram_size;

				hda->vram_pm32 = (void*)_MapPhysToLinear(fb_phy, hda->vram_size, 0);
				//hda->vram_pm32 = (void*)_MapPhysToLinear(ISA_LFB, hda->vram_size, 0);

				hda->flags    |= FB_SUPPORT_FLIPING;
				
				vesa_valid = TRUE;
				
				dbg_printf("VESA_init_hw(vram_size=%ld) = TRUE\n", hda->vram_size);
				return TRUE;
			}
		}
	}
	return FALSE;
}

BOOL VESA_valid()
{
	dbg_printf("VESA_valid()\n");
	return vesa_valid;
}

BOOL VESA_validmode(DWORD w, DWORD h, DWORD bpp)
{
	DWORD i;
	dbg_printf("VESA_validmode()\n");
	
	for(i = 0; i < vesa_modes_cnt; i++)
	{
		if(vesa_modes[i].width == w && vesa_modes[i].height == h && vesa_modes[i].bpp == bpp)
		{
			return TRUE;
		}
	}
	
	dbg_printf("fail to valid mode: %ld %ld %ld\n", w, h, bpp);
	return FALSE;
}

void VESA_clear()
{
	memset(hda->vram_pm32, 0, hda->pitch*hda->height);
}

BOOL VESA_setmode(DWORD w, DWORD h, DWORD bpp)
{
	DWORD i;
	dbg_printf("VESA_validmode()\n");
	//if(!VESA_validmode(w, h, bpp)) return FALSE;

	for(i = 0; i < vesa_modes_cnt; i++)
	{
		if(vesa_modes[i].width == w && vesa_modes[i].height == h && vesa_modes[i].bpp == bpp)
		{
			CRS_32 regs;
			load_client_state(&regs);
			// set mode
			regs.Client_EAX = 0x4f02;
			regs.Client_EBX = vesa_modes[i].mode_id | (1 << 14);
			regs.Client_ES = 0;
			regs.Client_EDI = 0;
			vesa_bios(&regs);
			
			if((regs.Client_EAX & 0xFFFF) == 0x004F)
			{
				dbg_printf("SET success: %d %d %d\n", w, h, bpp);
				hda->width  = w;
				hda->height = h;
				hda->bpp    = bpp;
				hda->pitch  = vesa_modes[i].pitch;
				hda->stride = h * hda->pitch;
				hda->surface = 0;

				VESA_clear();
				mouse_invalidate();
				FBHDA_update_heap_size(FALSE);
				
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

void FBHDA_palette_set(unsigned char index, DWORD rgb)
{

}

DWORD FBHDA_palette_get(unsigned char index)
{
	return 0;
}

BOOL FBHDA_swap(DWORD offset)
{
	return TRUE;
}

void FBHDA_access_begin(DWORD flags)
{
	//Wait_Semaphore(hda_sem, 0);
	if(fb_lock_cnt++ == 0)
	{
		mouse_erase();
	}
}

void FBHDA_access_rect(DWORD left, DWORD top, DWORD right, DWORD bottom)
{
	FBHDA_access_begin(0);
}

void FBHDA_access_end(DWORD flags)
{
	fb_lock_cnt--;
	if(fb_lock_cnt < 0) fb_lock_cnt = 0;
	
	if(fb_lock_cnt == 0)
	{
		mouse_blit();
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
