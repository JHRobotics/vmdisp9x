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

/* temp for reading registry */
static union
{
	DWORD dw;
	char str[64];
} RegReadConfBuf = {0};

/**
 * Read integer value from registry
 *
 **/ 
BOOL RegReadConf(UINT root, const char *path, const char *name, DWORD *out)
{
	DWORD hKey;
	DWORD type;
	DWORD cbData = sizeof(RegReadConfBuf);
	BOOL rv = FALSE;
	
	if(_RegOpenKey(root, (char*)path, &hKey) == ERROR_SUCCESS)
	{
		if(_RegQueryValueEx(hKey, (char*)name, 0, &type, &(RegReadConfBuf.str), &cbData) == ERROR_SUCCESS)
		{
			if(type == REG_SZ)
			{
				DWORD cdw = 0;
				char *ptr = &(RegReadConfBuf.str);
				
				while(*ptr == ' '){ptr++;}
				
				while(*ptr >= '0' && *ptr <= '9')
				{
					cdw *= 10;
					cdw += *ptr - '0';
					ptr++;
				}
				
				*out = cdw;
			}
			else
			{
				*out = RegReadConfBuf.dw;
			}
			
			rv = TRUE;
		}
		
		_RegCloseKey(hKey);
	}
	
	return rv;
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

volatile void __cdecl Begin_Critical_Section(ULONG Flags)
{
	static ULONG sFlags;
	sFlags = Flags;
	
	_asm push ecx
	_asm mov ecx, [sFlags]
	VMMCall(Begin_Critical_Section);
	_asm pop ecx	
}

volatile  void __cdecl End_Critical_Section()
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

void __cdecl Resume_VM(ULONG VM)
{
	static ULONG sVM;
	sVM = VM;
	
	_asm push ebx
	_asm mov ebx, [sVM]
	VMMCall(Resume_VM);
	_asm pop ebx	
}

void Release_Time_Slice()
{
	VMMCall(Release_Time_Slice);
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

void __cdecl Install_IO_Handler(DWORD port, DWORD callback)
{
	static DWORD sPort = 0;
	static DWORD sCallback = 0;

	sPort = port;
	sCallback = callback;
	
/*
 * esi <- IOCallback
 * edx <- I/O port numbers
 */
	_asm
	{
		push esi
		push edx
		mov esi, [sCallback]
		mov edx, [sPort]
	}
	VMMCall(Install_IO_Handler);
	_asm
	{
		pop edx
		pop esi
	}
}

DWORD __cdecl _GetFreePageCount(DWORD *pLockablePages)
{
	static DWORD sLockablePages = 0;
	static DWORD sFreePages = 0;
	_asm
	{
		push edx
		push ecx
		push 0 ; flags - must be zero
	}
	VMMCall(_GetFreePageCount);
	_asm
	{
		mov [sLockablePages], edx
		mov [sFreePages], eax
		pop eax
		pop ecx
		pop edx
	}
	
	if(pLockablePages != NULL)
	{
		*pLockablePages = sLockablePages;
	}
	
	return sFreePages;
}

static void __declspec(naked) __cdecl _BuildDescriptorDWORDs_(ULONG DESCBase, ULONG DESCLimit, ULONG DESCType, ULONG DESCSize, ULONG flags)
{
	VMMJmp(_BuildDescriptorDWORDs);
}

void __cdecl _BuildDescriptorDWORDs(ULONG DESCBase, ULONG DESCLimit, ULONG DESCType, ULONG DESCSize, ULONG flags, DWORD *outDescHigh, DWORD *outDescLow)
{
	static DWORD sHigh;
	static DWORD sLow;
	
	_BuildDescriptorDWORDs_(DESCBase, DESCLimit, DESCType, DESCSize, flags);
	_asm
	{
		mov [sHigh], edx
		mov [sLow],  eax
	}
	
	*outDescHigh = sHigh;
	*outDescLow  = sLow;
}

static void __declspec(naked) __cdecl _Allocate_LDT_Selector_(ULONG vm, ULONG DescHigh, ULONG DescLow, ULONG Count, ULONG flags)
{
	VMMJmp(_Allocate_LDT_Selector);
}

void __cdecl _Allocate_LDT_Selector(ULONG vm, ULONG DescHigh, ULONG DescLow, ULONG Count, ULONG flags, DWORD *outFirstSelector, DWORD *outSelectorTable)
{
	static DWORD firstSelector;
	static DWORD selectorTable;
	
	_Allocate_LDT_Selector_(vm, DescHigh, DescLow, Count, flags);
	_asm
	{
		mov [firstSelector], eax
		mov [selectorTable], edx
	}
	
	if(outFirstSelector != NULL)
	{
		*outFirstSelector = firstSelector;
	}
	
	if(outSelectorTable != NULL)
	{
		*outSelectorTable = selectorTable;
	}
}

static void __declspec(naked) __cdecl _Allocate_GDT_Selector_(ULONG DescHigh, ULONG DescLow, ULONG flags)
{
	VMMJmp(_Allocate_GDT_Selector);
}

void __cdecl _Allocate_GDT_Selector(ULONG DescHigh, ULONG DescLow, ULONG flags, DWORD *outFirstSelector, DWORD *outSelectorTable)
{
	static DWORD firstSelector;
	static DWORD selectorTable;
	
	_Allocate_GDT_Selector_(DescHigh, DescLow, flags);
	_asm
	{
		mov [firstSelector], eax
		mov [selectorTable], edx
	}
	
	if(outFirstSelector != NULL)
	{
		*outFirstSelector = firstSelector;
	}
	
	if(outSelectorTable != NULL)
	{
		*outSelectorTable = selectorTable;
	}
}

static ULONG __declspec(naked) __cdecl _PageAllocate_call(ULONG nPages, ULONG pType, ULONG VM, ULONG AlignMask, ULONG minPhys, ULONG maxPhys, ULONG *PhysAddr, ULONG flags)
{
	VMMJmp(_PageAllocate);
}

ULONG __cdecl _PageAllocate(ULONG nPages, ULONG pType, ULONG VM, ULONG AlignMask, ULONG minPhys, ULONG maxPhys, ULONG *PhysAddr, ULONG flags)
{
	ULONG r = _PageAllocate_call(nPages, pType, VM, AlignMask, minPhys, maxPhys, PhysAddr, flags);
	
	/* do examplicit TLB flush, help much for stability.
	 * There is probably another TLB bug in VMM...
	 */
	_asm
	{
		push eax
		mov eax, cr3
		mov cr3, eax
		pop eax
	};
	return r;
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

