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

#ifndef VXD32
#error VXD32 not defined!
#endif

#define VXD_MAIN

#include "winhack.h"
#include "vmm.h"
#include "vxd.h"
#include "vxd_lib.h"

#include "3d_accel.h"
#include "vxd_vdd.h"

#ifdef SVGA
# include "svga_all.h"
#endif

#include "version.h"

#include "code32.h"

void VMWS_control();
void VMWS_API_entry();
void Device_Init_proc();
void __stdcall Device_Dynamic_Init_proc(DWORD VM);
void __stdcall Device_Exit_proc(DWORD VM);

/*
  VXD structure
  this variable must be in first address in code segment.
  In other cases VXD isn't loadable (WLINK bug?)
*/
DDB VMWS_DDB = {
	NULL,                       // must be NULL
	DDK_VERSION,                // DDK_Version
	VXD_DEVICE_ID,              // Device ID
	VXD_MAJOR_VER,              // Major Version
	VXD_MINOR_VER,              // Minor Version
	NULL,
	"VMWSVXD",
	VDD_Init_Order, //Undefined_Init_Order,
	(DWORD)VMWS_control,
	(DWORD)VMWS_API_entry,
	(DWORD)VMWS_API_entry,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL, // DDB_Win32_Service_Table
	NULL, // prev
	sizeof(DDB)
};

#include "vxd_strings.h"

DWORD *DispatchTable = 0;
DWORD DispatchTableLength = 0;
DWORD ThisVM = 0;

/**
 * VDD calls wrapers
 **/
void VDD_Get_Mini_Dispatch_Table()
{
	VxDCall(VDD, Get_Mini_Dispatch_Table);
	_asm mov [DispatchTable],edi
	_asm mov [DispatchTableLength],ecx
}

/**
 * Control Handles
 **/
void Sys_Critical_Init_proc()
{
	// nop
}

DWORD __stdcall Device_IO_Control_proc(struct DIOCParams *params);

/*
 * 32-bit DeviceIoControl ends here
 *
 */
void __declspec(naked) Device_IO_Control_entry()
{
	_asm {
		push esi /* struct DIOCParams */
		call Device_IO_Control_proc
		retn
	}
}

/*
 * service module calls (init, exit, deviceIoControl, ...)
 * clear carry if succes (this is always success)
 */
void __declspec(naked) VMWS_control()
{
	// eax = 0x00 - Sys Critical Init
	// eax = 0x01 - sys dynamic init
	// eax = 0x1B - dynamic init
	// eax = 0x1C - dynamic exit
	// eax = 0x23 - device IO control
	
	_asm {
		cmp eax,Sys_Critical_Init
		jnz control_1
			pushad
			call Sys_Critical_Init_proc
			popad
			clc
			ret
		control_1: 
		cmp eax,Device_Init
		jnz control_2
		  call Device_Init_proc
			popad
			clc
			ret
		control_2:
		cmp eax,0x1B
		jnz control_3
			pushad
			push ebx ; VM handle
		  call Device_Dynamic_Init_proc
		  popad
			clc
			ret
		control_3:
		cmp eax,W32_DEVICEIOCONTROL
		jnz control_4
			jmp Device_IO_Control_entry
		control_4:
		cmp eax,System_Exit
		jnz control_5
			pushad
			push ebx ; VM handle
		  call Device_Exit_proc
			popad
			clc
			ret
		control_5:	
		clc
	  ret
	};
}

