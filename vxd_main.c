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

/* 
 * Code for QEMU is inspired by boxvmini.asm by Philip Kelley
 */

#ifndef VXD32
#error VXD32 not defined!
#endif

#define VXD_MAIN

#ifndef SVGA
# ifndef VESA
#  define VBE
# endif
#endif

#include "winhack.h"
#include "vmm.h"
#include "vxd.h"
#include "vxd_lib.h"

#include "3d_accel.h"
#include "vxd_vdd.h"

#ifdef SVGA
# include "svga_all.h"
# include "vxd_svga.h"
#endif

#include "version.h"

#include "code32.h"

void VXD_control();
void VXD_API_entry();
void __stdcall Device_Init_proc(DWORD VM);
void __stdcall Device_Exit_proc(DWORD VM);
void __stdcall Device_Init_Complete(DWORD VM);
void __stdcall win32_destroy_process_proc(DWORD pid);

#define SERVICE_TABLE_CNT (FBHDA__OVERLAY_UNLOCK+1)
extern DWORD service_table[SERVICE_TABLE_CNT];

/*
  VXD structure
  this variable must be in first address in code segment.
  In other cases VXD isn't loadable (WLINK bug?)
*/
DDB VXD_DDB = {
	NULL,                       // must be NULL
	DDK_VERSION,                // DDK_Version
	VXD_DEVICE_ID,              // Device ID
	VXD_MAJOR_VER,              // Major Version
	VXD_MINOR_VER,              // Minor Version
	NULL,
	VXD_DEVICE_NAME,
	VDD_Init_Order,
	(DWORD)VXD_control,
	(DWORD)VXD_API_entry,
	(DWORD)VXD_API_entry,
	NULL,// CS:IP of API entry point
	NULL,// CS:IP of API entry point
	NULL,// Reference data from real mode
	(DWORD)&service_table,// Pointer to service table
	SERVICE_TABLE_CNT,// Number of services
	NULL, // DDB_Win32_Service_Table
	'Prev', //NULL, // prev
	sizeof(DDB),
	'Rsv1',
	'Rsv2',
	'Rsv3',
};

#include "vxd_strings.h"

DWORD *DispatchTable = 0;
DWORD DispatchTableLength = 0;
DWORD ThisVM = 0;

#ifdef QEMU
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
		cmp edi, ThisVM             ; Is the CRTC controlled by Windows?
    jne _Virtual1CEPhysical     ; If not, we should allow the I/O
    cmp ebx, edi                ; Is the calling VM Windows?
    je _Virtual1CEPhysical      ; If not, we should eat the I/O
    	ret
		_Virtual1CEPhysical:
	}
	VxDJmp(VDD, Do_Physical_IO);
}
#endif /* QEMU */

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

DWORD __stdcall Device_IO_Control_proc(DWORD vmhandle, struct DIOCParams *params);

/*
 * 32-bit DeviceIoControl ends here
 *
 */
void __declspec(naked) Device_IO_Control_entry()
{
	_asm {
		push esi /* struct DIOCParams */
		push ebx /* VMHandle */
		call Device_IO_Control_proc
		retn
	}
}

void __stdcall print_ctrl(DWORD type)
{
	dbg_printf("VXD_control: %d\n", type);
}

/*
 * service module calls (init, exit, deviceIoControl, ...)
 * clear carry if succes (this is always success)
 */
void __declspec(naked) VXD_control()
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
			push ebx ; VM handle
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
		cmp eax,Sys_Dynamic_Device_Init
		jnz control_4
			pushad
			push ebx ; VM handle
		  call Device_Init_proc
		  popad
			clc
			ret
		control_4:
		cmp eax,W32_DEVICEIOCONTROL
		jnz control_5
			jmp Device_IO_Control_entry
		control_5:
		cmp eax,System_Exit
		jnz control_6
			pushad
			push ebx ; VM handle
		  call Device_Exit_proc
			popad
			clc
			ret
