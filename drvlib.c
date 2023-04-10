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

	/* Map the framebuffer physical memory. */
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
