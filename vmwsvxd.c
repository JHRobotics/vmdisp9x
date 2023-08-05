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

#define SVGA

#include "winhack.h"
#include "vmm.h"
#include "vmwsvxd.h"

#include "svga_all.h"

#include "minivdd32.h"

#include "version.h"

#ifndef PAGE_SIZE
#define PAGE_SIZE 4096
#endif

#define PTONPAGE (PAGE_SIZE/sizeof(DWORD))

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
char dbg_mob_allocate[] = "Allocted: %d\n";

char dbg_str[] = "%s\n";

char dbg_submitcb_fail[] = "CB submit FAILED\n";
char dbg_submitcb[] = "CB submit %d\n";
char dbg_lockcb[] = "Reused CB (%d) with status: %d\n";

char dbg_cb_on[] = "CB supported and allocated\n";
char dbg_gb_on[] = "GB supported and allocated\n";

char dbg_region_info_1[] = "Region id = %d\n";
char dbg_region_info_2[] ="Region address = %lX, PPN = %lX, GMRBLK = %lX\n";

#endif

typedef struct _otinfo_entry_t
{
	DWORD   phy;
	void   *lin;
	DWORD   size;
	DWORD   flags;
} otinfo_entry_t;

typedef struct _cmd_buf_t
{
	DWORD   phy;
	void   *lin;
	DWORD   status;
} cmd_buf_t;

#define CB_STATUS_EMPTY   0 /* buffer was not used yet */
#define CB_STATUS_LOCKED  1 /* buffer is using by Ring-3 proccess */
#define CB_STATUS_PROCESS 2 /* buffer is register by VGPU */

#define CB_COUNT 2

uint64 cmd_buf_next_id = {0, 0};

cmd_buf_t cmd_bufs[CB_COUNT] = {{0}};

BOOL cb_support = FALSE;
BOOL gb_support = FALSE;

#define ROUND_TO_PAGES(_cb)((((_cb) + PAGE_SIZE - 1)/PAGE_SIZE)*PAGE_SIZE)

#define SVGA3D_MAX_MOBS (SVGA3D_MAX_CONTEXT_IDS + SVGA3D_MAX_CONTEXT_IDS + SVGA3D_MAX_SURFACE_IDS)

#define FLAG_ALLOCATED 1
#define FLAG_ACTIVE    2

otinfo_entry_t otable[SVGA_OTABLE_DX_MAX] = {
	{0, NULL, ROUND_TO_PAGES(SVGA3D_MAX_MOBS*sizeof(SVGAOTableMobEntry)),              0}, /* SVGA_OTABLE_MOB */
	{0, NULL, ROUND_TO_PAGES(SVGA3D_MAX_SURFACE_IDS*sizeof(SVGAOTableSurfaceEntry)),   0}, /* SVGA_OTABLE_SURFACE */
	{0, NULL, ROUND_TO_PAGES(SVGA3D_MAX_CONTEXT_IDS*sizeof(SVGAOTableContextEntry)),   0}, /* SVGA_OTABLE_CONTEXT */
	{0, NULL, 0,                                                                       0}, /* SVGA_OTABLE_SHADER - not used */
	{0, NULL, ROUND_TO_PAGES(64*sizeof(SVGAOTableScreenTargetEntry)),                  0}, /* SVGA_OTABLE_SCREENTARGET (VBOX_VIDEO_MAX_SCREENS) */
	{0, NULL, ROUND_TO_PAGES(SVGA3D_MAX_CONTEXT_IDS*sizeof(SVGAOTableDXContextEntry)), 0}, /* SVGA_OTABLE_DXCONTEXT */
};

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
#define SVGA_REGION_CREATE   0x1114
#define SVGA_REGION_FREE     0x1115
#define SVGA_SYNC            0x1116
#define SVGA_RING            0x1117
#define SVGA_ALLOCPHY        0x1200
#define SVGA_FREEPHY         0x1201
#define SVGA_OTABLE_QUERY    0x1202
#define SVGA_OTABLE_FLAGS    0x1203

#define SVGA_CB_LOCK         0x1204
#define SVGA_CB_SUBMIT       0x1205
#define SVGA_CB_SYNC         0x1206

DWORD __stdcall Device_IO_Control_entry(struct DIOCParams *params);

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
 * @param outMobAddr: linear address of MOB PT block - if is NOT null,
 *                    extra memory of MOB maybe allocated. If result 
 *                    is NULL, MOB is full flat (no PT needed).
 * @param outMobPPN: physical page number of MOB
 *
 * @return: TRUE on success
 *
 **/
