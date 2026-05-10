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

#include "wram.h"
#include "async.h"

#include "vxd_lib.h"
#include "3d_accel.h"

#include "boxvint.h" /* VGA regitry */

#include "pci.h" /* re-use PCI functions from SVGA */

#include "vesa.h"
#include "mtrr.h"

#define IO_IN8
#define IO_OUT8
#include "io32.h"

#include "vxd_terror.h"

#include "code32.h"

#define ISA_LFB 0xE0000000UL

extern FBHDA_t *hda;
extern LONG fb_lock_cnt;
extern DWORD ThisVM;
extern ULONG hda_sem;

static char vesa_vxd_name[] = "vesamini.vxd";

#define VESA_MODE_MAX_FREQS 16

typedef struct vesa_mode
{
	DWORD width;
	DWORD height;
	DWORD bpp;
	DWORD pitch;
	DWORD phy;
	WORD  mode_id;
	WORD  flags;
	WORD  freqs[VESA_MODE_MAX_FREQS];
} vesa_mode_t;

static vesa_mode_t *vesa_modes = NULL;
static DWORD vesa_modes_cnt = 0;
WORD vesa_version = 0;
static DWORD vesa_caps = 0;
static int act_mode = -1;
static DWORD video_bpp;
static DWORD video_pitch;
static BOOL vga_mode = TRUE;
static BOOL mttr_was_set = FALSE;
static BOOL timer_set = FALSE;

static blit_t wblit;

static void VESA_draw(blit_t *blit);

/* from gtf.c */
void gtf_calc(DWORD w, DWORD h, DWORD freq, vesa_crtc_info_t *crtc_info);

/* vxd_mouse.vxd */
BOOL mouse_get_rect(DWORD *ptr_left, DWORD *ptr_top,
	DWORD *ptr_right, DWORD *ptr_bottom);

static CRS_32 reg_state;

void vesa_bios_V86(CRS_32 *V86regs)
{
	CRS_32 *pstate = &reg_state;
	
	// sizeof(CRS_32) = 108
	_asm    mov       ebx, [ThisVM]
	_asm    mov       ecx, [V86regs]
	_asm    mov       edi, [pstate]
  /* save state */
	//_asm    sub       esp, 128
	//_asm    mov       edi, esp
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
	_asm   mov       edi, [V86regs]
	VMMCall(Save_Client_State);
	/* restore state regs */
	//_asm    mov       esi, esp
	_asm   mov       esi, [pstate]
	VMMCall(Restore_Client_State)
	//_asm    add       esp, 128
}

void vesa_bios(CRS_32 *V86regs)
{
	_asm
	{
		pushad
		mov esi, [V86regs]
		mov eax, [esi+0x1C]
		mov ebx, [esi+0x10]
		mov ecx, [esi+0x18]
		mov edx, [esi+0x14]
		mov edi, [esi+0x00]
		mov esi, [esi+0x04]
		push dword ptr 10h
	};
	VMMCall(Exec_VxD_Int);
	//VMMCall(_ExecVxDIntMustComplete);
	_asm
	{
		mov esi, [V86regs]
		mov [esi+0x1C], eax
		mov [esi+0x10], ebx
		mov [esi+0x18], ecx
		mov [esi+0x14], edx
		mov [esi+0x00], edi
		mov [esi+0x04], esi
		popad
	};
}

void load_client_state(CRS_32 *V86regs)
{
//	_asm   mov       edi, [V86regs]
//	VMMCall(Save_Client_State);
	VMCB_t *vm = (VMCB_t*)ThisVM;
	memcpy(V86regs, vm->CB_Client_Pointer, sizeof(CRS_32));
	V86regs->Client_EBP = ThisVM;
}

static DWORD vesa_buf_v86;
static void *vesa_buf;
static DWORD vesa_buf_phy;
static vesa_palette_entry_t *vesa_pal;
static int vesa_pal_bits = 6;

static BOOL vesa_valid = FALSE;

static DWORD conf_dos_window = 0;
static DWORD conf_hw_double_buf = 1; /* much better performance on real HW */
static DWORD conf_mtrr = 1;

#define MODE_OFFSET 1024
#define CRTC_OFFSET 2048
#define PAL_OFFSET  3072 // pal size = 4*256

static DWORD clock_to_test[] = {120, 110, 100, 85, 75, 70, 60, 50, 30, 25, 0};

