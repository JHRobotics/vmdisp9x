/*****************************************************************************

Copyright (c) 2026 Jaroslav Hensl <emulator@emulace.cz>

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
#include "vxd_halloc.h"

#include "code32.h"

#include "wram.h"
#include "async.h"

static DWORD screen_time = ASYNC_DEFAULT; // 60 Hz
static blit_t *blit = NULL;
static draw_callback_h draw_callback = NULL;
volatile DWORD *curtime = NULL;

extern LONG fb_lock_cnt;
extern FBHDA_t *hda;
extern void *DeviceCTX;

#define TIME_MIN_PLAN 4

DWORD calc_delta(DWORD draw_time)
{
	DWORD m = (*curtime) % screen_time;
	DWORD delta = screen_time - m;
	if(delta < TIME_MIN_PLAN)
	{
		delta += screen_time;
	}

	if(draw_time)
	{
		DWORD s = draw_time/screen_time;
		delta += s*screen_time;
	}
	return delta;
}

void __declspec(naked) async_timeout_proc();

void __stdcall async_timeout(DWORD tardiness, DWORD refdata)
{
	DWORD burned;
	DWORD act = *curtime;
	DWORD delta;
	
	void *curctx = _GetCurrentContext();
	//dbg_printf("CTX: %lX, device CTX: %lX\n", curctx, DeviceCTX);
	//_ContextSwitch(DeviceCTX);
	
	//dbg_printf("async_timeout %d ...", refdata);
	
	if(FBHDA_lock())
	{
		if(fb_lock_cnt == 0) /* FIXME: check if locking surface is visible surface */
		{
			draw_callback(blit);
			hda->onflip = 0;
		}
		FBHDA_unlock();
	}
	
	burned = *curtime - act;
	
	delta = calc_delta(burned);
	Set_Async_Time_Out(delta, refdata+1, async_timeout_proc);
	_ContextSwitch(curctx);
	//dbg_printf("... set (%d)!\n", delta);
}

void __declspec(naked) async_timeout_proc()
{
	//ecx = tardiness (number of extra milliseconds that have elapsed)
	//edx = refdata
	_asm
	{
		push edx
		push ecx
		call async_timeout
		retn
	};
}

BOOL async_blit_init(blit_t *blitptr, draw_callback_h cbptr)
{
	if(blitptr != NULL && cbptr != NULL)
	{
		blit = blitptr;
		draw_callback = cbptr;
		dbg_printf("Get_System_Time_Address...\n");
		curtime = Get_System_Time_Address();
		dbg_printf("curtime = %X\n", curtime);
		
		if(Set_Async_Time_Out(calc_delta(0), 0, async_timeout_proc) != 0)
		{
			return TRUE;
		}
		else
		{
			dbg_printf("Set_Async_Time_Out FAILED!\n");
		}
	}
	
	return FALSE;
}

void async_blit_settime(DWORD delay)
{
	if(delay > 0)
	{
		screen_time = delay;
	}
}
