/**************************************************************************

Copyright (c) 2024 Jaroslav Hensl <emulator@emulace.cz>

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
#define VBE

#include "winhack.h"
#include "vmm.h"
#include "vxd.h"

#include "vxd_lib.h"
#include "3d_accel.h"

#include "boxvint.h"

#include "pci.h" /* re-use PCI functions from SVGA */

#include "code32.h"

#define IO_IN16
#define IO_OUT16
#define IO_IN8
#define IO_OUT8
#ifndef QEMU
# define IO_IN32
#endif
#include "io32.h"

#define PCI_VENDOR_ID_INNOTEK           0x80EE
#define PCI_DEVICE_ID_VBOX_VGA          0xBEEF
#define PCI_VENDOR_ID_BOCHSVBE          0x1234
#define PCI_DEVICE_ID_BOCHSVBE          0x1111

/* (max) 256MB VRAM: max 8K in 16:10 */
#define RES_MAX_X 8192
#define RES_MAX_Y 5120

#define ISA_LFB 0xE0000000UL

static BOOL vbe_is_valid = FALSE;
extern FBHDA_t *hda;
extern LONG fb_lock_cnt;
WORD vbe_chip_id = 0;

#ifdef QEMU
static char vbe_vxd_name[] = "qemumini.vxd";
#else
static char vbe_vxd_name[] = "boxvmini.vxd";
#endif

#include "vxd_strings.h"

static inline void wridx(unsigned short idx_reg, unsigned char idx, unsigned char data)
{
	outpw(idx_reg, (unsigned short)idx | (((unsigned short)data) << 8));
}

static WORD VBE_detect(DWORD *vram_size)
{
	WORD chip_id;

	outpw(VBE_DISPI_IOPORT_INDEX, VBE_DISPI_INDEX_ID);

	chip_id = inpw(VBE_DISPI_IOPORT_DATA);
#ifdef QEMU
	if(chip_id < VBE_DISPI_ID0 || chip_id > VBE_DISPI_ID6)
		return 0;

	if(vram_size)
	{
		outpw(VBE_DISPI_IOPORT_INDEX, VBE_DISPI_INDEX_VIDEO_MEMORY_64K);
		*vram_size = ((DWORD)inpw(VBE_DISPI_IOPORT_DATA)) << 16;
	}

	return chip_id;
#else
	if(vram_size)
	{
		*vram_size = inpd(VBE_DISPI_IOPORT_DATA);
	}

	if(chip_id >= VBE_DISPI_ID0 && chip_id <= VBE_DISPI_ID4)
		return chip_id;

	return 0;
#endif
}

DWORD VBE_get_LFB()
{
	PCIAddress pciAddr;
	
	/* virtualbox */
	if(PCI_FindDevice(PCI_VENDOR_ID_INNOTEK, PCI_DEVICE_ID_VBOX_VGA, &pciAddr))
	{
		PCI_SetMemEnable(&pciAddr, TRUE);
		return PCI_GetBARAddr(&pciAddr, 0);
	}
	
	/* qemu */
	if(PCI_FindDevice(PCI_VENDOR_ID_BOCHSVBE, PCI_DEVICE_ID_BOCHSVBE, &pciAddr))
	{
		PCI_SetMemEnable(&pciAddr, TRUE);
		return PCI_GetBARAddr(&pciAddr, 0);
	}
	
	/* try failback */
	return ISA_LFB;
}

BOOL VBE_init_hw()
{
	DWORD vram_size;
	DWORD vram_phy;
	
	vbe_chip_id = VBE_detect(&vram_size);
	if(vbe_chip_id == 0)
	{
		dbg_printf(dbg_vbe_fail);
		return FALSE;
	}

 	vram_phy = VBE_get_LFB();
	
 	if(!FBHDA_init_hw())
 	{
 		return FALSE;
 	}
 	
 	dbg_printf(dbg_vbe_init, vram_phy, vram_size);
 	
 	hda->vram_size = vram_size;
	hda->vram_pm32 = (void*)_MapPhysToLinear(vram_phy, vram_size, 0);
	hda->flags    |= FB_SUPPORT_FLIPING;
	
	memcpy(hda->vxdname, vbe_vxd_name, sizeof(vbe_vxd_name));
	
	dbg_printf(dbg_vbe_lfb, hda->vram_pm32);
	
	if(hda->vram_pm32 != NULL)
	{
		vbe_is_valid = TRUE;
	}
	
	return TRUE;
}

BOOL VBE_valid()
{
	return vbe_is_valid;
}

static DWORD VBE_pitch(DWORD width, DWORD bpp)
{
	DWORD bp = (bpp + 7) / 8;
	return (bp * width + 3) & 0xFFFFFFFC;
}

BOOL VBE_validmode(DWORD w, DWORD h, DWORD bpp)
{
	switch(bpp)
	{
		case 8:
		case 16:
		case 24:
		case 32:
			break;
		default:
			return FALSE;
	}
	
	if(w <= RES_MAX_X)
	{
		if(h <= RES_MAX_Y)
		{
			DWORD size = VBE_pitch(w, bpp) * h;
			if(size < hda->vram_size)
			{
				return TRUE;
			}
		}
	}
	
	return FALSE;
}