#define V86_SEG(_lin) ((_lin) >> 4)
#define V86_OFF(_lin) ((_lin) & 0xF)

#define LIN_FROM_V86(_flatptr) ((((_flatptr) >> 12) & 0xFFFF0UL) + ((_flatptr) & 0xFFFFUL))

#define VESA_SUCC(_r) (((_r).Client_EAX & 0xFFFF)==0x004F)

static void offset_calc(DWORD offset, DWORD *ox, DWORD *oy)
{
	DWORD ps = ((video_bpp+7)/8);
	*oy = offset/video_pitch;
	*ox = (offset % video_pitch)/ps;
}

static void alloc_modes_info(DWORD cnt)
{
	DWORD vesa_modes_pages = ((sizeof(vesa_mode_t) * cnt) + P_SIZE - 1) / P_SIZE;
	vesa_modes = (vesa_mode_t*)_PageAllocate(vesa_modes_pages, PG_SYS, 0, 0x0, PAGE_ALLOC_MIN, PAGE_ALLOC_MAX, NULL, PAGEZEROINIT);
	dbg_printf("vesa_modes = %lX\n", vesa_modes);
	vesa_modes_cnt = 0;
}

static DWORD VESA_pitch(DWORD width, DWORD bpp)
{
	DWORD bp = (bpp + 7) / 8;
	return (bp * width + (FBHDA_ROW_ALIGN-1)) & (~((DWORD)FBHDA_ROW_ALIGN-1));
}

#define DSWAP(_a, _i, _j) {WORD tmp = _a[_i]; _a[_i] = _a[_j]; _a[_j] = tmp;}

/* begin mode list with 60 Hz, 50 Hz and the higher freqs */
static void mode_sort_freqs(int m)
{
	int k = 0;
	int f60 = -1;
	int f50 = -1;

	while(vesa_modes[m].freqs[k] != 0)
	{
		if(vesa_modes[m].freqs[k] == 50)	f50 = k;
		if(vesa_modes[m].freqs[k] == 60)	f60 = k;
		k++;
	}
	
	k = 0;
	if(f60 >= 0)
	{
		DSWAP(vesa_modes[m].freqs, k, f60);
		k++;
	}
	
	if(f50 >= 0)
	{
		DSWAP(vesa_modes[m].freqs, k, f50);
		k++;
	}
}

DWORD vram_phy = 0;

static char VESA_conf_path[] = "Software\\vmdisp9x\\vesa";

void VESA_load_vbios_pm();

