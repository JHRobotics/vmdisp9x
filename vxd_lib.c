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
#include "vpicd.h"

#include "svga_all.h"

#include "vxd_vdd.h"

#include "vxd_lib.h"

#include "code32.h"

#include "vxd_strings.h"

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

int memcmp(const void *ptr1, const void *ptr2, unsigned int num)
{
	const unsigned char *p1 = (const unsigned char *)ptr1;
	const unsigned char *p2 = (const unsigned char *)ptr2;

	if(num == 0) return 0;

	while(num > 1 && *p1 == *p2)
	{
		p1++;
		p2++;
		num--;
	}

	return *p1 - *p2;
}

unsigned int strlen(const char *s)
{
	const char *ptr = s;
	while(*ptr != '\0') ptr++;
	return ptr - s;
}

char *strcpy(char *dst, const char *src)
{	
	char *pdst = dst;
	
	while(*src != '\0')
	{
		*pdst = *src;
		src++;
		pdst++;
	}
	
	*pdst = '\0';
	
	return dst;
}

char *strcat(char *dst, const char *src)
{	
	unsigned int i = strlen(dst);
	strcpy(dst+i, src);
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
	WORD ver = 0;
	
	_asm push ecx
	_asm push eax
	_asm xor eax, eax
	VMMCall(Get_VMM_Version);
	_asm mov [ver],ax
	_asm pop ecx
	_asm pop eax
	
	return ver;
}

void *Get_Cur_VM_Handle()
{
	void *handle = 0;

	_asm push ebx
	VMMCall(Get_Cur_VM_Handle);
	_asm mov [handle],ebx
	_asm pop ebx

	return handle;
}

volatile void __cdecl Begin_Critical_Section(ULONG Flags)
{
	_asm push ecx
	_asm mov ecx, [Flags]
	VMMCall(Begin_Critical_Section);
	_asm pop ecx
}

volatile void __cdecl End_Critical_Section()
{
	VMMCall(End_Critical_Section);
}

ULONG __cdecl Create_Semaphore(ULONG TokenCount)
{
	ULONG SemHandle = 0;

	_asm push eax
	_asm push ecx
	_asm mov ecx, [TokenCount]
	VMMCall(Create_Semaphore);
	_asm {
		jc create_sem_error
		mov [SemHandle], eax
		create_sem_error:
		pop ecx
		pop eax
	}
	
	return SemHandle;
}

void __cdecl Destroy_Semaphore(ULONG SemHandle)
{
	_asm push eax
	_asm mov eax, [SemHandle]
	VMMCall(Destroy_Semaphore);
	_asm pop eax
}

void __cdecl Wait_Semaphore(ULONG semHandle, ULONG flags)
{
	_asm push eax
	_asm push ecx
	_asm mov eax, [semHandle]
	_asm mov ecx, [flags]
	VMMCall(Wait_Semaphore);
	_asm pop ecx
	_asm pop eax
}

void __cdecl Signal_Semaphore(ULONG SemHandle)
{
	_asm push eax
	_asm mov eax, [SemHandle]
	VMMCall(Signal_Semaphore);
	_asm pop eax
}

void __cdecl Resume_VM(ULONG VM)
{
	_asm push ebx
	_asm mov ebx, [VM]
	VMMCall(Resume_VM);
	_asm pop ebx	
}

void Release_Time_Slice()
{
	VMMCall(Release_Time_Slice);
}