/*
	https://www.codeproject.com/Articles/3874/The-Ultimate-Process-Thread-spy-for-Windows-9x
	
	"You will find the definition of CREATE_PROCESS, but wont find how to get the
	PID of a newly created process. For thread (CREATE_THREAD), the thread ID is
	passed in EDI register, but for CREATE_PROCESS it's not there. At last, by
	trial and error I found where it is, the PID value, it is in EDX reg."
*/
		control_6:
		cmp eax,DESTROY_PROCESS
		jnz control_7
			pushad
			push edx ; PID
			call win32_destroy_process_proc
			popad
			clc
			ret
		control_7:
;			pushad
;			push eax
;			call print_ctrl
;			popad
		clc
	  ret
	};
}

/* process V86/PM16 calls */
WORD __stdcall VXD_API_Proc(PCRS_32 state)
{
	WORD rc = 0xFFFF;
	WORD service = state->Client_EDX & 0xFFFF;
	
	//dbg_printf("VXD_API_Proc, service: %X\n", service);
	//Begin_Critical_Section(0);
	
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
		case OP_FBHDA_ACCESS_BEGIN:
			FBHDA_access_begin(state->Client_ECX);
			rc = 1;
			break;
		case OP_FBHDA_ACCESS_END:
			FBHDA_access_end(state->Client_ECX);
			rc = 1;
			break;
		case OP_FBHDA_ACCESS_RECT:
			FBHDA_access_rect(state->Client_EBX, state->Client_ECX, state->Client_ESI, state->Client_EDI);
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
		case OP_FBHDA_GAMMA_GET:
			state->Client_ECX = FBHDA_gamma_get((void *)state->Client_EDI, state->Client_ECX);
			rc = 1;
			break;
		case OP_FBHDA_GAMMA_SET:
			state->Client_ECX = FBHDA_gamma_set((void *)state->Client_ESI, state->Client_ECX);
			rc = 1;
			break;
		/* mouse */
		case OP_MOUSE_LOAD:
			state->Client_ECX = mouse_load();
			rc = 1;
			break;
		case OP_MOUSE_BUFFER:
			state->Client_ECX = (DWORD)mouse_buffer();
			rc = 1;
			break;
		case OP_MOUSE_MOVE:
			mouse_move(state->Client_ESI, state->Client_EDI);
			rc = 1;
			break;
		case OP_MOUSE_SHOW:
			mouse_show();
			rc = 1;
			break;
		case OP_MOUSE_HIDE:
			mouse_hide();
			rc = 1;
			break;
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
			Begin_Critical_Section(0);
			rs = SVGA_setmode(state->Client_ESI, state->Client_EDI, state->Client_ECX);
			End_Critical_Section();
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

#ifdef VBE
		case OP_VBE_VALID:
		{
			BOOL rs;
			rs = VBE_valid();
			state->Client_ECX = (DWORD)rs;
			rc = 1;
			break;
		}
		case OP_VBE_SETMODE:
		{
			BOOL rs;
			Begin_Critical_Section(0);
			rs = VBE_setmode(state->Client_ESI, state->Client_EDI, state->Client_ECX);
			End_Critical_Section();
			state->Client_ECX = (DWORD)rs;
			rc = 1;
			break;
		}
		case OP_VBE_VALIDMODE:
		{
			BOOL rs;
			rs = VBE_validmode(state->Client_ESI, state->Client_EDI, state->Client_ECX);
			state->Client_ECX = (DWORD)rs;
			rc = 1;
			break;
		}
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
	
	//dbg_printf("VXD_API_Proc end\n");
	
	//End_Critical_Section();
	
	return rc;
}

/*
 * Service calls from protected mode (usually 16 bit) and virtual 86 mode
 * CPU state before call is already on stack (EBP points it).
 * Converts the call to __stdcall and call 'VXD_API_Proc', result of this
 * function is placed to saved AX register.
 *
 */
void __declspec(naked) VXD_API_entry()
{
	_asm {
		push ebp
		mov eax,cr3 ; TLB bug here? Looks like...
		mov cr3,eax
		call VXD_API_Proc
		mov [ebp+1Ch], ax
		retn
	}
}

