#include <string.h>
#include "winhack.h"

#include "drvlib.h"
#include "dpmi.h"

#include <gdidefs.h>
#include <dibeng.h>
#include <valmode.h>
#include <minivdd.h>
#include "minidrv.h"

#pragma code_seg( _INIT )

void drv_memcpy(void __far *dst, void __far *src, long size)
{
	unsigned char __far *psrc = src;
	unsigned char __far *pdst = dst;
	
	
	while(size-- > 0)
	{
		*pdst = *psrc;
		
		psrc++;
		pdst++;
	}
}


void __far *drv_malloc(DWORD dwSize, DWORD __far *lpLinear)
{
	WORD    wSel;
	DWORD   dwLin;

	wSel = DPMI_AllocLDTDesc(1);
	if(!wSel)
		return NULL;

	dwLin = DPMI_AllocMemBlk(dwSize);
    
	if(dwLin)
	{
		DPMI_SetSegBase(wSel, dwLin);
		DPMI_SetSegLimit(wSel, dwSize - 1);
    
    *lpLinear = dwLin;
    
		return wSel :> 0;
	}

	return NULL;
}

void drv_free(void __far *ptr, DWORD __far *lpLinear)
{
	DWORD sel = ((DWORD)ptr) >> 16;
	if(*lpLinear != 0)
	{
		DPMI_FreeMemBlk(*lpLinear);
		*lpLinear = 0;
	}
	
	if(sel != 0)
	{
		DPMI_FreeLDTDesc(sel);
	}
}

/**
 * MEMSET for area larger than one segment. This function is not very effective,
 * I assume it will be called only in few speed non critical situation
 * (driver/screen inicialization or reset).
 *
 * @param dwLinearBase: linear address
 * @param dwOffset: offset to linear address
 * @param value: value to set (same bahaviour as C memset)
 * @param dwNum: number of bytes to set;
 *
 **/
void drv_memset_large(DWORD dwLinearBase, DWORD dwOffset, UINT value, DWORD dwNum)
{
	WORD  wSel;
	DWORD dwLinPtr;
	DWORD dwLinPtrMax;
	WORD  wAddr;
	BYTE __far *lpPM16Ptr;
	
	wSel = DPMI_AllocLDTDesc(1);
	if(!wSel) return;
	
	dwLinPtr = dwLinearBase + dwOffset;
	dwLinPtrMax = dwLinPtr + dwNum;
	
	/* overflow or nothing to do */
	if(dwLinPtrMax <= dwLinPtr) return;
	
	DPMI_SetSegBase(wSel, dwLinPtr);
	DPMI_SetSegLimit(wSel, 0xFFFF);
	
	for(; dwLinPtr < dwLinPtrMax; dwLinPtr++)
	{
		wAddr = dwLinPtr & 0xFFFF;
		
		/* move to new segment */
		if(wAddr == 0x0000)
		{
			DPMI_SetSegBase(wSel, dwLinPtr);
		}
		
		lpPM16Ptr = (BYTE __far *)(wSel :> wAddr);
		
		*lpPM16Ptr = value;
	}
	
	DPMI_FreeLDTDesc(wSel);
}

BOOL drv_is_p2()
{
	/* CPUID detection from:
		https://www.prowaretech.com/articles/current/assembly/x86/check-for-cpuid-support
	*/
	static DWORD rc = 0;
	_asm {
		push eax
		push ebx
		pushfd                    // push eflags on the stack
		pop eax                   // pop them into eax
		mov ebx, eax              // save to ebx for restoring afterwards
		xor eax, 0x200000         // toggle bit 21
		push eax                  // push the toggled eflags
		popfd                     // pop them back into eflags
		pushfd                    // push eflags
		pop eax                   // pop them back into eax
		cmp eax, ebx              // see if bit 21 was reset
		jz non_support_cpuid
		push edx
		push ecx
		mov eax, 1
		db 0x0F                   // cpuid
		db 0xA2                   // (manual insert)
    and  edx, 0x1800000       // MMX + FXSR support
    cmp  edx, 0x1800000
    jnz  non_support_mmx
    pop  ecx
    pop  edx
    pop  ebx
    mov  eax, 1
    mov  [rc], eax
    pop  eax
    jmp done
    non_support_mmx:
    	pop ecx
    	pop edx
		non_support_cpuid:
			pop ebx
			pop eax
		done:
	};

	return (rc == 0) ? FALSE : TRUE;
}
