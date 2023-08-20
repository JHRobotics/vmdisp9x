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

/* 
 * This file is generally C port of boxvmini.asm by Philip Kelley
 */

#define VXD32
#define QEMU

#include "winhack.h"
#include "vmm.h"
#include "vmwsvxd.h"

#include "svga_all.h"

#include "minivdd32.h"

#include "version.h"

#include "code32.h"

void QEMU_Control();
void QEMU_API_Entry();

/*
  VXD structure
  this variable must be in first address in code segment.
  In other cases VXD isn't loadable (WLINK bug?)
*/
DDB QEMU_DDB = {
	NULL,                       // must be NULL
	DDK_VERSION,                // DDK_Version
	UNDEFINED_DEVICE_ID,        // Device ID
	VMWSVXD_MAJOR_VER,          // Major Version
	VMWSVXD_MINOR_VER,          // Minor Version
	NULL,
	"QEMU_VXD",
	VDD_Init_Order, //Undefined_Init_Order,
	(DWORD)QEMU_Control,
	(DWORD)QEMU_API_Entry,
	(DWORD)QEMU_API_Entry,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL, // DDB_Win32_Service_Table
	NULL, // prev
	sizeof(DDB)
};

/* string tables */
#ifdef DBGPRINT
char dbg_hello[] = "Hello world!\n";
char dbg_version[] = "VMM version: ";

char dbg_Device_Init_proc[] = "Device_Init_proc\n";
char dbg_Device_Init_proc_succ[] = "Device_Init_proc success\n";
char dbg_dic_unknown[] = "DeviceIOControl: Unknown: %d\n";
char dbg_dic_system[] = "DeviceIOControl: System code: %d\n";

char dbg_str[] = "%s\n";

#endif

DWORD *DispatchTable = 0;
DWORD DispatchTableLength = 0;
Bool svga_init_success = FALSE;

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

void Install_IO_Handler(DWORD port, DWORD callback)
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

ULONG __declspec(naked) __cdecl _PhysIntoV86(ULONG PhysPage, ULONG VM, ULONG VMLinPgNum, ULONG nPages, ULONG flags)
{
	VMMJmp(_PhysIntoV86);
}

ULONG __declspec(naked) __cdecl _PageAllocate(ULONG nPages, ULONG pType, ULONG VM, ULONG AlignMask, ULONG minPhys, ULONG maxPhys, ULONG *PhysAddr, ULONG flags)
{
	VMMJmp(_PageAllocate);
}

ULONG __declspec(naked) __cdecl _PageFree(PVOID hMem, DWORD flags)
{
	VMMJmp(_PageFree);
}

/* from minivdd.c */
void Enable_Global_Trapping(DWORD port);
void Disable_Global_Trapping(DWORD port);

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

/*
 * 32-bit DeviceIoControl ends here
 * (not much to do now)
 *
 */
DWORD __stdcall Device_IO_Control_entry(struct DIOCParams *params)
{
	switch(params->dwIoControlCode)
	{
		case DIOC_OPEN:
		case DIOC_CLOSEHANDLE:
			dbg_printf(dbg_dic_system, params->dwIoControlCode);		
			return 0;
	}
	dbg_printf(dbg_dic_unknown, params->dwIoControlCode);
	
	return 1;
}

void __declspec(naked) Device_IO_Control_proc()
{
	_asm {
		push esi /* struct DIOCParams */
		call Device_IO_Control_entry
		retn
	}
}

void Device_Init_proc();
void __stdcall Device_Dynamic_Init_proc(DWORD WinVMHandle);
void __stdcall Device_Init_Complete(DWORD VM);

/*
 * service module calls (init, exit, deviceIoControl, ...)
 * clear carry if succes (this is always success)
 */
void __declspec(naked) QEMU_Control()
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
			pushad
		  call Device_Init_proc
			popad
			clc
			ret
		control_2:
		cmp eax,Init_Complete
		jnz control_3
			pushad
			push ebx ; VM handle
		  call Device_Init_Complete
			popad
			clc
			ret
		control_3:
		cmp eax,0x1B ; dynamic init
		jnz control_4
			pushad
			push ebx
		  call Device_Dynamic_Init_proc
		  popad
			clc
			ret
		control_4:
		cmp eax,W32_DEVICEIOCONTROL
		jnz control_5
			jmp Device_IO_Control_proc
		control_5:
		clc
	  ret
	};
}