BOOL VESA_init_hw()
{
	DWORD flat;
	DWORD conf_vram_limit = 32;
	DWORD wram_size = 64;
	
	dbg_printf("VESA init begin...\n");

	RegReadConf(HKEY_LOCAL_MACHINE, VESA_conf_path, "VRAMLimit",        &conf_vram_limit);
	RegReadConf(HKEY_LOCAL_MACHINE, VESA_conf_path, "WRAMSize",         &wram_size);
	RegReadConf(HKEY_LOCAL_MACHINE, VESA_conf_path, "MTRR",             &conf_mtrr);
	RegReadConf(HKEY_LOCAL_MACHINE, VESA_conf_path, "DosWindowSetMode", &conf_dos_window);
	RegReadConf(HKEY_LOCAL_MACHINE, VESA_conf_path, "HWDoubleBuffer",   &conf_hw_double_buf);

	if(wram_size < WRAM_MIN_MB)
	{
		wram_size = WRAM_MIN_MB;
	}

	flat = _PageAllocate(1, PG_SYS, 0, 0x0, PAGE_ALLOC_MIN, PAGE_ALLOC_MAX, &vesa_buf_phy, PAGEUSEALIGN | PAGECONTIG | PAGEFIXED);
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

			vesa_bios_V86(&regs);

			if((regs.Client_EAX & 0xFFFF) == 0x004F &&
				memcmp(info->VESASignature, "VESA", 4) == 0)
			{
				WORD *modes = NULL;
				DWORD vram_size = 0;
				void  *vram_lin = NULL;

				dbg_printf("VESA bios result=%X, vesa_version=%X, modes_ptr=%lX vesa_caps=%lX\n",
					regs.Client_EAX & 0xFFFF, info->VESAVersion, info->VideoModePtr, info->Capabilities);

				if(info->VESAVersion < VESA_VBE_2_0)
				{
					terror("We need VBE 2.0 as minimum, abort\n");
					tpause();
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
					vesa_bios_V86(&regs);

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
							if(m->bpp == 16)
							{
								if(modeinfo->GreenMaskSize == 5)
								{
									m->bpp = 15;
								}
							}

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

#if 0
							if(info->VESAVersion >= VESA_VBE_3_0)
							{
								vesa_crtc_info_t crtc_test;
								int k = 0;
								DWORD *pfreq = clock_to_test;
								while(*pfreq != 0)
								{
									DWORD last_freq = 0;
									gtf_calc(m->width, m->height, *pfreq, &crtc_test);
									if(crtc_test.PixelClock <= modeinfo->MaxPixelClock)
									{
										regs.Client_EAX = VESA_CMD_PIXEL_CLOCK;
										regs.Client_EBX = 0;
										regs.Client_EDX = m->mode_id;
										regs.Client_ECX = crtc_test.PixelClock;
										vesa_bios(&regs);
										if(VESA_SUCC(regs))
										{
											if(k > 0)
											{
												if(last_freq > regs.Client_ECX)
												{
													m->freqs[k] = *pfreq;
													k++;
												}
											}
											else
											{
												m->freqs[k] = *pfreq;
												k++;
											}
											last_freq = regs.Client_ECX;
											dbg_printf("  freq=%d pixclk=%ld\n", *pfreq, last_freq);
										}
									}
								}
								m->freqs[k] = 0;
								mode_sort_freqs(i);
							}
							else
#endif
							{
								m->freqs[0] = 0;
							}
						}						
					}
				} // for

				if(vesa_modes_cnt == 0)
				{
					return FALSE;
				}

				vesa_version = info->VESAVersion;
				vesa_caps = info->Capabilities;
				vram_size = info->TotalMemory * 0x10000UL;

				if(vram_size > (conf_vram_limit*1024*1024))
				{
					vram_size = conf_vram_limit*1024*1024;
				}
				
				if(fb_phy == 0xFFFFFFFF)
				{
					fb_phy = ISA_LFB;
				}
				
				vram_phy = fb_phy;
				
				/* 9x have trobles with alloc large continuous memory block */
				vram_lin = (void*)_MapPhysToLinear(vram_phy, vram_size, 0);

				if(!wram_init(wram_size*1024*1024))
				{
					dbg_printf("cannot allocated %d MB RAM!\n", wram_size);
					return FALSE;
				}

			 	if(!FBHDA_init_hw())
			 	{
			 		dbg_printf("cannot init FBHDA\n");
			 		return FALSE;
			 	}

				memcpy(hda->vxdname, vesa_vxd_name, sizeof(vesa_vxd_name));

				hda->vram_size = vram_size;
				hda->vram_size_bar = conf_vram_limit*1024*1024;

				hda->vram_phylin = vram_lin;
				hda->vram_pm32   = (BYTE*)wram + wram->regs.s.fbmin;
				hda->vram_size_virt = wram->regs.s.fbmax - wram->regs.s.fbmin;
				dbg_printf("vram_phy=%lX vram_phylin=%lX\n", vram_phy, hda->vram_phylin);

				hda->flags |= FB_SUPPORT_FLIPING | FB_VESA_MODES;
				if(vesa_version >= VESA_VBE_3_0)
				{
					//hda->flags |= FB_SUPPORT_CLOCK;
				}

				vesa_pal = (vesa_palette_entry_t*)(((BYTE*)vesa_buf)+PAL_OFFSET);

				vesa_valid  = TRUE;

				VESA_load_vbios_pm();

				wblit.num_changes = 0;
				timer_set = async_blit_init(&wblit, VESA_draw);
				dbg_printf("timer = %d\n", timer_set);

				dbg_printf("VESA_init_hw(vram_size=%ld, wram_size=%ld, real_memory=%ld) = TRUE\n",
					hda->vram_size, hda->vram_size_virt, hda->vram_size_bar);

				return TRUE;
			}
			else
			{
				terror("Can't interact with VESA BIOS, this driver can't work\n");
				tpause();
			}
		}
	}
	return FALSE;
}

static WORD *vbios32 = NULL;
static WORD vbios32_sel = 0;
static DWORD vbios32_mem = 0;

