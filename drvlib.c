#include <string.h>
#include "winhack.h"

#include "drvlib.h"
#include "dpmi.h"

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