static BOOL GMRAlloc(ULONG nPages, ULONG *outDataAddr, ULONG *outGMRAddr, ULONG *outPPN, 
	ULONG *outMobAddr, ULONG *outMobPPN)
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
			
			if(outMobAddr)
				*outMobAddr = 0;
			
			if(outMobPPN)
				*outMobPPN = (phy/P_SIZE);

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
		
		//laddr = _PageAllocate(nPages, PG_SYS, 0, 0, 0x0, 0x100000, NULL, PAGEFIXED);
		laddr = 0;
		
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
				
				if(outMobAddr)
				{
					DWORD mobphy;
					/* for simplicity we're always creating table of depth 2, first page is page of PPN pages */
					DWORD *mob = (DWORD *)_PageAllocate(1 + (nPages + PTONPAGE - 1)/PTONPAGE, PG_SYS, 0, 0, 0x0, 0x100000, &mobphy,
						PAGECONTIG | PAGEUSEALIGN | PAGEFIXED);
					
					if(mob)
					{
						int i;
						/* dim 1 */
						for(i = 0; i < (nPages + PTONPAGE - 1)/PTONPAGE; i++)
						{
							mob[i] = (mobphy/PAGE_SIZE) + i + 1;
						}
						
						/* dim 2 */
						for(i = 0; i < nPages; i++)
						{
							mob[PTONPAGE + i] = getPPN(laddr + i*PAGE_SIZE);
						}
						
						*outMobAddr = (DWORD)mob;
						if(outMobPPN) *outMobPPN = mobphy/PAGE_SIZE;
					}
					else
					{
						*outMobAddr = NULL;
						if(outMobPPN) *outMobPPN = getPPN(laddr);
					}
				}
				
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
static BOOL GMRFree(ULONG DataAddr, ULONG GMRAddr, ULONG MobAddr)
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
	
	if(MobAddr != 0)
	{
		_PageFree((PVOID)MobAddr, 0);
	}
	
	return ret;
}

/**
 * Allocate OTable for GB objects
 **/
static void AllocateOTable()
{
	int i;
	for(i = 0; i < SVGA_OTABLE_DX_MAX; i++)
	{
		otinfo_entry_t *entry = &otable[i];
		if(entry->size != 0 && (entry->flags & FLAG_ALLOCATED) == 0)
		{
			entry->lin = (void*)_PageAllocate(entry->size/PAGE_SIZE, PG_SYS, 0, 0, 0x0, 0x100000, &entry->phy, PAGECONTIG | PAGEUSEALIGN | PAGEFIXED);
			if(entry->lin)
			{
				dbg_printf(dbg_mob_allocate, i);
				entry->flags |= FLAG_ALLOCATED;
			}
		}
	}
}

/**
 * Allocate Command Buffers
 **/
static void AllocateCB()
{
	int i;
	for(i = 0; i < CB_COUNT; i++)
	{
		if(cmd_bufs[i].phy == 0)
		{
			cmd_bufs[i].lin = (void *)_PageAllocate((SVGA_CB_MAX_SIZE+PAGE_SIZE-1)/PAGE_SIZE, PG_SYS, 0, 0, 0x0, 0x100000, &(cmd_bufs[i].phy), PAGECONTIG | PAGEUSEALIGN | PAGEFIXED);
			cmd_bufs[i].status = CB_STATUS_EMPTY;
		}
	}
}

/**
 * Test if selected CB is not used
 **/
static BOOL isCBfree(int index)
{
	switch(cmd_bufs[index].status)
	{
		case CB_STATUS_EMPTY:
			return TRUE;
		case CB_STATUS_PROCESS:
		{
			SVGACBHeader *cb = (SVGACBHeader *)cmd_bufs[index].lin;
			if(cb->status > SVGA_CB_STATUS_NONE)
			{
				return TRUE;
			}
			break;
		}
	}
	
	return FALSE;
}

/**
 * Increment 64-bit ID of CB segment
 **/
static void nextID_CB()
{
	_asm
	{
		inc dword ptr [cmd_buf_next_id]
		adc dword ptr [cmd_buf_next_id+4], 0
	}
}

/**
 * Lock one of buffers and return ptr for RING-3
 **/
static void *LockCB()
{
	int index;
	for(index = 0; index < CB_COUNT; index++)
	{
		if(isCBfree(index))
		{
			{
				SVGACBHeader *cb = (SVGACBHeader *)cmd_bufs[index].lin;
				dbg_printf(dbg_lockcb, index, cb->status);
			}
			cmd_bufs[index].status = CB_STATUS_LOCKED;
			return cmd_bufs[index].lin;
		}
	}
	
	return NULL;
}