WORD __stdcall QEMU_API_Proc(PCRS_32 state)
{
	WORD rc = 0xFFFF;
	WORD service = state->Client_EDX & 0xFFFF;
	switch(service)
	{
		case VMWSVXD_PM16_VERSION:
			rc = (VMWSVXD_MAJOR_VER << 8) | VMWSVXD_MINOR_VER;
			break;
		case VMWSVXD_PM16_VMM_VERSION:
			rc = Get_VMM_Version();
			break;
		/* create region = input: ECX - num_pages; output: EDX - lin. address of descriptor, ECX - PPN of descriptor */
		case VMWSVXD_PM16_ZEROMEM:
		{
			ULONG i = 0;
			unsigned char *ptr = (unsigned char *)state->Client_ESI;
			for(i = 0; i < state->Client_ECX; i++)
			{
				ptr[i] = 0;
			}
			rc = 1;
			break;
		}
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
void __declspec(naked) QEMU_API_Entry()
{
	_asm {
		push ebp
		call QEMU_API_Proc
		mov [ebp+1Ch], ax
		retn
	}
}

static DWORD dwWindowsVMHandle = NULL;

/**
 * This is fix of broken screen when open DOS window
 *
 **/
void __declspec(naked) virtual_0x1ce()
{
/*
 * AX/AL contains the value to be read or written on the port.
 * EBX contains the handle of the VM accessing the port.
 * ECX contains the direction (in/out) and size (byte/word) of the operation.
 * EDX contains the port number, which for us will either be 1CEh or 1CFh.
 */
	VxDCall(VDD, Get_VM_Info); // esi = result
	_asm{
		cmp edi, dwWindowsVMHandle  ; Is the CRTC controlled by Windows?
    jne _Virtual1CEPhysical     ; If not, we should allow the I/O
    cmp ebx, edi                ; Is the calling VM Windows?
    je _Virtual1CEPhysical      ; If not, we should eat the I/O
    	ret
		_Virtual1CEPhysical:
	}
	VxDJmp(VDD, Do_Physical_IO);
}

/* generate all entry pro VDD function */
#define VDDFUNC(_fnname, _procname) void __declspec(naked) _procname ## _entry() { \
	_asm { push ebp }; \
	_asm { call _procname ## _proc }; \
	_asm { retn }; \
	}
#include "minivdd_func.h"
#undef VDDFUNC

#define VDDFUNC(id, procname) DispatchTable[id] = (DWORD)(procname ## _entry);

/* init device and fill dispatch table */
void Device_Dynamic_Init_proc(DWORD WinVMHandle)
{
	dbg_printf(dbg_Device_Init_proc);
	//VMMCall(_Allocate_Device_CB_Area);

	VDD_Get_Mini_Dispatch_Table();
	if(DispatchTableLength >= 0x31)
	{
		#include "minivdd_func.h"
	}
	
	dwWindowsVMHandle = WinVMHandle;
	
	Install_IO_Handler(0x1ce, (DWORD)virtual_0x1ce);
	Disable_Global_Trapping(0x1ce);
	Install_IO_Handler(0x1cf, (DWORD)virtual_0x1ce);
	Disable_Global_Trapping(0x1cf);
	
	dbg_printf(dbg_Device_Init_proc_succ);
}
#undef VDDFUNC

/*
 * At Windows shutdown time, the display driver calls VDD_DRIVER_UNREGISTER,
 * which calls our DisplayDriverDisabling callback, and soon thereafter
 * does an INT 10h (AH=0) to mode 3 (and then 13) in V86 mode. However, the
 * memory ranges for display memory are not always mapped!
 *
 * If the VGA ROM BIOS, during execution of such a set video mode call, tries to
 * clear the screen, and the appropriate memory range isn't mapped, then we end
 * up in a page fault handler, which I guess maps the memory and then resumes
 * execution in V86 mode. But strangely, in QEMU, this fault & resume mechanism
 * does not work with KVM or WHPX assist, at least on Intel. The machine hangs
 * instead (I have not root caused why).
 *
 * We can dodge this problem by setting up real mappings up front.
 *
 * At entry, EBX contains a Windows VM handle.
 *
 */
void Device_Init_Complete(DWORD VM)
{
	_PhysIntoV86(0xA0, VM, 0xA0, 16, 0);
	_PhysIntoV86(0xB8, VM, 0xB8, 8, 0);
}

void Device_Init_proc()
{
	/* NOP */
}
