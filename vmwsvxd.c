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

#include "winhack.h"
#include "vmm.h"
#include "vmwsvxd.h"

#include "svga_all.h"

#include "minivdd32.h"

#include "version.h"

#if 0
/* dynamic VXDs don't using this init function in discardable segment  */

#pragma data_seg("_ITEXT", "ICODE")
#pragma code_seg("_ITEXT", "ICODE")

BOOL __cdecl VMWS_Sys_Critical_Init(DWORD hVM, DWORD dwRefData,
                                    PSTR pCmdTail, PCRS_32 pCRS)
{
  return TRUE;
}

BOOL __cdecl VMWS_Device_Init(DWORD hVM, PSTR pCmdTail, PCRS_32 pCRS)
{
  return TRUE;
}

#endif

#include "code32.h"

void VMWS_Control();
void VMWS_API_Entry();

/*
  VXD structure
  this variable must be in first address in code segment.
  In other cases VXD isn't loadable (WLINK bug?)
*/
DDB VMWS_DDB = {
	NULL,                       // must be NULL
	DDK_VERSION,                // DDK_Version
	VMWSVXD_DEVICE_ID,         // Device ID
	VMWSVXD_MAJOR_VER,         // Major Version
	VMWSVXD_MINOR_VER,         // Minor Version
	NULL,
	"VMWSVXD",
	VDD_Init_Order, //Undefined_Init_Order,
	(DWORD)VMWS_Control,
	(DWORD)VMWS_API_Entry,
	(DWORD)VMWS_API_Entry,
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
char dbg_region_err[] = "region create error\n";

char dbg_Device_Init_proc[] = "Device_Init_proc\n";
char dbg_Device_Init_proc_succ[] = "Device_Init_proc success\n";
char dbg_dic_ring[] = "DeviceIOControl: Ring\n";
char dbg_dic_sync[] = "DeviceIOControl: Sync\n";
char dbg_dic_unknown[] = "DeviceIOControl: Unknown: %d\n";
char dbg_dic_system[] = "DeviceIOControl: System code: %d\n";
char dbg_get_ppa[] = "%lx -> %lx\n";
char dbg_get_ppa_beg[] = "Virtual: %lx\n";

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
	static WORD ver;
	
	_asm push ecx
	_asm push eax
	_asm xor eax, eax
	VMMCall(Get_VMM_Version);
	_asm mov [ver],ax
	_asm pop ecx
	_asm pop eax
	
	return ver;
}
 