/**
 * Submit CB to GPU.
 * If length is 0, buffer not executed
 * cbctx_id = SVGA_CB_CONTEXT_0,...
 **/

static BOOL submitCB(void *ptr, DWORD cbctx_id)
{
	int index;
	
	for(index = 0; index < CB_COUNT; index++)
	{
		if(cmd_bufs[index].lin == ptr)
		{
			SVGACBHeader *cb = (SVGACBHeader *)cmd_bufs[index].lin;
			if(cb->length > 0)
			{
				cb->status = SVGA_CB_STATUS_NONE;
				cb->ptr.pa.hi   = 0;
				cb->ptr.pa.low  = cmd_bufs[index].phy + sizeof(SVGACBHeader);
				cb->flags |= SVGA_CB_FLAG_NO_IRQ;
				cb->id.low = cmd_buf_next_id.low;
				cb->id.hi  = cmd_buf_next_id.hi;
				
				SVGA_WriteReg(SVGA_REG_COMMAND_HIGH, 0); // high part of 64-bit memory address...
				SVGA_WriteReg(SVGA_REG_COMMAND_LOW, cmd_bufs[index].phy | cbctx_id);
				cmd_bufs[index].status = CB_STATUS_PROCESS;
				
				dbg_printf(dbg_submitcb, index);
				
				nextID_CB();
			}
			else
			{
				cmd_bufs[index].status = CB_STATUS_EMPTY;
			}
			
			return TRUE;
		}
	}
	
	dbg_printf(dbg_submitcb_fail);
	
	return FALSE;
}

/**
 * Wait until all CB are completed
 **/
static void syncCB()
{
	int index;
	BOOL synced;
	do
	{
		synced = TRUE;
		for(index = 0; index < CB_COUNT; index++)
		{
			switch(cmd_bufs[index].status)
			{
				case CB_STATUS_EMPTY:
				case CB_STATUS_LOCKED:
					break;
				case CB_STATUS_PROCESS:
				{
					SVGACBHeader *cb = (SVGACBHeader *)cmd_bufs[index].lin;
					if(cb->status == SVGA_CB_STATUS_NONE)
					{
						synced = FALSE;
					}
					break;
				}
			}
		}
	} while(!synced);
}

/**
 * PM16 driver RING0 calls
 **/
BOOL CreateRegion(unsigned int nPages, ULONG *lpLAddr, ULONG *lpPPN, ULONG *lpPGBLK)
{	
	return GMRAlloc(nPages, lpLAddr, lpPGBLK, lpPPN, NULL, NULL);
}

BOOL FreeRegion(ULONG LAddr, ULONG PGBLK, ULONG MobAddr)
{
	return GMRFree(LAddr, PGBLK, MobAddr);
}

#if 0
/* I'm using this sometimes for developing, not for end user! */
BOOL AllocPhysical(DWORD nPages, DWORD *outLinear, DWORD *outPhysical)
{
	*outLinear = _PageAllocate(nPages, PG_SYS, 0, 0, 0x0, 0x100000, outPhysical, PAGECONTIG | PAGEUSEALIGN | PAGEFIXED);
	
	if(*outLinear != NULL)
	{
		return TRUE;
	}
	
	return FALSE;
}

void FreePhysical(DWORD linear)
{
	_PageFree((void*)linear, 0);
}
#endif

/* process V86/PM16 calls */
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
			if(FreeRegion(state->Client_ECX, state->Client_EBX, 0))
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
			
		if(SVGA_ReadReg(SVGA_REG_CAPABILITIES) & SVGA_CAP_GBOBJECTS)
		{
			AllocateOTable();
			gb_support = TRUE;
			dbg_printf(dbg_gb_on);
		}
		
		if(SVGA_ReadReg(SVGA_REG_CAPABILITIES) & (SVGA_CAP_COMMAND_BUFFERS | SVGA_CAP_CMD_BUFFERS_2))
		{
			AllocateCB();
			cb_support = TRUE;
			dbg_printf(dbg_cb_on);
		}
			
		VDD_Get_Mini_Dispatch_Table();
		if(DispatchTableLength >= 0x31)
		{
			#include "minivdd_func.h"
		}
	}
}
#undef VDDFUNC

/* process user space (PM32, RING-3) call */
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
#if 0
		case SVGA_ALLOCPHY:
		{
			DWORD nPages = ((DWORD*)params->lpInBuffer)[0];
			DWORD *linear = &((DWORD*)params->lpOutBuffer)[0];
			DWORD *phy = &((DWORD*)params->lpOutBuffer)[1];
			if(AllocPhysical(nPages, linear, phy))
			{
				return 0;
			}
			return 1;
		}
		case SVGA_FREEPHY:
		{
			DWORD linear = ((DWORD*)params->lpInBuffer)[0];
			FreePhysical(linear);
			return 0;
		}
