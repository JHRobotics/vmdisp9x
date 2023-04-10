#ifndef __DPMI_H__INCLUDED__
#define __DPMI_H__INCLUDED__

extern WORD DPMI_AllocLDTDesc( WORD cSelectors );
#pragma aux DPMI_AllocLDTDesc = \
    "xor    ax, ax"             \
    "int    31h"                \
    "jnc    OK"                 \
    "xor    ax, ax"             \
    "OK:"                       \
    parm [cx];

extern DWORD DPMI_GetSegBase( WORD Selector );
#pragma aux DPMI_GetSegBase =   \
    "mov    ax, 6"              \
    "int    31h"                \
    "mov    ax, cx"             \
    "xchg   ax, dx"             \
    parm [bx] modify [cx];

extern void DPMI_SetSegBase( WORD Selector, DWORD Base );
#pragma aux DPMI_SetSegBase =   \
    "mov    ax, 7"              \
    "int    31h"                \
    parm [bx] [cx dx];

extern void DPMI_SetSegLimit( WORD Selector, DWORD Limit );
#pragma aux DPMI_SetSegLimit =   \
    "mov    ax, 8"              \
    "int    31h"                \
    parm [bx] [cx dx];

/* NB: Compiler insists on CX:BX and DI:SI, DPMI needs it word swapped. */
extern DWORD DPMI_MapPhys( DWORD Base, DWORD Size );
#pragma aux DPMI_MapPhys =      \
    "xchg   cx, bx"             \
    "xchg   si, di"             \
    "mov    ax, 800h"           \
    "int    31h"                \
    "jnc    OK"                 \
    "xor    bx, bx"             \
    "xor    cx, cx"             \
    "OK:"                       \
    "mov    dx, bx"             \
    "mov    ax, cx"             \
    parm [cx bx] [di si];

extern DWORD DPMI_AllocMemBlk(DWORD Size);
#pragma aux DPMI_AllocMemBlk =  \
    "xchg   cx, bx"             \
    "mov    ax, 501h"           \
    "int    31h"                \
    "jnc    alloc_ok"           \
    "xor    bx, bx"             \
    "xor    cx, cx"             \
    "alloc_ok:"                 \
    "mov    dx, bx"             \
    "mov    ax, cx"             \
    parm [cx bx] modify [di si];
    

static DWORD DPMI_AllocLinBlk_LinAddress = 0;
static DWORD DPMI_AllocLinBlk_MHandle = 0;
    
extern WORD DPMI_AllocLinBlk(DWORD size);
#pragma aux DPMI_AllocLinBlk =  \
    ".386"                      \
    "push ebx"                  \
    "push ecx"                  \
    "push esi"                  \
    "push edx"                  \
    "push eax"                  \
    "shl  ecx, 16"              \
    "mov   cx, bx"              \
    "xor  ebx, ebx"             \
    "mov  edx, 1"               \
    "mov  eax, 504h" /* alloc linear address (in EBX, handle in ESI) */ \
    "int  31h"                  \
    "jc alloc_fail"             \
      "mov dword ptr [DPMI_AllocLinBlk_LinAddress], ebx" \
      "mov dword ptr [DPMI_AllocLinBlk_MHandle], esi" \
      "pop eax"                 \
      "mov ax, 0"               \
      "pop edx"                 \
      "pop esi"                 \
      "pop ecx"                 \
      "pop ebx"                 \
      "jmp alloc_end"           \
    "alloc_fail:"               \
      "mov dword ptr [DPMI_AllocLinBlk_LinAddress], ecx" \
      "mov dx, ax"              \
      "pop eax"                 \
      "mov ax, dx"              \
      "pop edx"                 \
      "pop esi"                 \
      "pop ecx"                 \
      "pop ebx"                 \
      "alloc_end:"              \
    parm [cx bx] modify [ax];

extern WORD DPMI_LockRegion(DWORD linAddress, DWORD size);
#pragma aux DPMI_LockRegion = \
    "xchg   cx, bx"           \
    "xchg   si, di"           \
    "mov    ax, 600h"         \
    "int    31h"              \
    "jc lock_fail"            \
    "xor ax, ax"              \
    "lock_fail:"              \
    parm [cx bx] [di si] modify [ax];

extern void DPMI_FreeLDTDesc ( WORD desc);
#pragma aux DPMI_FreeLDTDesc =  \
    "mov    ax, 1"              \
    "int    31h"                \
    parm [bx];


#endif /* __DPMI_H__INCLUDED__ */
