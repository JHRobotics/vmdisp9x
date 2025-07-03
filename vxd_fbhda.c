/*****************************************************************************

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

#include "winhack.h"
#include "vmm.h"
#include "vxd.h"

#include "vxd_lib.h"
#include "3d_accel.h"

#include "code32.h"

#include "vxd_gamma.h"

FBHDA_t *hda = NULL;
ULONG hda_sem = 0;
LONG fb_lock_cnt = 0;
DWORD gamma_quirk = 0;

#include "vxd_strings.h"

BOOL FBHDA_init_hw()
{
	hda = (FBHDA_t *)_PageAllocate(RoundToPages(sizeof(FBHDA_t)), PG_SYS, 0, 0, 0x0, 0x100000, NULL, PAGEFIXED);
	if(hda)
	{
		memset(hda, 0, sizeof(FBHDA_t));
	
		hda->cb = sizeof(FBHDA_t);
		hda->version = API_3DACCEL_VER;
		hda->flags = 0;
		
		hda->gamma = 1 << 16;
		
		hda_sem = Create_Semaphore(1);
		if(hda_sem == 0)
		{
			_PageFree(hda, 0);
			return FALSE;
		}
		
		return TRUE;
	}
	return FALSE;	
}

void FBHDA_update_heap_size(BOOL init)
{
	if(hda)
	{
		DWORD bottom = hda->vram_size - hda->overlays_size;
		DWORD blocks = bottom/FB_VRAM_HEAP_GRANULARITY;
		DWORD blocks_size = blocks*sizeof(DWORD);
		DWORD top;
		DWORD start;

		blocks_size += ((FB_VRAM_HEAP_GRANULARITY - (blocks_size) % FB_VRAM_HEAP_GRANULARITY)) % FB_VRAM_HEAP_GRANULARITY;

		hda->heap_end = ((BYTE*)hda->vram_pm32) + (blocks*FB_VRAM_HEAP_GRANULARITY - blocks_size);
		hda->heap_info = (DWORD*)hda->heap_end;

		top = hda->system_surface + hda->stride;
		start = hda->heap_end - ((BYTE*)hda->vram_pm32 + top);
		
		hda->heap_count  = blocks;
		hda->heap_length = start / FB_VRAM_HEAP_GRANULARITY;
		hda->heap_start = hda->heap_end - FB_VRAM_HEAP_GRANULARITY*hda->heap_length;
		
		dbg_printf("FBHDA_update_heap_size(start=%lu bottom=%lu, hda->heap_length=%lu)\n", start, bottom, hda->heap_length);

		if(init)
		{
			DWORD i;
			for(i = 0; i < blocks; i++)
			{
				hda->heap_info[i] = (~0UL);
			}
		}
	}
}

void FBHDA_release_hw()
{
	if(hda)
	{
		_PageFree(hda, 0);
	}
	
	if(hda_sem)
	{
		Destroy_Semaphore(hda_sem);
	}
}

FBHDA_t *FBHDA_setup()
{
	dbg_printf("FBHDA_setup()\n");
	dbg_printf("sizeof(FBHDA_t) = %ld\n", sizeof(FBHDA_t));
	
	return hda;
}

void FBHDA_clean()
{
	FBHDA_access_begin(0);
	memset(hda->vram_pm32, 0, hda->stride);
	FBHDA_access_end(0);
}

static WORD gamma_ramp[3][256];
static BOOL gamma_ramp_init = FALSE;

static void init_ramp()
{
	WORD i = 0;
	for(i = 0; i < 256; i++)
	{
		gamma_ramp[0][i] = (i << 8) | i;
		gamma_ramp[1][i] = (i << 8) | i;
		gamma_ramp[2][i] = (i << 8) | i;
	}
	
	gamma_ramp_init = TRUE;
}

BOOL FBHDA_gamma_get(VOID FBPTR ramp, DWORD buffer_size)
{
	if(sizeof(gamma_ramp) == buffer_size)
	{
		if(!gamma_ramp_init)
			init_ramp();
		
		memcpy(ramp, &gamma_ramp[0][0], sizeof(gamma_ramp));
		return TRUE;
	}
	else
	{
		dbg_printf("Wrong ramp size: %ld\n", buffer_size);
	}
	
	return FALSE;
}

BOOL FBHDA_gamma_set(VOID FBPTR ramp, DWORD buffer_size)
{
	if(sizeof(gamma_ramp) == buffer_size)
	{
		WORD *new_ramp = ramp;
		DWORD gamma = 0;
		DWORD used_quirk = gamma_quirk;
		if(used_quirk == 0)
		{
			if(new_ramp[0*256 + 128] == 0xFFFF)
			{
				used_quirk = 1;
			}
			else
			{
				used_quirk = 2;
			}
		}
		
		dbg_printf("Gamma quirk: %ld\n", used_quirk);

		if(used_quirk == 1) /* gamma quirk for ID Software games */
		{
			gamma = (
				gamma_table[(new_ramp[0*256 + 64] >> 8)] +
				gamma_table[(new_ramp[1*256 + 64] >> 8)] +
				gamma_table[(new_ramp[2*256 + 64] >> 8)]
			) / 3;
			
			if(gamma >= 0x3000 && gamma < 0x1000000) /* from ~0.2000 to 256.0000 */
			{
				int i = 0;
				hda->gamma = gamma;
				hda->gamma_update++;
				
				/* copy new ramp and duplicate odd values */
				for(i = 0; i < 128; i ++)
				{
					gamma_ramp[0][(i*2) + 0] = new_ramp[0*256 + i];
					gamma_ramp[0][(i*2) + 1] = new_ramp[0*256 + i];
					gamma_ramp[1][(i*2) + 0] = new_ramp[1*256 + i];
					gamma_ramp[1][(i*2) + 1] = new_ramp[1*256 + i];
					gamma_ramp[2][(i*2) + 0] = new_ramp[2*256 + i];
					gamma_ramp[2][(i*2) + 1] = new_ramp[2*256 + i];
				}
				
				gamma_ramp_init = TRUE;
				
				dbg_printf("gamma update to 0x%lX\n", hda->gamma);
				
				return TRUE;
			}
		}
		else /* normal way by MS specification */
		{
			gamma = (
				gamma_table[(new_ramp[0*256 + 128] >> 8)] +
				gamma_table[(new_ramp[1*256 + 128] >> 8)] +
				gamma_table[(new_ramp[2*256 + 128] >> 8)]
			) / 3;
			
			if(gamma >= 0x3000 && gamma < 0x1000000) /* from ~0.2000 to 256.0000 */
			{
				hda->gamma = gamma;
				hda->gamma_update++;
				
				/* copy new ramp */
				memcpy(&gamma_ramp[0][0], ramp, sizeof(gamma_ramp));
				gamma_ramp_init = TRUE;
				
				dbg_printf("gamma update to 0x%lX\n", hda->gamma);
				
				return TRUE;
			}
		}

		dbg_printf("Gamma out of reach: 0x%lX\n", gamma);
		{
			int i = 0;
			dbg_printf("RAMP\n");
			for(i = 0; i < 256; i++)
			{
				dbg_printf("%d: R: %X, G: %X, B: %X\n", i,
					new_ramp[0*256 + i],
					new_ramp[1*256 + i],
					new_ramp[2*256 + i]);
			}
		}
	}
	else
	{
		dbg_printf("Wrong ramp size: %ld\n", buffer_size);
	}

	return FALSE;
}

