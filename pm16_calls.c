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

#include "dpmi.h"

#include "pm16_calls.h"
#include "vxd.h"
#include "3d_accel.h"

#ifdef DBGPRINT
extern void dbg_printf( const char *s, ... );
#else
#define dbg_printf(...)
#endif

#ifndef SVGA
# ifndef VESA
#  define VBE
# endif
#endif

static DWORD VXD_VM = 0;

#pragma code_seg( _INIT )

BOOL VXD_VM_connect()
{
	if(VXD_VM != 0)
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
		mov bx, VXD_DEVICE_ID
		int 2FH
		mov word ptr [VXD_VM],di
		mov word ptr [VXD_VM+2],es
		
		pop di
		pop bx
		pop dx
		pop ax
		pop es
	};
	
	return VXD_VM != 0;
}

void FBHDA_setup(FBHDA_t __far* __far* FBHDA, DWORD __far* FBHDA_linear)
{
	static DWORD linear;
	linear = 0;
	
	_asm
	{
		.386
		push eax
		push edx
		push ecx
	  
	  mov edx, OP_FBHDA_SETUP
	  call dword ptr [VXD_VM]
	  mov  [linear], ecx
	  
	  pop ecx
		pop edx
		pop eax
	}
	
	if(linear)
	{
		void __far *longptr = NULL;
		WORD desc = DPMI_AllocLDTDesc(1);
		DPMI_SetSegBase(desc,  linear);
		DPMI_SetSegLimit(desc, sizeof(FBHDA_t)-1);
		
		longptr = desc :> 0;
			
		*FBHDA = (FBHDA_t __far*)longptr;
		*FBHDA_linear = linear;
	}
}

void FBHDA_access_begin(DWORD flags)
{
	static DWORD sFlags;
	sFlags = flags;
	
	_asm
	{
		.386
		push eax
		push edx
		push ecx
		
	  mov edx, OP_FBHDA_ACCESS_BEGIN
	  mov ecx, [sFlags]
	  call dword ptr [VXD_VM]
	  
	  pop ecx
		pop edx
		pop eax
	}
}

void FBHDA_access_end(DWORD flags)
{
	static DWORD sFlags;
	sFlags = flags;
	
	_asm
	{
		.386
		push eax
		push edx
		push ecx
		
	  mov edx, OP_FBHDA_ACCESS_END
	  mov ecx, [sFlags]
	  call dword ptr [VXD_VM]
	  
	  pop ecx
		pop edx
		pop eax
	}
}

BOOL FBHDA_swap(DWORD offset)
{
	static DWORD sOffset;
	static BOOL status;
	sOffset = offset;
	status = FALSE;
	
	_asm
	{
		.386
		push eax
		push edx
		push ecx
		
	  mov edx, OP_FBHDA_SWAP
	  mov ecx, [sOffset]
	  call dword ptr [VXD_VM]
	  mov [status], cx
	  
	  pop ecx
		pop edx
		pop eax
	}
	
	return status == 0 ? FALSE : TRUE;
}

void FBHDA_clean()
{
	_asm
	{
		.386
		push eax
		push edx
		
	  mov edx, OP_FBHDA_CLEAN
	  call dword ptr [VXD_VM]
	  
		pop edx
		pop eax
	}
}

void FBHDA_palette_set(unsigned char index, DWORD rgb)
{
	static unsigned char sIndex;
	static DWORD sRGB;
	
	sIndex = index;
	sRGB = rgb;
	
	_asm
	{
		.386
		push eax
		push edx
		push ecx
		push edi
		
	  mov edx, OP_FBHDA_PALETTE_SET
	  mov cl, [index]
	  mov edi, [sRGB]
	  call dword ptr [VXD_VM]
	  
	  pop edi
	  pop ecx
		pop edx
		pop eax
	}
}

DWORD FBHDA_palette_get(unsigned char index)
{
	static unsigned char sIndex;
	static DWORD sRGB;
	
	sIndex = index;
	sRGB = 0;
	
	_asm
	{
		.386
		push eax
		push edx
		push ecx
		push edi
		
	  mov edx, OP_FBHDA_PALETTE_GET
	  mov cl, [index]
	  call dword ptr [VXD_VM]
	  mov [sRGB], edi
	  
	  pop edi
	  pop ecx
		pop edx
		pop eax
	}
	
	return sRGB;
}

BOOL mouse_load()
{
	static BOOL status;
	
	_asm
	{
		.386
		push eax
		push edx
		push ecx
		
		mov edx, OP_MOUSE_LOAD
		call dword ptr [VXD_VM]
		mov [status],cx
		
		pop ecx
		pop edx
		pop eax
	}
	
	return status == 0 ? FALSE : TRUE;
}

void mouse_buffer(void __far* __far* pBuf, DWORD __far* pLinear)
{
	static DWORD buffer;
	
	_asm
	{
		.386
		push eax
		push edx
		push ecx
		
		mov edx, OP_MOUSE_BUFFER
		call dword ptr [VXD_VM]
		mov [buffer],ecx
		
		pop ecx
		pop edx
		pop eax
	}
	
	if(buffer)
	{
		void __far *longptr = NULL;
		WORD desc = DPMI_AllocLDTDesc(1);
		DPMI_SetSegBase(desc,  buffer);
		DPMI_SetSegLimit(desc, MOUSE_BUFFER_SIZE);
		
		longptr = desc :> 0;
			
		*pBuf = longptr;
		*pLinear = buffer;
	}
}