void __cdecl *Map_Flat(BYTE SegOffset, BYTE OffOffset)
{
	void *result = NULL;
	
	_asm
	{
		push eax
		xor eax, eax
		mov al, [OffOffset]
		mov ah, [SegOffset]
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
/*
 * esi <- IOCallback
 * edx <- I/O port numbers
 */
	_asm
	{
		push esi
		push edx
		mov esi, [callback]
		mov edx, [port]
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
	DWORD LockablePages = 0;
	DWORD FreePages = 0;
	_asm
	{
		push edx
		push ecx
		push 0 ; flags - must be zero
	}
	VMMCall(_GetFreePageCount);
	_asm
	{
		mov [LockablePages], edx
		mov [FreePages], eax
		pop eax
		pop ecx
		pop edx
	}
	
	if(LockablePages != NULL)
	{
		*pLockablePages = LockablePages;
	}
	
	return FreePages;
}

static void __declspec(naked) __cdecl _BuildDescriptorDWORDs_(ULONG DESCBase, ULONG DESCLimit, ULONG DESCType, ULONG DESCSize, ULONG flags)
{
	VMMJmp(_BuildDescriptorDWORDs);
}

void __cdecl _BuildDescriptorDWORDs(ULONG DESCBase, ULONG DESCLimit, ULONG DESCType, ULONG DESCSize, ULONG flags, DWORD *outDescHigh, DWORD *outDescLow)
{
	DWORD High = 0;
	DWORD Low = 0;
	
	_BuildDescriptorDWORDs_(DESCBase, DESCLimit, DESCType, DESCSize, flags);
	_asm
	{
		mov [High], edx
		mov [Low],  eax
	}
	
	*outDescHigh = High;
	*outDescLow  = Low;
}

static void __declspec(naked) __cdecl _Allocate_LDT_Selector_(ULONG vm, ULONG DescHigh, ULONG DescLow, ULONG Count, ULONG flags)
{
	VMMJmp(_Allocate_LDT_Selector);
}

void __cdecl _Allocate_LDT_Selector(ULONG vm, ULONG DescHigh, ULONG DescLow, ULONG Count, ULONG flags, DWORD *outFirstSelector, DWORD *outSelectorTable)
{
	DWORD firstSelector = 0;
	DWORD selectorTable = 0;
	
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
	DWORD firstSelector = 0;
	DWORD selectorTable = 0;
	
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

ULONG __declspec(naked) __cdecl _SetDescriptor(ULONG selector, ULONG vm, DWORD DescDWORD1, DWORD DescDWORD2, ULONG flags)
{
	VMMJmp(_SetDescriptor);
}

static ULONG __declspec(naked) __cdecl _PageAllocate_call(ULONG nPages, ULONG pType, ULONG VM, ULONG AlignMask, ULONG minPhys, ULONG maxPhys, ULONG *PhysAddr, ULONG flags)
{
	VMMJmp(_PageAllocate);
}

ULONG __cdecl _PageAllocate(ULONG nPages, ULONG pType, ULONG VM, ULONG AlignMask, ULONG minPhys, ULONG maxPhys, ULONG *PhysAddr, ULONG flags)
{
	ULONG r = _PageAllocate_call(nPages, pType, VM, AlignMask, minPhys, maxPhys, PhysAddr, flags);
	
	if(r == 0)
	{
		dbg_printf("_PageAllocate FAIL - pages: %ld\n", nPages);
	}
	
#if 0
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
#endif
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

DWORD __declspec(naked) __cdecl _PageModifyPermissions(ULONG page, ULONG npages, ULONG permand, ULONG permor)
{
	VMMJmp(_PageModifyPermissions);
}

DWORD __declspec(naked) __cdecl _PageReserve(ULONG page, ULONG npages, ULONG flags)
{
	VMMJmp(_PageReserve);
}

DWORD __declspec(naked) __cdecl _PageCommit(ULONG page, ULONG npages, ULONG hpd, ULONG pagerdata, ULONG flags)
{
	VMMJmp(_PageCommit);
}

DWORD __declspec(naked) __cdecl _PageReAllocate(ULONG hMem, ULONG nPages, ULONG flags)
{
	VMMJmp(_PageReAllocate);
}

DWORD __declspec(naked) __cdecl _PageCommitPhys(ULONG page, ULONG npages, ULONG physpg, ULONG flags)
{
	VMMJmp(_PageCommitPhys);
}

DWORD __declspec(naked) __cdecl _PageCommitContig(ULONG page, ULONG npages, ULONG flags, ULONG alignmask, ULONG minphys, ULONG maxphys)
{
	VMMJmp(_PageCommitContig);
}

DWORD __declspec(naked) __cdecl _LinMapIntoV86(ULONG HLinPgNum, ULONG VM, ULONG VMLinPgNum, ULONG nPages, ULONG flags)
{
	VMMJmp(_LinMapIntoV86);
}

DWORD __declspec(naked) __cdecl _MapIntoV86(ULONG hMem, ULONG VM, ULONG VMLinPgNum, ULONG nPages, ULONG PageOff, ULONG flags)
{
	VMMJmp(_MapIntoV86);
}

DWORD __declspec(naked) __cdecl _GetFirstV86Page()
{
	VMMJmp(_GetFirstV86Page);
}

DWORD __declspec(naked) __cdecl _GetLastV86Page()
{
	VMMJmp(_GetLastV86Page);
}

DWORD __declspec(naked) __cdecl _SetLastV86Page(ULONG PgNum, ULONG flags)
{
	VMMJmp(_SetLastV86Page);
}

DWORD __declspec(naked) __cdecl _Allocate_Global_V86_Data_Area(ULONG nBytes, ULONG flags)
{
	VMMJmp(_Allocate_Global_V86_Data_Area);
}

void Enable_Global_Trapping(DWORD port)
{
	_asm push edx
	_asm mov edx, [port];
	VMMCall(Enable_Global_Trapping);
	_asm pop edx
}

void Disable_Global_Trapping(DWORD port)
{
	_asm push edx
	_asm mov edx, [port];
	VMMCall(Disable_Global_Trapping);
	_asm pop edx
}

BOOL VPICD_Virtualize_IRQ(struct _VPICD_IRQ_Descriptor *vid)
{
	BOOL r = 0;
	
	_asm {
		push edi
		mov edi, [vid]
	};
	VxDCall(VPICD, Virtualize_IRQ)
	_asm {
		jc Virtualize_IRQ_err
		mov [r], 1
		Virtualize_IRQ_err:
		pop edi
	};
	
	return r;
}

void Hook_V86_Int_Chain(DWORD int_num, DWORD HookProc)
{
	_asm mov eax, [int_num]
	_asm mov esi, [HookProc]
	VMMCall(Hook_V86_Int_Chain)
}
