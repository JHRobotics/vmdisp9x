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

void memset(void *dst, int c, unsigned int size);
void *memcpy(void *dst, const void *src, unsigned int size);

DWORD Get_VMM_Version();
ULONG __cdecl _PageAllocate(ULONG nPages, ULONG pType, ULONG VM, ULONG AlignMask, ULONG minPhys, ULONG maxPhys, ULONG *PhysAddr, ULONG flags);
ULONG __cdecl _PageFree(PVOID hMem, DWORD flags);
ULONG __cdecl _CopyPageTable(ULONG LinPgNum, ULONG nPages, DWORD *PageBuf, ULONG flags);
ULONG __cdecl _LinPageLock(ULONG page, ULONG npages, ULONG flags);
ULONG __cdecl _LinPageUnLock(ULONG page, ULONG npages, ULONG flags);
ULONG __cdecl _MapPhysToLinear(ULONG PhysAddr, ULONG nBytes, ULONG flags);
ULONG __cdecl GetNulPageHandle();
ULONG __cdecl _PhysIntoV86(ULONG PhysPage, ULONG VM, ULONG VMLinPgNum, ULONG nPages, ULONG flags);
DWORD __cdecl _RegOpenKey(DWORD hKey, char *lpszSubKey, DWORD *lphKey);
DWORD __cdecl _RegCloseKey(DWORD hKey);
DWORD __cdecl _RegQueryValueEx(DWORD hKey, char *lpszValueName, DWORD *lpdwReserved, DWORD *lpdwType, BYTE *lpbData, DWORD *lpcbData);
void __cdecl Begin_Critical_Section(ULONG Flags);
void __cdecl End_Critical_Section();
ULONG __cdecl Create_Semaphore(ULONG TokenCount);
void __cdecl Destroy_Semaphore(ULONG SemHandle);
void __cdecl Wait_Semaphore(ULONG semHandle, ULONG flags);
void __cdecl Signal_Semaphore(ULONG SemHandle);

/**
 * round size in bytes to number of pages
 **/
#define RoundToPages(_size) (((_size) + P_SIZE - 1)/P_SIZE)

/* sometimes missing */
#ifndef ERROR_SUCCESS
#define ERROR_SUCCESS 0
#endif

/* debug */
#ifdef DBGPRINT
void dbg_printf( const char *s, ... );
#else
#define dbg_printf(s, ...)
#endif