void mouse_move(int x, int y)
{
	static DWORD sx;
	static DWORD sy;
	
	sx = x;
	sy = y;
	
	_asm
	{
		.386
		push eax
		push edx
		push esi
		push edi
		
		mov esi, [sx]
		mov edi, [sy]
		mov edx, OP_MOUSE_MOVE
		call dword ptr [VXD_VM]
		
		pop edi
		pop esi
		pop edx
		pop eax
	}
}

void mouse_show()
{
	_asm
	{
		.386
		push eax
		push edx
		
		mov edx, OP_MOUSE_SHOW
		call dword ptr [VXD_VM]
		
		pop edx
		pop eax
	}
}

void mouse_hide()
{
	_asm
	{
		.386
		push eax
		push edx
		
		mov edx, OP_MOUSE_HIDE
		call dword ptr [VXD_VM]
		
		pop edx
		pop eax
	}
}


#ifdef SVGA
BOOL SVGA_valid()
{
	static BOOL status;
	status = FALSE;
	
	_asm
	{
		.386
		push eax
		push edx
		push ecx
	  
	  mov edx, OP_SVGA_VALID
	  call dword ptr [VXD_VM]
	  mov  [status], cx
	  
	  pop ecx
		pop edx
		pop eax
	}
	
	return status == 0 ? FALSE : TRUE;
}

BOOL SVGA_setmode(DWORD w, DWORD h, DWORD bpp)
{
	static BOOL status;
	static DWORD sw, sh, sbpp;
	
	status = FALSE;
	sw = w;
	sh = h;
	sbpp = bpp;
	
	_asm
	{
		.386
		push eax
		push edx
		push ecx
		push esi
		push edi
	  
	  mov edx, OP_SVGA_SETMODE
	  mov ecx, [bpp]
	  mov esi, [w]
	  mov edi, [h]
	  call dword ptr [VXD_VM]
	  mov  [status], cx
	  
	  pop edi
	  pop esi
	  pop ecx
		pop edx
		pop eax
	}
	
	return status == 0 ? FALSE : TRUE;
}

BOOL SVGA_validmode(DWORD w, DWORD h, DWORD bpp)
{
	static BOOL status;
	static DWORD sw, sh, sbpp;
	
	status = FALSE;
	sw = w;
	sh = h;
	sbpp = bpp;
	
	_asm
	{
		.386
		push eax
		push edx
		push ecx
		push esi
		push edi
	  
	  mov edx, OP_SVGA_VALIDMODE
	  mov ecx, [bpp]
	  mov esi, [w]
	  mov edi, [h]
	  call dword ptr [VXD_VM]
	  mov  [status], cx
	  
	  pop edi
	  pop esi
	  pop ecx
		pop edx
		pop eax
	}
	
	return status == 0 ? FALSE : TRUE;
}

void SVGA_HW_enable()
{
	_asm
	{
		.386
		push eax
		push edx
		
	  mov edx, OP_SVGA_HW_ENABLE
	  call dword ptr [VXD_VM]
	  
		pop edx
		pop eax
	}
}

void SVGA_HW_disable()
{
	_asm
	{
		.386
		push eax
		push edx
		
	  mov edx, OP_SVGA_HW_DISABLE
	  call dword ptr [VXD_VM]
	  
		pop edx
		pop eax
	}
}
#endif

#ifdef VBE
BOOL VBE_valid()
{
	static BOOL status;
	status = FALSE;
	
	_asm
	{
		.386
		push eax
		push edx
		push ecx
	  
	  mov edx, OP_VBE_VALID
	  call dword ptr [VXD_VM]
	  mov  [status], cx
	  
	  pop ecx
		pop edx
		pop eax
	}
	
	return status == 0 ? FALSE : TRUE;
}

BOOL VBE_setmode(DWORD w, DWORD h, DWORD bpp)
{
	static BOOL status;
	static DWORD sw, sh, sbpp;
	
	status = FALSE;
	sw = w;
	sh = h;
	sbpp = bpp;
	
	_asm
	{
		.386
		push eax
		push edx
		push ecx
		push esi
		push edi
	  
	  mov edx, OP_VBE_SETMODE
	  mov ecx, [bpp]
	  mov esi, [w]
	  mov edi, [h]
	  call dword ptr [VXD_VM]
	  mov  [status], cx
	  
	  pop edi
	  pop esi
	  pop ecx
		pop edx
		pop eax
	}
	
	return status == 0 ? FALSE : TRUE;
}

BOOL VBE_validmode(DWORD w, DWORD h, DWORD bpp)
{
	static BOOL status;
	static DWORD sw, sh, sbpp;
	
	status = FALSE;
	sw = w;
	sh = h;
	sbpp = bpp;
	
	_asm
	{
		.386
		push eax
		push edx
		push ecx
		push esi
		push edi
	  
	  mov edx, OP_VBE_VALIDMODE
	  mov ecx, [bpp]
	  mov esi, [w]
	  mov edi, [h]
	  call dword ptr [VXD_VM]
	  mov  [status], cx
	  
	  pop edi
	  pop esi
	  pop ecx
		pop edx
		pop eax
	}
	
	return status == 0 ? FALSE : TRUE;
}

#endif /* VBE */
