/*****************************************************************************

Copyright (c) 2023 Jaroslav Hensl <emulator@emulace.cz>

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
/* Communication from ring3 PM16 -> ring0 PM32 */

#include <wchar.h> /* wchar_t */
#include <string.h> /* _fmemset */
#include <stdint.h>
#include "winhack.h" /* BOOL */

#include "vmwsvxd.h"
#include "control_vxd.h"

#ifdef DBGPRINT
extern void dbg_printf( const char *s, ... );
#else
#define dbg_printf(...)
#endif

static DWORD VXD_srv = 0;

#pragma code_seg( _INIT )

BOOL VXD_load()
{
	if(VXD_srv != 0)
	{
		return TRUE;
	}
	
	_asm{
		push es
		push ax
		push dx
		push bx
		push di
		
		xor di,di
		mov es,di
		mov ax, 1684H
		mov bx, VMWSVXD_DEVICE_ID
		int 2FH
		mov word ptr [VXD_srv],di
		mov word ptr [VXD_srv+2],es
		
		pop di
		pop bx
		pop dx
		pop ax
		pop es
	};
	
	return VXD_srv != 0;
}

BOOL VXD_CreateRegion(DWORD nPages, DWORD __far *lpLAddr, DWORD __far *lpPPN, DWORD __far *lpPGBLKAddr)
{
	static DWORD snPages;
	static DWORD sLAddr;
	static DWORD sPPN;
	static DWORD sPGBLKAddr;
	static uint16_t state = 0;
	
	snPages = nPages;
	
	if(VXD_srv != 0)
	{
		_asm
		{
			.386
			push eax
			push edx
			push ecx
			push ebx
			
			mov  edx,      VMWSVXD_PM16_CREATE_REGION
			mov  ecx,      [snPages]
			call dword ptr [VXD_srv]
			mov  [state],  ax
			mov  [sLAddr], edx
			mov  [sPPN],   ecx
			mov  [sPGBLKAddr], ebx
			
			pop ebx
			pop ecx
			pop edx
			pop eax
		};
		
		if(state == 1)
		{
			*lpLAddr = sLAddr;
			*lpPPN   = sPPN;
			*lpPGBLKAddr = sPGBLKAddr;
			
			return TRUE;
		}
		else
		{
			dbg_printf("state: %X, LAddr: %lX, PPN: %lX\n", state, sLAddr, sPPN);
		}
	}
	
	return FALSE;
}

BOOL VXD_FreeRegion(DWORD LAddr, DWORD PGBLKAddr)
{
	static DWORD sLAddr;
	static uint16_t state;
	static DWORD sPGBLKAddr;
	
	sLAddr = LAddr;
	sPGBLKAddr = PGBLKAddr;
	
	if(VXD_srv != 0)
	{
		_asm
		{
			.386
			push eax
			push edx
			push ecx
			push ebx
			
			mov  edx,      VMWSVXD_PM16_DESTROY_REGION
			mov  ecx,      [sLAddr]
			mov  ebx,      [sPGBLKAddr]
			call dword ptr [VXD_srv]
			mov  [state],  ax
			
			pop ebx
			pop ecx
			pop edx
			pop eax
		};
		
		if(state == 1)
		{
			return TRUE;
		}
	}
	
	return FALSE;
}

void VXD_zeromem(DWORD LAddr, DWORD size)
{
	static DWORD sLAddr;
	static DWORD ssize;
	
	sLAddr = LAddr;
	ssize = size;
	
	if(VXD_srv != 0)
	{
		_asm
		{
			.386
			push eax
			push edx
			push ecx
			push esi
		  
		  mov edx, VMWSVXD_PM16_ZEROMEM
		  mov esi, [sLAddr]
		  mov ecx, [ssize]
		  call dword ptr [VXD_srv]
		  
		  pop esi
		  pop ecx
			pop edx
			pop eax
		}
	}
}

DWORD VXD_apiver()
{
	static DWORD sver = 0;
	
	if(VXD_srv != 0)
	{
		_asm
		{
			.386
			push eax
			push edx
			push ecx
		  
		  mov edx, VMWSVXD_PM16_APIVER
		  call dword ptr [VXD_srv]
		  mov [sver], ecx
		  
		  pop ecx
			pop edx
			pop eax
		}
	}
	
	return sver;
}


void CB_start()
{
	if(VXD_srv != 0)
	{
		_asm
		{
			.386
			push eax
			push edx
		  mov edx, VMWSVXD_PM16_CB_START
		  call dword ptr [VXD_srv]
			pop edx
			pop eax
		}
	}
}

void CB_stop()
{
	if(VXD_srv != 0)
	{
		_asm
		{
			.386
			push eax
			push edx
		  mov edx, VMWSVXD_PM16_CB_STOP
		  call dword ptr [VXD_srv]
			pop edx
			pop eax
		}
	}
}

void VXD_get_addr(DWORD __far *lpLinFB, DWORD __far *lpLinFifo, DWORD __far *lpLinFifoBounce)
{
	static DWORD linFB = 0;
	static DWORD linFifo = 0;
	static DWORD linFifoBounce = 0;
	
	if(VXD_srv != 0)
	{
		_asm
		{
			.386
			push eax
			push edx
			
			push ecx
			push esi
			push edi
			
		  mov edx, VMWSVXD_PM16_GET_ADDR
		  call dword ptr [VXD_srv]
			
			mov [linFB], ecx
			mov [linFifo], edi
			mov [linFifoBounce], esi
			
			pop edi
			pop esi
			pop ecx
			
			pop edx
			pop eax
		}
	}
	
	*lpLinFB = linFB;
	*lpLinFifo = linFifo;
	*lpLinFifoBounce = linFifoBounce;
}

void VXD_get_flags(DWORD __far *lpFlags)
{
	static DWORD flags = 0;
	
	if(VXD_srv != 0)
	{
		_asm
		{
			.386
			push eax
			push edx
			
			push ecx
			
		  mov edx, VMWSVXD_PM16_GET_FLAGS
		  call dword ptr [VXD_srv]
			
			mov [flags], ecx
			
			pop ecx
			
			pop edx
			pop eax
		}
	}
	
	*lpFlags = flags;
}

BOOL VXD_FIFOCommit(DWORD bytes)
{
	static DWORD sbytes;

	sbytes = bytes;
	
	if(VXD_srv != 0)
	{
		_asm
		{
			.386
			push eax
			push edx
			push ecx
			
			mov ecx, [sbytes]
		  mov edx, VMWSVXD_PM16_FIFO_COMMIT
		  call dword ptr [VXD_srv]
			
			pop ecx
			pop edx
			pop eax
		}
	
		return TRUE;
	}
	
	return FALSE;
}