BOOL VBE_setmode(DWORD w, DWORD h, DWORD bpp)
{
	if(!VBE_validmode(w, h, bpp)) return FALSE;

	/* Put the hardware into a state where the mode can be safely set. */
	inp(VGA_STAT_ADDR);                   /* Reset flip-flop. */
	outp(VGA_ATTR_W, 0);                  /* Disable palette. */
	wridx(VGA_SEQUENCER, VGA_SR_RESET, VGA_SR_RESET);

	/* Disable the extended display registers. */
	outpw(VBE_DISPI_IOPORT_INDEX, VBE_DISPI_INDEX_ENABLE);
	outpw(VBE_DISPI_IOPORT_DATA, VBE_DISPI_DISABLED);

	/* Program the extended non-VGA registers. */

	/* Set X resoultion. */
	outpw(VBE_DISPI_IOPORT_INDEX, VBE_DISPI_INDEX_XRES);
	outpw(VBE_DISPI_IOPORT_DATA, (WORD)w);
	/* Set Y resoultion. */
	outpw(VBE_DISPI_IOPORT_INDEX, VBE_DISPI_INDEX_YRES);
	outpw(VBE_DISPI_IOPORT_DATA, (WORD)h);
	/* Set bits per pixel. */
	outpw(VBE_DISPI_IOPORT_INDEX, VBE_DISPI_INDEX_BPP);
	outpw(VBE_DISPI_IOPORT_DATA, (WORD)bpp);
	/* Set the virtual resolution. (same as FB resolution) */
	outpw(VBE_DISPI_IOPORT_INDEX, VBE_DISPI_INDEX_VIRT_WIDTH);
	outpw(VBE_DISPI_IOPORT_DATA, (WORD)w);
	outpw(VBE_DISPI_IOPORT_INDEX, VBE_DISPI_INDEX_VIRT_HEIGHT);
	outpw(VBE_DISPI_IOPORT_DATA, (WORD)h);
	/* Reset the current bank. */
	outpw(VBE_DISPI_IOPORT_INDEX, VBE_DISPI_INDEX_BANK);
	outpw(VBE_DISPI_IOPORT_DATA, 0);
	/* Set the X and Y display offset to 0. */
	outpw(VBE_DISPI_IOPORT_INDEX, VBE_DISPI_INDEX_X_OFFSET);
	outpw(VBE_DISPI_IOPORT_DATA, 0);
	outpw(VBE_DISPI_IOPORT_INDEX, VBE_DISPI_INDEX_Y_OFFSET);
	outpw(VBE_DISPI_IOPORT_DATA, 0);
	/* Enable the extended display registers. */
	outpw(VBE_DISPI_IOPORT_INDEX, VBE_DISPI_INDEX_ENABLE);
#ifdef QEMU
	outpw(VBE_DISPI_IOPORT_DATA, VBE_DISPI_ENABLED | VBE_DISPI_8BIT_DAC | VBE_DISPI_LFB_ENABLED | VBE_DISPI_NOCLEARMEM);
#else
	outpw(VBE_DISPI_IOPORT_DATA, VBE_DISPI_ENABLED | VBE_DISPI_8BIT_DAC);
#endif

	/* Re-enable the sequencer. */
	wridx(VGA_SEQUENCER, VGA_SR_RESET, VGA_SR0_NORESET);

	/* Reset flip-flop again and re-enable palette. */
	inp(VGA_STAT_ADDR);
	outp(VGA_ATTR_W, 0x20);
	
	hda->width  = w;
	hda->height = h;
	hda->bpp    = bpp;
	hda->pitch  = VBE_pitch(w, bpp);
	hda->stride = h * hda->pitch;
	hda->surface = 0;

	mouse_invalidate();

	return TRUE;
}

void  FBHDA_palette_set(unsigned char index, DWORD rgb)
{
	outp(VGA_DAC_W_INDEX, index);    /* Set starting index. */
	outp(VGA_DAC_DATA,   (rgb >> 16) & 0xFF);
	outp(VGA_DAC_DATA,   (rgb >>  8) & 0xFF);
	outp(VGA_DAC_DATA,    rgb        & 0xFF);
}

DWORD FBHDA_palette_get(unsigned char index)
{
	DWORD r, g, b;
	
	outp(VGA_DAC_W_INDEX, index);
	r = inp(VGA_DAC_DATA);
	g = inp(VGA_DAC_DATA);
	b = inp(VGA_DAC_DATA);
	
	return (r << 16) | (g << 8) | b;
}

BOOL FBHDA_swap(DWORD offset)
{
	DWORD ps = ((hda->bpp+7)/8);
	DWORD offset_y = offset/hda->pitch;
	DWORD offset_x = (offset % hda->pitch)/ps;
	
	if(offset + hda->stride > hda->vram_size) return FALSE; /* if exceed VRAM */
		
	FBHDA_access_begin(0);
	
	outpw(VBE_DISPI_IOPORT_INDEX, VBE_DISPI_INDEX_Y_OFFSET);
	outpw(VBE_DISPI_IOPORT_DATA, (WORD)offset_y);
	outpw(VBE_DISPI_IOPORT_INDEX, VBE_DISPI_INDEX_X_OFFSET);
	outpw(VBE_DISPI_IOPORT_DATA, (WORD)offset_x);
	
	hda->surface = offset;
	
	FBHDA_access_end(0);

	return TRUE;
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