ULONG __declspec(naked) __cdecl _PageAllocate(ULONG nPages, ULONG pType, ULONG VM, ULONG AlignMask, ULONG minPhys, ULONG maxPhys, ULONG *PhysAddr, ULONG flags)
{
	VMMJmp(_PageAllocate);
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
 *
 */
#define SVGA_DBG             0x110B
#define SVGA_SYNC            0x1116
#define SVGA_RING            0x1117

DWORD __stdcall Device_IO_Control_entry(struct DIOCParams *params)
{
	switch(params->dwIoControlCode)
	{
		case DIOC_OPEN:
		case DIOC_CLOSEHANDLE:
			dbg_printf(dbg_dic_system, params->dwIoControlCode);
			return 0;
		case SVGA_SYNC:
			//dbg_printf(dbg_dic_sync);
			SVGA_Flush();
			return 0;
		case SVGA_RING:
			//dbg_printf(dbg_dic_ring);
			SVGA_WriteReg(SVGA_REG_SYNC, 1);
			return 0;
		case SVGA_DBG:
#ifdef DBGPRINT
			dbg_printf(dbg_str, params->lpInBuffer);
#endif			
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

/*
 * service module calls (init, exit, deviceIoControl, ...)
 * clear carry if succes (this is always success)
 */
void __declspec(naked) VMWS_Control()
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
		cmp eax,0x1B
		jnz control_3
			pushad
		  call Device_Init_proc
		  popad
			clc
			ret
		control_3:
		cmp eax,W32_DEVICEIOCONTROL
		jnz control_4
			jmp Device_IO_Control_proc
		control_4:
		clc
	  ret
	};
}

/**
 * Return PPN (physical page number) from virtual address
 * 
 **/
static DWORD getPPN(DWORD virtualaddr)
{
	DWORD phy = 0;
	_CopyPageTable(virtualaddr/P_SIZE, 1, &phy, 0);
	return phy/P_SIZE;
}

/**
 * Allocate guest memory region (GMR) - HW needs know memory physical
 * addressed of pages in (virtual) memory block.
 * Technically this allocate 2 memory block, 1st for data and 2nd as its
 * physical description.
 *
 * @param nPages: number of pages to allocate
 * @param outDataAddr: linear address (system space) of data block
 * @param outGMRAddr: linear address of GMR block
 * @param outPPN: physical page number of outGMRAddr (first page)
 *
 * @return: TRUE on success
 *
 **/
static BOOL GMRAlloc(ULONG nPages, ULONG *outDataAddr, ULONG *outGMRAddr, ULONG *outPPN)
{
	ULONG phy;
	ULONG pgblk_phy;
	ULONG laddr;
	ULONG pgblk;
	SVGAGuestMemDescriptor *desc;
	laddr = _PageAllocate(nPages, PG_SYS, 0, 0, 0x0, 0x100000, &phy, PAGECONTIG | PAGEUSEALIGN | PAGEFIXED);
	
	if(laddr)
	{
		pgblk = _PageAllocate(1, PG_SYS, 0, 0, 0x0, 0x100000, &pgblk_phy, PAGECONTIG | PAGEUSEALIGN | PAGEFIXED);
		if(pgblk)
		{
			desc = (SVGAGuestMemDescriptor*)pgblk;
			desc->ppn = (phy/P_SIZE);
			desc->numPages = nPages;
			desc++;
			desc->ppn = 0;
			desc->numPages = 0;
		
			*outDataAddr = laddr;
			*outGMRAddr = pgblk;
			*outPPN = (pgblk_phy/P_SIZE);
		
			return TRUE;
		}
		else
		{
			_PageFree((PVOID)laddr, 0);
			return FALSE;
		}
	}
	else
	{
		ULONG taddr;
		ULONG tppn;
		ULONG pgi;
		ULONG base_ppn;
		ULONG base_cnt;
		ULONG blocks = 1;
		ULONG blk_pages = 0;
		
		laddr = _PageAllocate(nPages, PG_SYS, 0, 0, 0x0, 0x100000, NULL, PAGEFIXED);
		
		if(laddr)
		{
			/* determine how many physical continuous blocks we have */
			base_ppn = getPPN(laddr);
			base_cnt = 1;
			for(pgi = 1; pgi < nPages; pgi++)
			{
				taddr = laddr + pgi*P_SIZE;
				tppn = getPPN(taddr);
				
				if(tppn != base_ppn + base_cnt)
				{
					base_ppn = tppn;
					base_cnt = 1;
					blocks++;
				}
				else
				{
					base_cnt++;
				}
			}
			
			// number of pages to store regions information
			blk_pages = ((blocks+1)/(P_SIZE/sizeof(SVGAGuestMemDescriptor))) + 1;
			
			// add extra descriptors for pages edge
			blk_pages = ((blocks+blk_pages+1)/(P_SIZE/sizeof(SVGAGuestMemDescriptor))) + 1;
			
			pgblk = _PageAllocate(blk_pages, PG_SYS, 0, 0, 0x0, 0x100000, &pgblk_phy, PAGECONTIG | PAGEUSEALIGN | PAGEFIXED);
			if(pgblk)
			{
				desc = (SVGAGuestMemDescriptor*)pgblk;
				desc->ppn = getPPN(laddr);
				desc->numPages = 1;
				blocks = 1;
				
				for(pgi = 1; pgi < nPages; pgi++)
				{
					taddr = laddr + pgi*P_SIZE;
					tppn = getPPN(taddr);
					
					if(tppn == desc->ppn + desc->numPages)
					{
						desc->numPages++;
					}
					else
					{
						/* next descriptor is on page edge */
						if((blocks+1) % (P_SIZE/sizeof(SVGAGuestMemDescriptor)) == 0)
						{
							desc++;
							desc->numPages = 0;
							desc->ppn = getPPN((DWORD)(desc+1));
							//dbg_printf(old_new_ppn, blocks, getPPN((DWORD)(desc)), getPPN((DWORD)(desc+1)), getPPN((DWORD)(desc+2)));
							blocks++;
						}
						
						desc++;
						desc->ppn = tppn;
						desc->numPages = 1;
						blocks++;
					}
				}
				
				desc++;
				desc->ppn = 0;
				desc->numPages = 0;
				
				*outDataAddr = laddr;
				*outGMRAddr = pgblk;
				*outPPN = (pgblk_phy/P_SIZE);
				
				return TRUE;
			}
			else
			{
				_PageFree((PVOID)laddr, 0);
				return FALSE;
			}
		}
	}
	
	return FALSE;
}

/**
 * Free data allocated by GMRAlloc
 *
 **/
static BOOL GMRFree(ULONG DataAddr, ULONG GMRAddr)
{
	BOOL ret = FALSE;

	if(_PageFree((PVOID)DataAddr, 0) != 0)
	{
		ret = TRUE;
	}
	
	if(_PageFree((PVOID)GMRAddr, 0) != 0)
	{
		ret = TRUE;
	}
	
	return ret;
}

/**
 * PM16 driver RING0 calls
 **/
BOOL CreateRegion(unsigned int nPages, ULONG *lpLAddr, ULONG *lpPPN, ULONG *lpPGBLK)
{	
	return GMRAlloc(nPages, lpLAddr, lpPGBLK, lpPPN);
}

BOOL FreeRegion(ULONG LAddr, ULONG PGBLK)
{
	return GMRFree(LAddr, PGBLK);
}

#define MOB_PER_PAGE_CNT (P_SIZE/sizeof(ULONG))

BOOL CreateMOB(unsigned int nPages, ULONG *lpLAddr, ULONG *lpPPN)
{
	unsigned int mobBasePages = (nPages + MOB_PER_PAGE_CNT - 1)/MOB_PER_PAGE_CNT;
	ULONG phy;
	ULONG laddr;
	
	laddr = _PageAllocate(nPages + mobBasePages, PG_SYS, 0, 0x0, 0, 0x0fffff, &phy, PAGECONTIG | PAGEUSEALIGN | PAGEFIXED);
	if(laddr)
	{
		unsigned int i;
		ULONG ppu = (phy/P_SIZE) + mobBasePages;
		ULONG *ptr = (ULONG*)laddr;
		
		for(i = 0; i < nPages; i++)
		{
			ptr[i] = ppu;
			ppu++;
		}

		*lpLAddr = laddr;
		*lpPPN = (phy/P_SIZE);
		return TRUE;
	}
	
	return FALSE;
}

BOOL FreeMOB(ULONG LAddr)
{
	if(_PageFree((PVOID)LAddr, 0) != 0)
	{
		return TRUE;
	}
	
	return FALSE;	
}

WORD __stdcall VMWS_API_Proc(PCRS_32 state)
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
		case VMWSVXD_PM16_CREATE_REGION:
		{
			ULONG lAddr;
			ULONG PPN;
			ULONG PGBLK;
				
			if(CreateRegion(state->Client_ECX, &lAddr, &PPN, &PGBLK))
			{
				state->Client_ECX = PPN;
				state->Client_EDX = lAddr;
				state->Client_EBX = PGBLK;
				rc = 1;
			}
			else
			{
				state->Client_ECX = 0;
				state->Client_EDX = 0;
				state->Client_EBX = 0;
				
				dbg_printf(dbg_region_err);
				
				rc = 0;
			}
			break;
		}
		/* free region = input: ECX: lin. address */
		case VMWSVXD_PM16_DESTROY_REGION: 
		{
			if(FreeRegion(state->Client_ECX, state->Client_EBX))
			{
				rc = 1;
			}
			else
			{
				rc = 0;
			}
			break;			
		}
		/* Create MOB = input: ECX - num_pages; output: EDX - lin. address of descriptor, ECX - PPN of descriptor */
		case VMWSVXD_PM16_CREATE_MOB:
		{
			ULONG lAddr;
			ULONG PPN;
				
			if(CreateMOB(state->Client_ECX, &lAddr, &PPN))
			{
				state->Client_ECX = PPN;
				state->Client_EDX = lAddr;
				rc = 1;
			}
			else
			{
				state->Client_ECX = 0;
				state->Client_EDX = 0;
				
				dbg_printf(dbg_region_err);
				
				rc = 0;
			}
			break;
		}
		/* Free MOB = input: ECX: lin. address */
		case VMWSVXD_PM16_DESTROY_MOB:
		{
			if(FreeMOB(state->Client_ECX))
			{
				rc = 1;
			}
			else
			{
				rc = 0;
			}
			break;
		}
		/* clear memory on linear address (ESI) by defined size (ECX) */
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
		/* set ECX to actual api version */
		case VMWSVXD_PM16_APIVER:
			state->Client_ECX = DRV_API_LEVEL;
			rc = 1;
			break;
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
void __declspec(naked) VMWS_API_Entry()
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
#include "minivdd_func.h"
#undef VDDFUNC

#define VDDFUNC(id, procname) DispatchTable[id] = (DWORD)(procname ## _entry);

/* init device and fill dispatch table */
void Device_Init_proc()
{
	dbg_printf(dbg_Device_Init_proc);
	// VMMCall _Allocate_Device_CB_Area
	
	if(SVGA_Init(FALSE) == 0)
	{
		dbg_printf(dbg_Device_Init_proc_succ);
		svga_init_success = TRUE;
			
		VDD_Get_Mini_Dispatch_Table();
		if(DispatchTableLength >= 0x31)
		{
			#include "minivdd_func.h"
		}
	}
}
#undef VDDFUNC