const char reg_path[] = "Software\\vmdisp9x";
extern DWORD gamma_quirk;

static void configure_FBHDA()
{
	FBHDA_t *fbhda;
	DWORD force_sw = 0;
	DWORD force_qemu3dfx = 0;
	int i;
	char buf[32];
	
	fbhda = FBHDA_setup();
	
	if(fbhda)
	{
		DWORD size_mb;
		
		RegReadConf(HKEY_LOCAL_MACHINE, reg_path, "FORCE_SOFTWARE", &force_sw);
		RegReadConf(HKEY_LOCAL_MACHINE, reg_path, "FORCE_QEMU3DFX", &force_qemu3dfx);
		RegReadConf(HKEY_LOCAL_MACHINE, reg_path, "GAMMA_QUIRK",    &gamma_quirk);

		for(i = 1; i < FBHA_OVERLAYS_MAX; i++)
		{
			char *ptr;
			strcpy(buf, "OVERLAY_");
			
			ptr = buf + strlen(buf);
			if(i >= 10)
			{
				*ptr = '0' + (i/10);
				ptr++;
			}
			*ptr = '0' + (i % 10);
			ptr++;
			*ptr = '\0';
			strcat(buf, "_SIZE");
			
			size_mb = 0;
			RegReadConf(HKEY_LOCAL_MACHINE, reg_path, buf, &size_mb);
			fbhda->overlays[i].size = size_mb*(1024*1024);

			if(fbhda->overlays[i].size > 0)
			{
				fbhda->overlays_size += fbhda->overlays[i].size;
				fbhda->overlays[i].ptr = ((BYTE*)fbhda->vram_pm32) + fbhda->vram_size - fbhda->overlays_size;
			}
			else
			{
				fbhda->overlays[i].ptr = 0;
			}
			
			dbg_printf("Overlay #%ld: %ld\n", i, fbhda->overlays[i].size);			
		}
		
		if(force_sw)
		{
			fbhda->flags |= FB_FORCE_SOFTWARE;
		}
		
		if(force_qemu3dfx)
		{
			fbhda->flags |= FB_ACCEL_QEMU3DFX;
		}
	}
}

/* generate all entry pro VDD function */
#define VDDFUNC(_fnname, _procname) void __declspec(naked) _procname ## _entry() { \
	_asm { push ebp }; \
	_asm { call _procname ## _proc }; \
	_asm { retn }; \
	}

#define VDDNAKED(_fnname, _procname) void __declspec(naked) _procname ## _entry() { \
	_asm { pushad }; \
	_asm { mov eax, esp }; \
	_asm { add eax, 32 }; \
	_asm { push eax }; \
	_asm { call _procname ## _proc }; \
	_asm { popad }; \
	_asm { retn }; \
	}

#include "vxd_vdd_list.h"
#undef VDDFUNC
#undef VDDNAKED

#define VDDFUNC(id, procname) DispatchTable[id] = (DWORD)(procname ## _entry);
#define VDDNAKED VDDFUNC

/* init device and fill dispatch table */
void Device_Init_proc(DWORD VM)
{
	Begin_Critical_Section(0);
	dbg_printf(dbg_Device_Init_proc);

	VMMCall(_Allocate_Device_CB_Area);
 	ThisVM = VM;
 	
#ifdef SVGA
	SVGA_init_hw();
#endif

#ifdef VBE
	VBE_init_hw();
#endif

#ifdef QEMU
	Install_IO_Handler(0x1ce, (DWORD)virtual_0x1ce);
	Disable_Global_Trapping(0x1ce);
	Install_IO_Handler(0x1cf, (DWORD)virtual_0x1ce);
	Disable_Global_Trapping(0x1cf);
#endif
 	
	/* register miniVDD functions */
	VDD_Get_Mini_Dispatch_Table();
	if(DispatchTableLength >= 0x31)
	{
		#include "vxd_vdd_list.h"
	}
	
	configure_FBHDA();
	End_Critical_Section();
}
#undef VDDFUNC
#undef VDDNAKED