BOOL VESA_display_start_pm(BOOL vtrace, DWORD start_addr)
{
	BYTE cmd = vtrace ? 0x80 : 0;
	/* DOC: DX:CX is the 32 bit offset in display memory, aligned to a plane boundar,
	... but in 8+ bits per pixel modes this is the offset from the start of memory divided by 4 */
	WORD h1 = (start_addr >> 2) & 0xFFFF;
	WORD h2 = start_addr >> 18;

	if(vbios32 != NULL && vbios32[VESA_PMTABLE_OFF_DISPLAY_START])
	{
		//dbg_printf("VESA_display_start_pm ...");
		_asm
		{
			pushad
			push es
			mov eax, [vbios32]
			xor ebx, ebx
			mov bx,  [eax+2] /* VESA_PMTABLE_OFF_DISPLAY_START*2 */
			add eax, ebx
			mov bx, [vbios32_sel]
			or bx, bx
			jz skip_load_es
				mov es, bx
			skip_load_es:
			xor ebx, ebx
			xor ecx, ecx
			xor edx, edx
			mov bl,[cmd]
			mov cx,[h1]
			mov dx,[h2]
			call eax
			pop es
			popad
		};
		//dbg_printf("OK\n");
		
		return TRUE;
	}
	
	return FALSE;
}

void VESA_load_vbios_pm()
{
	CRS_32 regs;

	load_client_state(&regs);
	regs.Client_EAX = VESA_CMD_PM_ENTRY;
	regs.Client_EBX = 0;

	vesa_bios_V86(&regs);

	if(VESA_SUCC(regs))
	{
		WORD *bios_pm;
		WORD *ports;
		DWORD bios_pm_flat = regs.Client_ES * 0x10 + (regs.Client_EDI & 0xFFFF);
		dbg_printf("table_flat es=%lX, edi=%lX, code_size=%ld\n",
			regs.Client_ES, regs.Client_EDI, regs.Client_ECX & 0xFFFF);

		if(bios_pm_flat != 0)
		{
			DWORD code_size = regs.Client_ECX & 0xFFFF;
			DWORD code_pages = RoundToPages(code_size);

			bios_pm = (WORD*)bios_pm_flat;
			
			dbg_printf("set display off func: %X\n", bios_pm[VESA_PMTABLE_OFF_DISPLAY_START]);

			vbios32 = (WORD*)_PageAllocate(code_pages, PG_SYS, 0, 0x0, PAGE_ALLOC_MIN, PAGE_ALLOC_MAX, NULL, PAGEZEROINIT);
			if(vbios32 != NULL)
			{
				memset(vbios32, 0xFF, code_pages*P_SIZE);
				memcpy(vbios32, bios_pm, code_size);
				_PageModifyPermissions(((DWORD)vbios32) / P_SIZE, code_pages, 0, PC_USER | PC_WRITEABLE);
				dbg_printf("copy to vbios32 success\n");
			}
/*
			terror("Here is dump of pm32 table:\n");
			terrorf("vbios32[0] = %X\n", vbios32[0]);
			terrorf("vbios32[1] = %X\n", vbios32[1]);
			terrorf("vbios32[2] = %X\n", vbios32[2]);
			terrorf("vbios32[3] = %X\n", vbios32[3]);
			terrorf("code size = %d\n", code_size);
*/
			if(vbios32[VESA_PMTABLE_OFF_PORTS])
			{
				ports = vbios32 + vbios32[VESA_PMTABLE_OFF_PORTS];
				// ^ some BIOSes has this table after the code to copy
				
				while(*ports != 0xFFFF)
				{
					dbg_printf("port: %04X\n", *ports);
					//terrorf("port: = %X\n", *ports);
					ports++;
				}
				ports++;
				if(*ports != 0xFFFF)
				{
					DWORD mem_adr = *((DWORD*)ports);
					DWORD mem_size = ports[2];
					
					//terrorf("mem: = %X, size = %d\n", mem_adr, mem_size);
					
					vbios32_mem = _MapPhysToLinear(mem_adr, mem_size, 0);
					if(vbios32_mem != 0xFFFFFFFF)
					{
						DWORD hi  = 0;
						DWORD low = 0;
						DWORD selector;
						_BuildDescriptorDWORDs(mem_adr, RoundToPages(mem_adr), 0x92, 0x80, 0, &hi, &low);
	
						_Allocate_GDT_Selector(hi, low, 0, &selector, NULL);
						vbios32_sel = selector << 16;
					}
					
					dbg_printf("VESA PM need memory addr=0x%8X (size=%d)\n", mem_adr, mem_size);
				}
			}

#if 0
			/* print part of PM code (until first RET) */
			{
				BYTE *code = ((BYTE*)vbios32) + vbios32[VESA_PMTABLE_OFF_DISPLAY_START];
				dbg_printf("display start code: ");
				while(*code != 0xC3)
				{
					dbg_printf("0x%02X ", *code);
					code++;
				}
				dbg_printf("\n");
			}
#endif
		}
		else
		{
			dbg_printf("VESA_CMD_ADAPTER_INFO success but no table\n");
		}
	}
	else
	{
		dbg_printf("VESA_CMD_ADAPTER_INFO failed\n");
	}
}

