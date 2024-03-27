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

FBHDA_t *hda = NULL;
ULONG hda_sem = 0;
LONG fb_lock_cnt = 0;

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
	dbg_printf(dbg_fbhda_setup);
	
	return hda;
}

void FBHDA_access_begin(DWORD flags)
{
	//Wait_Semaphore(hda_sem, 0);
	if(fb_lock_cnt++ == 0)
	{
		mouse_erase();
	}
}

void FBHDA_clean()
{
	FBHDA_access_begin(0);
	memset(hda->vram_pm32, 0, hda->stride);
	FBHDA_access_end(0);
}
