#ifndef __VXD_VDD_H__INCLUDED__
#define __VXD_VDD_H__INCLUDED__

#ifdef DBGPRINT
void dbg_printf( const char *s, ... );
#else
#define dbg_printf(s, ...)
#endif

/* function codes */
#define REGISTER_DISPLAY_DRIVER     0
#define GET_VDD_BANK     1
#define SET_VDD_BANK     2
#define RESET_BANK     3
#define PRE_HIRES_TO_VGA     4
#define POST_HIRES_TO_VGA     5
#define PRE_VGA_TO_HIRES     6
#define POST_VGA_TO_HIRES     7
#define SAVE_REGISTERS     8
#define RESTORE_REGISTERS     9
#define MODIFY_REGISTER_STATE     10
#define ACCESS_VGA_MEMORY_MODE     11
#define ACCESS_LINEAR_MEMORY_MODE     12
#define ENABLE_TRAPS     13
#define DISABLE_TRAPS     14
#define MAKE_HARDWARE_NOT_BUSY     15
#define VIRTUALIZE_CRTC_IN     16
#define VIRTUALIZE_CRTC_OUT     17
#define VIRTUALIZE_SEQUENCER_IN     18
#define VIRTUALIZE_SEQUENCER_OUT     19
#define VIRTUALIZE_GCR_IN     20
#define VIRTUALIZE_GCR_OUT     21
#define SET_LATCH_BANK     22
#define RESET_LATCH_BANK     23
#define SAVE_LATCHES     24
#define RESTORE_LATCHES     25
#define DISPLAY_DRIVER_DISABLING     26
#define SELECT_PLANE     27
#define PRE_CRTC_MODE_CHANGE     28
#define POST_CRTC_MODE_CHANGE     29
#define VIRTUALIZE_DAC_OUT     30
#define VIRTUALIZE_DAC_IN     31
#define GET_CURRENT_BANK_WRITE     32
#define GET_CURRENT_BANK_READ     33
#define SET_BANK     34
#define CHECK_HIRES_MODE     35
#define GET_TOTAL_VRAM_SIZE     36
#define GET_BANK_SIZE     37
#define SET_HIRES_MODE     38
#define PRE_HIRES_SAVE_RESTORE     39
#define POST_HIRES_SAVE_RESTORE     40
#define VESA_SUPPORT     41
#define GET_CHIP_ID     42
#define CHECK_SCREEN_SWITCH_OK     43
#define VIRTUALIZE_BLTER_IO     44
#define SAVE_MESSAGE_MODE_STATE     45
#define SAVE_FORCED_PLANAR_STATE     46
#define VESA_CALL_POST_PROCESSING     47
#define PRE_INT_10_MODE_SET     48
#define NBR_MINI_VDD_FUNCTIONS_40     49
#define GET_NUM_UNITS     49
#define TURN_VGA_OFF     50
#define TURN_VGA_ON     51
#define SET_ADAPTER_POWER_STATE     52
#define GET_ADAPTER_POWER_STATE_CAPS     53
#define SET_MONITOR_POWER_STATE     54
#define GET_MONITOR_POWER_STATE_CAPS     55
#define GET_MONITOR_INFO     56
#define I2C_OPEN     57
#define I2C_ACCESS     58
#define GPIO_OPEN     59
#define GPIO_ACCESS     60
#define COPYPROTECTION_ACCESS     61
#define NBR_MINI_VDD_FUNCTIONS_41     62

/* VxDCall(VDD, ...) constants */
#define VDD__Get_Version 0
#define VDD__PIF_State 1
#define VDD__Get_GrabRtn 2
#define VDD__Hide_Cursor 3
#define VDD__Set_VMType 4
#define VDD__Get_ModTime 5
#define VDD__Set_HCurTrk 6
#define VDD__Msg_ClrScrn 7
#define VDD__Msg_ForColor 8
#define VDD__Msg_BakColor 9
#define VDD__Msg_TextOut 10
#define VDD__Msg_SetCursPos 11
#define VDD__Query_Access 12
#define VDD__Check_Update_Soon 13
#define VDD__Get_Mini_Dispatch_Table 14 /* edi -> pointer to table, ecx -> table items count */
#define VDD__Register_Virtual_Port 15
#define VDD__Get_VM_Info 16
#define VDD__Get_Special_VM_IDs 17
#define VDD__Register_Extra_Screen_Selector 18
#define VDD__Takeover_VGA_Port 19
#define VDD__Get_DISPLAYINFO 20
#define VDD__Do_Physical_IO 21
#define VDD__Set_Sleep_Flag_Addr 22
#define VDD__EnableDevice 23

/* generate prototypes */
#define VDDFUNC(_fnname, _procname) void __stdcall _procname ## _proc(PCRS_32 state);
#include "vxd_vdd_list.h"
#undef VDDFUNC

#define VDDPROC(_fnname, _procname) void __stdcall _procname ## _proc(PCRS_32 state)

#define VDD_CY state->Client_EFlags |= 0x1
#define VDD_NC state->Client_EFlags &= 0xFFFFFFFEUL

typedef struct {
	WORD    diHdrSize;
	WORD    diInfoFlags;
	DWORD   diDevNodeHandle;
	char    diDriverName[16];
	WORD    diXRes;
	WORD    diYRes;
	WORD    diDPI;
	BYTE    diPlanes;
	BYTE    diBpp;
	WORD    diRefreshRateMax;
	WORD    diRefreshRateMin;
	WORD    diLowHorz;
	WORD    diHighHorz;
	WORD    diLowVert;
	WORD    diHighVert;
	DWORD   diMonitorDevNodeHandle;
	BYTE    diHorzSyncPolarity;
	BYTE    diVertSyncPolarity;
} DISPLAYINFO;

void Enable_Global_Trapping(DWORD port);
void Disable_Global_Trapping(DWORD port);

#endif /* __VXD_VDD_H__INCLUDED__ */