BOOL VESA_valid()
{
	return vesa_valid;
}

void VESA_clear()
{
	memset((BYTE*)hda->vram_pm32+hda->system_surface, 0, hda->stride);
	dbg_printf("Clear success\n");
}

static int VESA_modechoice(DWORD w, DWORD h, DWORD bpp)
{
	DWORD i = 0;
	
	// try #1: look for exact resolution and 32bpp
	for(i = 0; i < vesa_modes_cnt; i++)
	{
		if(vesa_modes[i].width == w && vesa_modes[i].height == h && vesa_modes[i].bpp == 32)
		{
			return i;
		}
	}
	
	// try #2: look for any larger resolution in 32 bpp
	for(i = 0; i < vesa_modes_cnt; i++)
	{
		if(vesa_modes[i].width >= w && vesa_modes[i].height >= h && vesa_modes[i].bpp == 32)
		{
			return i;
		}
	}
	
	// try #3: look for exact resolution in best bpp
	for(i = 0; i < vesa_modes_cnt; i++)
	{
		if(vesa_modes[i].width == w && vesa_modes[i].height == h && vesa_modes[i].bpp >= bpp)
		{
			return i;
		}
	}
	
	// try #4: lok for any larger resolution in best bpp
	for(i = 0; i < vesa_modes_cnt; i++)
	{
		if(vesa_modes[i].width >= w && vesa_modes[i].height >= h && vesa_modes[i].bpp >= bpp)
		{
			return i;
		}
	}
	
	return vesa_modes_cnt;
}

static DWORD VESA_freqchoice(int mode, DWORD rr_min, DWORD rr_max)
{
	int k;
	for(k = 0; k < VESA_MODE_MAX_FREQS; k++)
	{
		DWORD f = vesa_modes[mode].freqs[k];
		if(f == 0)
		{
			return 0; /* default */
		}
		
		if(f >= rr_min && f <= rr_max)
		{
			return f;
		}
	}
	return 0;
}

BOOL VESA_validmode(DWORD w, DWORD h, DWORD bpp)
{
	DWORD mode = VESA_modechoice(w, h, bpp);

	if(mode < vesa_modes_cnt)
	{
		return TRUE;
	}

	return FALSE;
}