#endif
		/* input:
		    - id 
		   output:
		    - linear
		    - physical
		    - size
		    - flags
		 */
		case SVGA_OTABLE_QUERY:
		{
			DWORD   id = ((DWORD*)params->lpInBuffer)[0];
			DWORD *out = (DWORD*)params->lpOutBuffer;
			
			if(!gb_support)
				return 1;
			
			if(id < SVGA_OTABLE_DX_MAX)
			{
				out[0] = (DWORD)otable[id].lin;
				out[1] = otable[id].phy;
				out[2] = otable[id].size;
				out[3] = otable[id].flags;
				return 0;
			}
			return 1;
		}
		/* input:
		   - id
		   - new flags
		 */
		case SVGA_OTABLE_FLAGS:
		{
			DWORD   id  = ((DWORD*)params->lpInBuffer)[0];
			DWORD flags = ((DWORD*)params->lpInBuffer)[1];
			
			if(!gb_support)
				return 1;
			
			if(id < SVGA_OTABLE_DX_MAX)
			{
				otable[id].flags = flags;
				return 0;
			}
			return 1;
		}
		/*
		 input:
		 output:
		  - linear address of buffer
		 */
		case SVGA_CB_LOCK:
		{
			if(cb_support)
			{
				void *ptr = LockCB();
				if(ptr)
				{
					DWORD* out = (DWORD*)params->lpOutBuffer;
					*out = (DWORD)ptr;
					return 0;
				}
			}
			return 1;
		}
		/*
		 input:
		  - linear address of buffer
		  - CB context ID
		 output:
		 */
		case SVGA_CB_SUBMIT:
		{
			if(cb_support)
			{
				DWORD *in = (DWORD*)params->lpInBuffer;
				
				if(submitCB((void*)in[0], in[1]))
				{
					return 0;
				}
			}
			return 1;
		}
		/* 
		 none arguments
		*/
		case SVGA_CB_SYNC:
			if(cb_support)
			{
				syncCB();
				return 0;
			}
			return 1;
		/*
		 input:
			- region id
			- pages
		 output:
			- region id
			- user page address
			- page block
			- mob page address
			- mob ppn
		*/
		case SVGA_REGION_CREATE:
		{
  		DWORD *lpIn  = (DWORD*)params->lpInBuffer;
  		DWORD *lpOut = (DWORD*)params->lpOutBuffer;
  		DWORD gmrPPN = 0;
  		
  		dbg_printf(dbg_region_info_1, lpIn[0]);
  		
			if(GMRAlloc(lpIn[1], &lpOut[1], &lpOut[2], &gmrPPN, &lpOut[3], &lpOut[4]))
			{
				if(lpIn[0] != 0)
				{
		    	SVGA_WriteReg(SVGA_REG_GMR_ID, lpIn[0]);
		    	SVGA_WriteReg(SVGA_REG_GMR_DESCRIPTOR, gmrPPN);
		    	
		    	dbg_printf(dbg_region_info_2, lpOut[1], gmrPPN, lpOut[2]);
		    	
		    	/* refresh all register, so make sure that new commands will accepts this region */
		    	SVGA_Flush();
		    	
		    	lpOut[0] = lpIn[0]; // copy GMR ID
				}
				return 0;
			}
			else
			{
				dbg_printf(dbg_region_err);
			}
  		
  		return 1;
		}
		/*
		 input
			- region id
			- user page address
			- page block address
			- mob PT address
		*/
		case SVGA_REGION_FREE:
		{
			DWORD *lpIn = (DWORD*)params->lpInBuffer;
			
			/* flush all register inc. fifo so make sure, that all commands are processed */
			if(cb_support)
			{
				syncCB();
			}
    	SVGA_Flush();
    
    	SVGA_WriteReg(SVGA_REG_GMR_ID, lpIn[0]);
    	SVGA_WriteReg(SVGA_REG_GMR_DESCRIPTOR, 0);
			
			/* sync again */
    	SVGA_Flush();
			
			if(FreeRegion(lpIn[1], lpIn[2], lpIn[3]))
			{
				return 0;
			}
			return 1;
		}
		case SVGA_DBG:
#ifdef DBGPRINT
			dbg_printf(dbg_str, params->lpInBuffer);
#endif			
			return 0;
	}
		
	dbg_printf(dbg_dic_unknown, params->dwIoControlCode);
	
	return 1;
}