/* process user space (PM32, RING-3) call */
DWORD __stdcall Device_IO_Control_proc(DWORD vmhandle, struct DIOCParams *params)
{
	DWORD *inBuf  = (DWORD*)params->lpInBuffer;
	DWORD *outBuf = (DWORD*)params->lpOutBuffer;
	DWORD rc = 1;
	
	//Begin_Critical_Section(0);
	
	//dbg_printf("I%x\n", params->dwIoControlCode);
	
	switch(params->dwIoControlCode)
	{
		/* DX */
		case DIOC_OPEN:
		case DIOC_CLOSEHANDLE:
			dbg_printf(dbg_dic_system, params->dwIoControlCode);
			rc = 0;
			break;
		case OP_FBHDA_SETUP:
			outBuf[0] = (DWORD)FBHDA_setup();
			rc = 0;
			break;
		case OP_FBHDA_ACCESS_BEGIN:
			FBHDA_access_begin(inBuf[0]);
			rc = 0;
			break;
		case OP_FBHDA_ACCESS_END:
			FBHDA_access_end(inBuf[0]);
			rc = 0;
			break;
		case OP_FBHDA_ACCESS_RECT:
			FBHDA_access_rect(inBuf[0], inBuf[1], inBuf[2], inBuf[3]);
			rc = 0;
			break;
		case OP_FBHDA_SWAP:
			outBuf[0] = FBHDA_swap(inBuf[0]);
			rc = 0;
			break;
		case OP_FBHDA_CLEAN:
			FBHDA_clean();
			rc = 0;
			break;
		case OP_FBHDA_PALETTE_SET:
			FBHDA_palette_set(inBuf[0], inBuf[1]);
			rc = 0;
			break;
		case OP_FBHDA_PALETTE_GET:
			outBuf[0] = FBHDA_palette_get(inBuf[0]);
			rc = 0;
			break;
		case OP_FBHDA_OVERLAY_SETUP:
			outBuf[0] = FBHDA_overlay_setup(inBuf[0], inBuf[1], inBuf[2], inBuf[3]);
			rc = 0;
			break;
		case OP_FBHDA_OVERLAY_LOCK:
			FBHDA_overlay_lock(inBuf[0], inBuf[1], inBuf[2], inBuf[3]);
			rc = 0;
			break;
		case OP_FBHDA_OVERLAY_UNLOCK:
			FBHDA_overlay_unlock(inBuf[0]);
			rc = 0;
			break;
#ifdef SVGA
		case OP_SVGA_VALID:
			outBuf[0] = SVGA_valid();
			rc = 0;
			break;
		case OP_SVGA_CMB_ALLOC:
			outBuf[0] = (DWORD)SVGA_CMB_alloc();
			rc = 0;
			break;
		case OP_SVGA_CMB_FREE:
			SVGA_CMB_free((DWORD*)inBuf[0]);
			rc = 0;
			break;
		case OP_SVGA_CMB_SUBMIT:
		{
				SVGA_CMB_submit_io_t *inio  = (SVGA_CMB_submit_io_t*)inBuf;
				SVGA_CMB_status_t *status = (SVGA_CMB_status_t*)outBuf;
				
				SVGA_CMB_submit(inio->cmb, inio->cmb_size, status, inio->flags, inio->DXCtxId);
				rc = 0;
				break;
		}
		case OP_SVGA_FENCE_GET:
			outBuf[0] = SVGA_fence_get();
			rc = 0;
			break;
		case OP_SVGA_FENCE_QUERY:
			SVGA_fence_query(&outBuf[0], &outBuf[1]);
			rc = 0;
			break;
		case OP_SVGA_FENCE_WAIT:
			SVGA_fence_wait(inBuf[0]);
			rc = 0;
			break;
		case OP_SVGA_REGION_CREATE:
		{
			SVGA_region_info_t *inio  = (SVGA_region_info_t*)inBuf;
			SVGA_region_info_t *outio = (SVGA_region_info_t*)outBuf;
			if(outio != inio)
			{
				memcpy(outio, inio, sizeof(SVGA_region_info_t));
			}
			outio->address = NULL;
			if(!SVGA_region_create(outio))
			{
				SVGA_flushcache();
				SVGA_region_create(outio);
			}
			rc = 0;
			break;
		}
		case OP_SVGA_REGION_FREE:
			{
				SVGA_region_info_t *inio  = (SVGA_region_info_t*)inBuf;
				SVGA_region_free(inio);
				rc = 0;
				break;
			}
		case OP_SVGA_QUERY:
			outBuf[0] = SVGA_query(inBuf[0], inBuf[1]);
			rc = 0;
			break;
		case OP_SVGA_QUERY_VECTOR:
			SVGA_query_vector(inBuf[0], inBuf[1], inBuf[2], outBuf);
			rc = 0;
			break;
		case OP_SVGA_DB_SETUP:
			outBuf[0] = (DWORD)SVGA_DB_setup();
			rc = 0;
			break;
		case OP_SVGA_OT_SETUP:
			outBuf[0] = (DWORD)SVGA_OT_setup();
			rc = 0;
			break;
		case OP_SVGA_FLUSHCACHE:
			SVGA_flushcache();
			rc = 0;
			break;
		case OP_SVGA_VXDCMD:
			outBuf[0] = (DWORD)SVGA_vxdcmd(inBuf[0]);
			rc = 0;
			break;
#ifdef DBGPRINT
		/* export some mouse function for debuging */
		case OP_MOUSE_MOVE:
			mouse_move(inBuf[0], inBuf[1]);
			rc = 0;
			break;
		case OP_MOUSE_SHOW:
			mouse_show();
			rc = 0;
			break;
		case OP_MOUSE_HIDE:
			mouse_hide();
			rc = 0;
			break;
#endif /*DBGPRINT */
#endif /* SVGA */
		default:
			dbg_printf("DeviceIOControl: Unknown: %d\n", params->dwIoControlCode);
			break;
	}
	
	//dbg_printf("IL\n");
	//End_Critical_Section();

	return rc;
}