BOOL VESA_setmode_phy(DWORD w, DWORD h, DWORD bpp, DWORD rr_min, DWORD rr_max)
{
	DWORD mode = 0;
	DWORD num_buffers = 0;
	DWORD video_stride;
	DWORD freq;
	CRS_32 regs;
	vesa_crtc_info_t *crtc_info = NULL;
	DWORD refresh = ASYNC_DEFAULT;

	VESA_HIRES_enable();	

	mode = VESA_modechoice(w, h, bpp);
	if(mode >= vesa_modes_cnt) return FALSE;
		
	freq = VESA_freqchoice(mode, rr_min, rr_max);
	
	if(freq != 0)
	{
		crtc_info = (vesa_crtc_info_t*)((char*)vesa_buf + CRTC_OFFSET);
		gtf_calc(vesa_modes[mode].width, vesa_modes[mode].height, freq, crtc_info);
	}

	load_client_state(&regs);

	// set mode
	regs.Client_EAX = VESA_CMD_MODE_SET;
	regs.Client_EBX = vesa_modes[mode].mode_id | VESA_SETMODE_LFB | VESA_SETMODE_NOCLEAR;
	if(crtc_info)
	{
		regs.Client_ES  = V86_SEG(vesa_buf_v86+CRTC_OFFSET);
		regs.Client_EDI = V86_OFF(vesa_buf_v86+CRTC_OFFSET);
		
		vesa_bios(&regs);
		if(!VESA_SUCC(regs))
		{
			/* fail to set mode, try with default frequency */
			regs.Client_ES = 0;
			regs.Client_EDI = 0;
			dbg_printf("freq=%d failed, try default\n", freq);
			vesa_bios(&regs);
		}
		else
		{
			refresh = 1000 / freq;
		}
	}
	else
	{
		regs.Client_ES = 0;
		regs.Client_EDI = 0;
		vesa_bios(&regs);
	}
	
	if(VESA_SUCC(regs))
	{
		DWORD test_x;
		DWORD test_y;

		dbg_printf("SET success: %d %d %d\n", w, h, bpp);
		hda->width  = w;
		hda->height = h;
		hda->bpp    = bpp;
		hda->pitch  = VESA_pitch(w, bpp); //vesa_modes[mode].pitch;
		hda->stride = h * hda->pitch;
		hda->surface = 0;
		
		video_bpp    = vesa_modes[mode].bpp;
		video_pitch  = vesa_modes[mode].pitch;
		video_stride = h * video_pitch;

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
		num_buffers = 1;

		/* I found there is some deadlock hazard on V86 or PM16 ints,
		 * and when is called on very frame there is big chance on it.
		 * This is why HW double/tripple buffering is available only
		 * when there is PM32 procedure in VBIOS.
		 */
		if(vbios32 && vbios32[VESA_PMTABLE_OFF_DISPLAY_START])
		{
			if(conf_hw_double_buf > 0)
			{
				if(conf_hw_double_buf >= 2)
				{
					/* test if HW flip and vtrace supported */
					offset_calc(video_stride, &test_x, &test_y);
					regs.Client_EAX = VESA_CMD_DISPLAY_START;
					regs.Client_EBX = VESA_DISPLAYSTART_VTRACE;
					regs.Client_ECX = test_x;
					regs.Client_EDX = test_y;
					vesa_bios(&regs);
					if(VESA_SUCC(regs))
					{
						num_buffers = 3;
	
						regs.Client_EAX = VESA_CMD_DISPLAY_START;
						regs.Client_EBX = VESA_DISPLAYSTART_SET;
						regs.Client_ECX = 0;
						regs.Client_EDX = 0;
						vesa_bios(&regs);
					}
				}
	
				if(num_buffers <= 1)
				{
					/* test if HW flip supported */
					offset_calc(video_stride, &test_x, &test_y);
					regs.Client_EAX = VESA_CMD_DISPLAY_START;
					regs.Client_EBX = VESA_DISPLAYSTART_SET;
					regs.Client_ECX = test_x;
					regs.Client_EDX = test_y;
					vesa_bios(&regs);
	
					if(VESA_SUCC(regs))
					{
						num_buffers = 2;
	
						regs.Client_EAX = VESA_CMD_DISPLAY_START;
						regs.Client_EBX = VESA_DISPLAYSTART_SET;
						regs.Client_ECX = 0;
						regs.Client_EDX = 0;
						vesa_bios(&regs);
					}
				}
			}
		}

		hda->system_surface = 0;
		hda->surface        = 0;
		if(num_buffers >= 3)
		{
			hda->flags |= FB_SUPPORT_VSYNC;
		}
		else
		{
			hda->flags &= ~FB_SUPPORT_VSYNC;
		}
		
		/* set VRAM destination */
		wblit.base.ptr = hda->vram_phylin;
		wblit.dst[0].flat.ptr = hda->vram_phylin;
		wblit.dst[1].flat.ptr = NULL;
		wblit.dst[2].flat.ptr = NULL;
		wblit.dst_w     = vesa_modes[mode].width;
		wblit.dst_h     = vesa_modes[mode].height;
		wblit.dst_pitch = vesa_modes[mode].pitch;

		switch(vesa_modes[mode].bpp)
		{
			case 8:  wblit.dst_mode = MODE_8;  break;
			case 15: wblit.dst_mode = MODE_15; break;
			case 16: wblit.dst_mode = MODE_16; break;
			case 24: wblit.dst_mode = MODE_24; break;
			case 32: wblit.dst_mode = MODE_32; break;
		}
	
		if(wblit.dst_w > w || wblit.dst_h > h)
		{
			DWORD scans = wblit.dst_w/w;
			if((scans*h) > wblit.dst_h)
			{
				scans = wblit.dst_w/h;
			}
			
			wblit.dst_scans = scans;
			wblit.dst_padx  = (wblit.dst_w - scans*w) / 2;
			wblit.dst_pady  = (wblit.dst_h - scans*h) / 2;	
		}
		else
		{
			wblit.dst_scans = 1;
			wblit.dst_padx  = 0;
			wblit.dst_pady  = 0;
		}
		
		/* set WRAM source */
		wram->regs.s.width  = w;
		wram->regs.s.height = h;
		switch(bpp)
		{
			case 8:  wram->regs.s.mode = MODE_8;  break;
			case 15: wram->regs.s.mode = MODE_15; break;
			case 16: wram->regs.s.mode = MODE_16; break;
			case 24: wram->regs.s.mode = MODE_24; break;
			case 32: wram->regs.s.mode = MODE_32; break;
		}
		wram->regs.s.pitch = hda->pitch;

		act_mode = mode;

		/* check if we have enought memory for all buffers */
		if(num_buffers >= 2)
		{
			DWORD pages = (video_stride + P_SIZE - 1) / P_SIZE;
			DWORD lines = ((pages * P_SIZE) + video_pitch - 1) / video_pitch;
			DWORD page_stride = lines * video_pitch;

			if(hda->vram_size >= (page_stride + video_stride))
			{
				wblit.dst[1].flat.dw = wblit.dst[0].flat.dw + page_stride;
			}
			else
			{
				num_buffers = 1;
			}
			
			if(num_buffers >= 3)
			{
				if(hda->vram_size >= ((page_stride*2) + video_stride))
				{
					wblit.dst[2].flat.dw = wblit.dst[1].flat.dw + page_stride;
				}
				else
				{
					num_buffers = 2;
				}
			}
		}
		
		async_blit_settime(refresh);

		dbg_printf("mode set, mode_id=%d, i=%d, pitch=%d, buffers=%d\n", vesa_modes[mode].mode_id, mode, wram->regs.s.pitch, num_buffers);
		return TRUE;
	}
	else
	{
		dbg_printf("vbios fail: eax=0x%lX\n", regs.Client_EAX);
	}

	dbg_printf("fail to set %ld %ld %ld\n", w, h, bpp);
	return FALSE;
}

