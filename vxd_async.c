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
static DWORD target_time = 0;

extern LONG fb_lock_cnt;
extern FBHDA_t *hda;

DWORD thread_handle = NULL;

static BOOL thead_init = FALSE;

#define TIME_MIN_PLAN 4

BOOL async_blit_init(blit_t *blitptr, draw_callback_h cbptr)
{
	if(!thead_init)
	{
		async_init();
		thead_init = TRUE;
	}
	
	if(blitptr != NULL && cbptr != NULL)
	{
		blit = blitptr;
		draw_callback = cbptr;
		return TRUE;
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

BOOL async_watchdog()
{
	// ...
	return TRUE;
}

void __stdcall async_thread_main(DWORD dwStartParam)
{
	dbg_printf("async_thread_main\n");
	for(;;)
	{
		DWORD wait_time = 0;
		if((*curtime) > target_time)
		{
			target_time = (*curtime) + screen_time;
			if(draw_callback)
			{
				if(fb_lock_cnt == 0) /* FIXME: check if locking surface is visible surface */
				{
					draw_callback(blit);
					hda->onflip = 0;
				}
			}
		}
		wait_time = target_time - (*curtime);
		if(wait_time > screen_time) /* on overflow for example */
		{
			wait_time = screen_time;
		}

		/* task switch */
		Time_Slice_Sleep(wait_time);
	}
}

/*
	Create Extra RING0 thread
	
	IMPORTANT: VWIN32_CreateRing0Thread MUST be called at APPY time, so don't call this
	directly but as callback from SHELL_CallAtAppyTime
 */
static void __cdecl create_thread(DWORD dwRefData, DWORD dwFlags)
{
	dbg_printf("create_thread\n");
	VWIN32_CreateRing0Thread(4096, 0, async_thread_main, NULL, &thread_handle);
	dbg_printf("create_thread DONE\n");
}

void async_init()
{
	dbg_printf("async_init\n");
	curtime = Get_System_Time_Address();
	SHELL_CallAtAppyTime(create_thread, 0, /*CAAFL_RING0*/0, 0);
	dbg_printf("async_init DONE\n");
}