/* process V86/PM16 calls */
WORD __stdcall VMWS_API_Proc(PCRS_32 state)
{
	WORD rc = 0xFFFF;
	WORD service = state->Client_EDX & 0xFFFF;
	switch(service)
	{
		case VXD_PM16_VERSION:
			rc = (VXD_MAJOR_VER << 8) | VXD_MINOR_VER;
			break;
		case VXD_PM16_VMM_VERSION:
			rc = Get_VMM_Version();
			break;
		case OP_FBHDA_SETUP:
			{
				DWORD dptr = (DWORD)FBHDA_setup();
				if(dptr)
				{
					state->Client_ECX = dptr;
					rc = 1;
				}
				break;
			}
			break;
		case OP_FBHDA_ACCESS_BEGIN:
			FBHDA_access_begin(state->Client_ECX);
			rc = 1;
			break;
		case OP_FBHDA_ACCESS_END:
			FBHDA_access_end(state->Client_ECX);
			rc = 1;
			break;
		case OP_FBHDA_SWAP:
			{
				BOOL rs;
				rs = FBHDA_swap(state->Client_ECX);
				state->Client_ECX = (DWORD)rs;
				rc = 1;
				break;
			}
		case OP_FBHDA_CLEAN:
			FBHDA_clean();
			rc = 1;
			break;
		case OP_FBHDA_PALETTE_SET:
			FBHDA_palette_set(state->Client_ECX & 0xFF, state->Client_EDI);
			rc = 1;
			break;
		case OP_FBHDA_PALETTE_GET:
			{
				DWORD color;
				color = FBHDA_palette_get(state->Client_ECX & 0xFF);
				state->Client_EDI = color;
				rc = 1;
				break;
			}
#ifdef SVGA
		case OP_SVGA_VALID:
			{
				BOOL rs;
				rs = SVGA_valid();
				state->Client_ECX = (DWORD)rs;
				rc = 1;
				break;
			}
		case OP_SVGA_SETMODE:
			{
				BOOL rs;
				rs = SVGA_setmode(state->Client_ESI, state->Client_EDI, state->Client_ECX);
				state->Client_ECX = (DWORD)rs;
				rc = 1;
				break;
			}
		case OP_SVGA_VALIDMODE:
			{
				BOOL rs;
				rs = SVGA_validmode(state->Client_ESI, state->Client_EDI, state->Client_ECX);
				state->Client_ECX = (DWORD)rs;
				rc = 1;
				break;
			}
		case OP_SVGA_HW_ENABLE:
			SVGA_HW_enable();
			rc = 1;
			break;
		case OP_SVGA_HW_DISABLE:
			SVGA_HW_disable();
			rc = 1;
			break;
#endif
	}
	
	if(rc == 0xFFFF)
	{
		state->Client_EFlags |= 0x1; // set carry
	}
	else
	{
		state->Client_EFlags &= 0xFFFFFFFEUL; // clear carry
	}
	
	return rc;
}

/*
 * Service calls from protected mode (usually 16 bit) and virtual 86 mode
 * CPU state before call is already on stack (EBP points it).
 * Converts the call to __stdcall and call 'VMWS_API_Proc', result of this
 * function is placed to saved AX register.
 *
 */
void __declspec(naked) VMWS_API_entry()
{
	_asm {
		push ebp
		call VMWS_API_Proc
		mov [ebp+1Ch], ax
		retn
	}
}

/* generate all entry pro VDD function */
#define VDDFUNC(_fnname, _procname) void __declspec(naked) _procname ## _entry() { \
	_asm { push ebp }; \
	_asm { call _procname ## _proc }; \
	_asm { retn }; \
	}
#include "vxd_vdd_list.h"
#undef VDDFUNC

#define VDDFUNC(id, procname) DispatchTable[id] = (DWORD)(procname ## _entry);

/* init device and fill dispatch table */
void Device_Dynamic_Init_proc(DWORD VM)
{
	dbg_printf(dbg_Device_Init_proc);

	// VMMCall _Allocate_Device_CB_Area
 	ThisVM = VM;
 	
#ifdef SVGA
	SVGA_init_hw();
#endif
 		
	/* register miniVDD functions */
	VDD_Get_Mini_Dispatch_Table();
	if(DispatchTableLength >= 0x31)
	{
		#include "vxd_vdd_list.h"
	}
}
#undef VDDFUNC

