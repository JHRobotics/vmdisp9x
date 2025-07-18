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
int memcmp(const void *ptr1, const void *ptr2, unsigned int num);
unsigned int strlen(const char *s);
char *strcpy(char *dst, const char *src);
char *strcat(char *dst, const char *src);

BOOL RegReadConf(UINT root, const char *path, const char *name, DWORD *out);

DWORD Get_VMM_Version();
void *Get_Cur_VM_Handle();
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
DWORD __cdecl _PageModifyPermissions(ULONG page, ULONG npages, ULONG permand, ULONG permor);
volatile void __cdecl Begin_Critical_Section(ULONG Flags);
volatile void __cdecl End_Critical_Section();
ULONG __cdecl Create_Semaphore(ULONG TokenCount);
void __cdecl Destroy_Semaphore(ULONG SemHandle);
void __cdecl Wait_Semaphore(ULONG semHandle, ULONG flags);
void __cdecl Signal_Semaphore(ULONG SemHandle);
void __cdecl *Map_Flat(BYTE SegOffset, BYTE OffOfset);
void __cdecl Install_IO_Handler(DWORD port, DWORD callback);
DWORD __cdecl _GetFreePageCount(DWORD *pLockablePages);

DWORD __cdecl _PageReserve(ULONG page, ULONG npages, ULONG flags);
DWORD __cdecl _PageCommit(ULONG page, ULONG npages, ULONG hpd, ULONG pagerdata, ULONG flags);
DWORD __cdecl _PageCommitPhys(ULONG page, ULONG npages, ULONG physpg, ULONG flags);
DWORD __cdecl _PageReAllocate(ULONG hMem, ULONG nPages, ULONG flags);
DWORD __cdecl _PageCommitContig(ULONG page, ULONG npages, ULONG flags, ULONG alignmask, ULONG minphys, ULONG maxphys);
DWORD __cdecl _LinMapIntoV86(ULONG HLinPgNum, ULONG VM, ULONG VMLinPgNum, ULONG nPages, ULONG flags);
DWORD __cdecl _MapIntoV86(ULONG hMem, ULONG VM, ULONG VMLinPgNum, ULONG nPages, ULONG PageOff, ULONG flags);
DWORD __cdecl _Allocate_Global_V86_Data_Area(ULONG nBytes, ULONG flags);
DWORD __cdecl _GetFirstV86Page();
DWORD __cdecl _GetLastV86Page();
DWORD __cdecl _SetLastV86Page(ULONG PgNum, ULONG flags);

void __cdecl Resume_VM(ULONG VM);
void Release_Time_Slice();

void __cdecl _BuildDescriptorDWORDs(ULONG DESCBase, ULONG DESCLimit, ULONG DESCType, ULONG DESCSize, ULONG flags, DWORD *outDescHigh, DWORD *outDescLow);
void __cdecl _Allocate_LDT_Selector(ULONG vm, ULONG DescHigh, ULONG DescLow, ULONG Count, ULONG flags, DWORD *outFirstSelector, DWORD *outSelectorTable);
void __cdecl _Allocate_GDT_Selector(ULONG DescHigh, ULONG DescLow, ULONG flags, DWORD *outFirstSelector, DWORD *outSelectorTable);
ULONG __cdecl _SetDescriptor(ULONG selector, ULONG vm, DWORD DescDWORD1, DWORD DescDWORD2, ULONG flags);

void Hook_V86_Int_Chain(DWORD int_num, DWORD HookProc);

void Enable_Global_Trapping(DWORD port);
void Disable_Global_Trapping(DWORD port);

/**
 * round size in bytes to number of pages
 **/
#define RoundToPages(_size) (((_size) + P_SIZE - 1)/P_SIZE)

#define RoundTo4k(_n)  (((_n) + 0x0FFFUL) & 0xFFFFF000UL)
#define RoundTo64k(_n) (((_n) + 0xFFFFUL) & 0xFFFF0000UL)

/**
 * PM16 to PM32 memory mapping
 **/
#define INVALID_FLAT_ADDRESS ((void*)(0xFFFFFFFFUL))
#define Client_Ptr_Flat(_pcrs, _segReg, _offReg) Map_Flat( \
	((DWORD)(&(_pcrs->_segReg)))-((DWORD)(_pcrs)), \
	((DWORD)(&(_pcrs->_offReg)))-((DWORD)(_pcrs)) )

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

struct _VPICD_IRQ_Descriptor;

BOOL VPICD_Virtualize_IRQ(struct _VPICD_IRQ_Descriptor *vid);

/* extra FBHA */
void FBHDA_update_heap_size(BOOL init);
void FBHDA_memtest();