void Device_Init_Complete(DWORD VM)
{
#ifdef QEMU
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
	_PhysIntoV86(0xA0, VM, 0xA0, 16, 0);
	_PhysIntoV86(0xB8, VM, 0xB8, 8, 0);
#endif
}

/* shutdown procedure */
void Device_Exit_proc(DWORD VM)
{
#ifdef SVGA
	SVGA_HW_disable();
#endif

	dbg_printf(dbg_destroy);
	
	// TODO: free memory
}

static DWORD __stdcall service_get_ver()
{
	return API_3DACCEL_VER;
}

static FBHDA_t *__stdcall service_setup()
{
	return FBHDA_setup();
}

static DWORD __stdcall service_overlay_setup(DWORD overlay, DWORD width, DWORD height, DWORD bpp)
{
	return FBHDA_overlay_setup(overlay, width, height, bpp);
}

static void __stdcall service_overlay_lock(DWORD left, DWORD top, DWORD right, DWORD bottom)
{
	FBHDA_overlay_lock(left, top, right, bottom);
}

static void __stdcall service_overlay_unlock(DWORD flags)
{
	FBHDA_overlay_unlock(flags);
}

DWORD service_table[SERVICE_TABLE_CNT] = {
	(DWORD)&service_get_ver, // 0
	(DWORD)&service_setup, // 1
	0, // 2
	0, // 3
	0, // 4
	0, // 5
	0, // 6
	0, // 7
	0, // 8
	(DWORD)&service_overlay_setup, // 9
	(DWORD)&service_overlay_lock, // 10
	(DWORD)&service_overlay_unlock, // 11
};

void __stdcall win32_destroy_process_proc(DWORD pid)
{
	dbg_printf("DESTROY_PROCESS: %lX\n", pid);
	
#ifdef SVGA
	SVGA_ProcessCleanup(pid);
#endif
}