/* process user space (PM32, RING-3) call */
DWORD __stdcall Device_IO_Control_proc(struct DIOCParams *params)
{
	DWORD *inBuf  = (DWORD*)params->lpInBuffer;
	DWORD *outBuf = (DWORD*)params->lpOutBuffer;
	
	//dbg_printf(dbg_deviceiocontrol, params->dwIoControlCode);
	
	switch(params->dwIoControlCode)
	{
		/* DX */
		case DIOC_OPEN:
		case DIOC_CLOSEHANDLE:
			dbg_printf(dbg_dic_system, params->dwIoControlCode);
			return 0;
		case OP_FBHDA_SETUP:
			outBuf[0] = (DWORD)FBHDA_setup();
			return 0;
		case OP_FBHDA_ACCESS_BEGIN:
			FBHDA_access_begin(inBuf[0]);
			return 0;
		case OP_FBHDA_ACCESS_END:
			FBHDA_access_end(inBuf[0]);
			return 0;
		case OP_FBHDA_SWAP:
			outBuf[0] = FBHDA_swap(inBuf[0]);
			return 0;
		case OP_FBHDA_CLEAN:
			FBHDA_clean();
			return 0;
		case OP_FBHDA_PALETTE_SET:
			FBHDA_palette_set(inBuf[0], inBuf[1]);
			return 0;
		case OP_FBHDA_PALETTE_GET:
			outBuf[0] = FBHDA_palette_get(inBuf[0]);
			return 0;
#ifdef SVGA
		case OP_SVGA_VALID:
			outBuf[0] = SVGA_valid();
			return 0;
		case OP_SVGA_CMB_ALLOC:
			outBuf[0] = (DWORD)SVGA_CMB_alloc();
			return 0;
		case OP_SVGA_CMB_FREE:
			SVGA_CMB_free((DWORD*)inBuf[0]);
			return 0;
		case OP_SVGA_CMB_SUBMIT:
			{
				SVGA_CMB_submit_io_t *inio  = (SVGA_CMB_submit_io_t*)inBuf;
				SVGA_CMB_status_t *status = (SVGA_CMB_status_t*)outBuf;
				
				SVGA_CMB_submit(inio->cmb, inio->cmb_size, status, inio->flags, inio->DXCtxId);
				return 0;
			}
		case OP_SVGA_FENCE_GET:
			outBuf[0] = SVGA_fence_get();
			return 0;
		case OP_SVGA_FENCE_QUERY:
			SVGA_fence_query(&outBuf[0], &outBuf[1]);
			return 0;
		case OP_SVGA_FENCE_WAIT:
			SVGA_fence_wait(inBuf[0]);
			return 0;
		case OP_SVGA_REGION_CREATE:
			{
				SVGA_region_info_t *inio  = (SVGA_region_info_t*)inBuf;
				SVGA_region_info_t *outio = (SVGA_region_info_t*)outBuf;
				if(outio != inio)
				{
					memcpy(outio, inio, sizeof(SVGA_region_info_t));
				}
				SVGA_region_create(outio);
				return 0;
			}
		case OP_SVGA_REGION_FREE:
			{
				SVGA_region_info_t *inio  = (SVGA_region_info_t*)inBuf;
				SVGA_region_free(inio);
				return 0;
			}
		case OP_SVGA_QUERY:
			outBuf[0] = SVGA_query(inBuf[0], inBuf[1]);
			return 0;
		case OP_SVGA_QUERY_VECTOR:
			SVGA_query_vector(inBuf[0], inBuf[1], inBuf[2], outBuf);
			return 0;
		case OP_SVGA_DB_SETUP:
			outBuf[0] = (DWORD)SVGA_DB_setup();
			return 0;
		case OP_SVGA_OT_SETUP:
			outBuf[0] = (DWORD)SVGA_OT_setup();
			return 0;
#endif /* SVGA */
	}
		
	dbg_printf(dbg_dic_unknown, params->dwIoControlCode);
	
	return 1;
}

void Device_Init_proc()
{
	/* nop */
}

/* shutdown procedure */
void Device_Exit_proc(DWORD VM)
{
	dbg_printf(dbg_destroy);
	
#ifdef SVGA
	SVGA_HW_disable();
#endif
	
	// TODO: free memory
}
