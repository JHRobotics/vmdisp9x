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

/* 32 bit RING-0 system calls and CRT function */

#include "winhack.h"
#include "vmm.h"
#include "vxd.h"

#include "svga_all.h"

#include "vxd_vdd.h"

#include "vxd_lib.h"

#include "code32.h"

/* some CRT */
void memset(void *dst, int c, unsigned int size)
{
	unsigned int par;
	unsigned int i;
	unsigned int dw_count;
	unsigned int dw_size;
	
	c &= 0xFF;
	par = (c << 24) | (c << 16) | (c << 8) | c;
	dw_count = size >> 2;
	dw_size = size & 0xFFFFFFFCUL;
	
	for(i = 0; i < dw_count; i++)
	{
		((unsigned int*)dst)[i] = par; 
	}
	
	for(i = dw_size; i < size; i++)
	{
		((unsigned char*)dst)[i] = c; 
	}
}

void *memcpy(void *dst, const void *src, unsigned int size)
{
	unsigned int dw_count;
	unsigned int dw_size;
	unsigned int i;
	
	dw_count = size >> 2;
	dw_size = size & 0xFFFFFFFCUL;
	
	for(i = 0; i < dw_count; i++)
	{
		((unsigned int*)dst)[i] = ((unsigned int*)src)[i];
	}
	
	for(i = dw_size; i < size; i++)
	{
		((unsigned char*)dst)[i] = ((unsigned char*)src)[i]; 
	}
	
	return dst;
}


/**
 * VMM calls wrapers
 **/
DWORD Get_VMM_Version()
{
	static WORD ver = 0;
	
	_asm push ecx
	_asm push eax
	_asm xor eax, eax
	VMMCall(Get_VMM_Version);
	_asm mov [ver],ax
	_asm pop ecx
	_asm pop eax
	
	return ver;
}

void __cdecl Begin_Critical_Section(ULONG Flags)
{
	static ULONG sFlags;
	sFlags = Flags;
	
	_asm push ecx
	_asm mov ecx, [sFlags]
	VMMCall(Begin_Critical_Section);
	_asm pop ecx	
}

void __cdecl End_Critical_Section()
{
	VMMCall(End_Critical_Section);
}

ULONG __cdecl Create_Semaphore(ULONG TokenCount)
{
	static ULONG sTokenCount;
	static ULONG sSemHandle;
	sTokenCount = TokenCount;
	sSemHandle = 0;
	
	_asm push eax
	_asm push ecx
	_asm mov ecx, [sTokenCount]
	VMMCall(Create_Semaphore);
	_asm {
		jc create_sem_error
		mov [sSemHandle], eax
		create_sem_error:
		pop ecx
		pop eax
	}
	
	return sSemHandle;
}

void __cdecl Destroy_Semaphore(ULONG SemHandle)
{
	static ULONG sSemHandle;
	sSemHandle = SemHandle;
	
	_asm push eax
	_asm mov eax, [sSemHandle]
	VMMCall(Destroy_Semaphore);
	_asm pop eax
}

void __cdecl Wait_Semaphore(ULONG semHandle, ULONG flags)
{
	static ULONG sSemHandle;
	static ULONG sFlags;
	sSemHandle = semHandle;
	sFlags     = flags;
	
	_asm push eax
	_asm push ecx
	_asm mov eax, [sSemHandle]
	_asm mov ecx, [sFlags]
	VMMCall(Wait_Semaphore);
	_asm pop ecx
	_asm pop eax
}

void __cdecl Signal_Semaphore(ULONG SemHandle)
{
	static ULONG sSemHandle;
	sSemHandle = SemHandle;
	
	_asm push eax
	_asm mov eax, [sSemHandle]
	VMMCall(Signal_Semaphore);
	_asm pop eax
}

void __cdecl *Map_Flat(BYTE SegOffset, BYTE OffOffset)
{
	static BYTE sSegOffset;
	static BYTE sOffOffset;
	static void *result;
	
	sSegOffset = SegOffset;
	sOffOffset = OffOffset;
	
	_asm
	{
		push eax
		xor eax, eax
		mov al, [sOffOffset]
		mov ah, [sSegOffset]
	}
	VMMCall(Map_Flat);
	_asm
	{
		mov [result],eax
		pop eax
	}
	
	return result;
}

ULONG __declspec(naked) __cdecl _PageAllocate(ULONG nPages, ULONG pType, ULONG VM, ULONG AlignMask, ULONG minPhys, ULONG maxPhys, ULONG *PhysAddr, ULONG flags)
{
	VMMJmp(_PageAllocate);
}

ULONG __declspec(naked) __cdecl _PageFree(PVOID hMem, DWORD flags)
{
	VMMJmp(_PageFree);
}

ULONG __declspec(naked) __cdecl _CopyPageTable(ULONG LinPgNum, ULONG nPages, DWORD *PageBuf, ULONG flags)
{
	VMMJmp(_CopyPageTable);
}

ULONG __declspec(naked) __cdecl _LinPageLock(ULONG page, ULONG npages, ULONG flags)
{
	VMMJmp(_LinPageLock);
}

ULONG __declspec(naked) __cdecl _LinPageUnLock(ULONG page, ULONG npages, ULONG flags)
{
	VMMJmp(_LinPageUnLock);
}

ULONG __declspec(naked) __cdecl _MapPhysToLinear(ULONG PhysAddr, ULONG nBytes, ULONG flags)
{
	VMMJmp(_MapPhysToLinear);
}

ULONG __declspec(naked) __cdecl GetNulPageHandle()
{
	VMMJmp(_MapPhysToLinear);
}

ULONG __declspec(naked) __cdecl _PhysIntoV86(ULONG PhysPage, ULONG VM, ULONG VMLinPgNum, ULONG nPages, ULONG flags)
{
	VMMJmp(_PhysIntoV86);
}

DWORD __declspec(naked) __cdecl _RegOpenKey(DWORD hKey, char *lpszSubKey, DWORD *lphKey)
{
	VMMJmp(_RegOpenKey);
}

DWORD __declspec(naked) __cdecl _RegCloseKey(DWORD hKey)
{
	VMMJmp(_RegCloseKey);
}

DWORD __declspec(naked) __cdecl _RegQueryValueEx(DWORD hKey, char *lpszValueName, DWORD *lpdwReserved, DWORD *lpdwType, BYTE *lpbData, DWORD *lpcbData)
{
	VMMJmp(_RegQueryValueEx);
}