static void mtrr_setup()
{
	if(conf_mtrr && !mttr_was_set)
	{
		if(vram_phy > 1*1024*1024)
		{
			if(MTRR_GetVersion())
			{
				dbg_printf("MTRR supported\n");
				dbg_printf("MTRR_SetPhysicalCacheTypeRange(%lX, 0, %ld, %ld)\n", vram_phy, hda->vram_size, MTRR_FRAMEBUFFERCACHED);
				MTRR_SetPhysicalCacheTypeRange(vram_phy, 0, hda->vram_size, MTRR_FRAMEBUFFERCACHED);
				mttr_was_set = TRUE;
			}
			else
			{
				dbg_printf("MTRR unsupported\n");
			}
		}
	}
}

static void VESA_setptr(DWORD offset, BOOL triplebuf)
{
	if(!VESA_display_start_pm(triplebuf, offset))
	{
		CRS_32 regs;
		DWORD off_x, off_y;
	
		offset_calc(offset, &off_x, &off_y);
	
		load_client_state(&regs);
	
		regs.Client_EAX = VESA_CMD_DISPLAY_START;
		regs.Client_EBX = triplebuf ? VESA_DISPLAYSTART_VTRACE : VESA_DISPLAYSTART_SET;
		regs.Client_ECX = off_x;
		regs.Client_EDX = off_y;
	
		vesa_bios(&regs);
	}
}

BOOL VESA_setmode(DWORD w, DWORD h, DWORD bpp, DWORD rr_min, DWORD rr_max)
{
	BOOL valid;
	
	FBHDA_lock();
	valid = VESA_setmode_phy(w, h, bpp, rr_min, rr_max);
	FBHDA_unlock();
	
	if(valid)
	{
		mtrr_setup();
		VESA_clear();
		mouse_invalidate();
		return TRUE;
	}
	return FALSE;
}

void FBHDA_palette_set(unsigned char index, DWORD rgb)
{
	if(wblit.dst_mode != MODE_8)
	{
		wram->palette[index].dw = rgb;
		return;
	}
	
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
		//regs.Client_EDI = ((DWORD)vesa_buf)+PAL_OFFSET+4*index;
		
		vesa_bios_V86(&regs);
	}
}

DWORD FBHDA_palette_get(unsigned char index)
{
	if(wblit.dst_mode != MODE_8)
	{
		return wram->palette[index].dw;
	}

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

static void VESA_draw(blit_t *blit)
{
	if(vga_mode) return;
	if(act_mode < 0) return;

	if(blit->num_changes)
	{
		if(blit->dst[1].flat.ptr == NULL && blit->dst[2].flat.ptr == NULL)
		{
			/* single buffer, write only changes */
			wram_blit(&wblit);
		}
		else if(blit->dst[2].flat.ptr == NULL)
		{
			/* double buffer = write full frame
			
				dst[0] <- visible
				dst[1] <- CPU write
			*/
			DWORD offset;
			{
				void *new_ptr = blit->dst[1].flat.ptr;
				blit->dst[1].flat.ptr = blit->dst[0].flat.ptr;
				blit->dst[0].flat.ptr = new_ptr;
			}
			offset = blit->dst[0].flat.dw - blit->base.dw;
			blit->src_sx = 0;
			blit->src_sy = 0;
			blit->src_ex = hda->width;
			blit->src_ey = hda->height;
			wram_blit(&wblit);
			
			VESA_setptr(offset, FALSE);
		}
		else
		{
			/* triple buffer, write full frame
				dst[0] <- unvisible -> visible (swap on VSYNC)
				dst[1] <- CPU write
				dst[2] <- visible -> unvisible ( will be on next VSYNC)
			*/
			DWORD offset;
			{
				void *old_ptr = blit->dst[0].flat.ptr;
				blit->dst[0].flat.ptr = blit->dst[1].flat.ptr;
				blit->dst[1].flat.ptr = blit->dst[2].flat.ptr;
				blit->dst[2].flat.ptr = old_ptr;
			}
			offset = blit->dst[0].flat.dw - blit->base.dw;
			blit->src_sx = 0;
			blit->src_sy = 0;
			blit->src_ex = hda->width;
			blit->src_ey = hda->height;
			wram_blit(&wblit);
			
			VESA_setptr(offset, TRUE);
		}
		blit->num_changes = 0;
	}
}

BOOL FBHDA_swap(DWORD offset, DWORD flags)
{
	void *new_fb;
	if((flags & FBHDA_SWAP_QUERY) != 0)
	{
		return TRUE;
	}

	new_fb = ((BYTE*)hda->vram_pm32) + offset;
	if(wram_swap(new_fb, &wblit))
	{
		if(!timer_set)
		{
			VESA_draw(&wblit);
		}
		else
		{
			hda->onflip = 1;
		}
		hda->surface = offset;
		return TRUE;
	}
	return TRUE;
}

void FBHDA_access_begin(DWORD flags)
{
	FBHDA_lock();
	
	if(flags & FBHDA_ACCESS_MOUSE_MOVE)
	{
		DWORD left, top, right, bottom;
		if(mouse_get_rect(&left, &top, &right, &bottom))
		{
			wram_changes(&wblit, left, top, right, bottom);
		}
	}
	else
	{
		wram_changes(&wblit, 0, 0, hda->width, hda->height);
	}
	
	FBHDA_unlock();
}

void FBHDA_access_rect(DWORD left, DWORD top, DWORD right, DWORD bottom)
{
	FBHDA_lock();
	wram_changes(&wblit, left, top, right, bottom);
	FBHDA_unlock();
}

void FBHDA_access_end(DWORD flags)
{
	FBHDA_lock();

	if(flags & FBHDA_ACCESS_MOUSE_MOVE)
	{
		DWORD left, top, right, bottom;
		if(mouse_get_rect(&left, &top, &right, &bottom))
		{
			wram_changes(&wblit, left, top, right, bottom);
		}
	}

	fb_lock_cnt--;
	if(fb_lock_cnt < 0) fb_lock_cnt = 0;

	if(!timer_set)
	{
		if(fb_lock_cnt == 0)
		{
			VESA_draw(&wblit);
		}
	}

	FBHDA_unlock();
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
		int i;
		
		mode->cb = sizeof(FBHDA_mode_t);
		mode->width = vesa_modes[index].width;
		mode->height = vesa_modes[index].height;
		mode->bpp = vesa_modes[index].bpp;
		for(i = 0; i < FBHDA_MODE_MAX_REFRESH; i++)
		{
			mode->refresh[i] = vesa_modes[index].freqs[i];
			if(mode->refresh[i] == 0)
			{
				break;
			}
		}
		for(; i < FBHDA_MODE_MAX_REFRESH; i++)
		{
			mode->refresh[i] = 0;
		}
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
