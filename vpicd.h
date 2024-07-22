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
#ifndef __VPICD_H__INCLUDED__
#define __VPICD_H__INCLUDED__

#pragma pack(push)
#pragma pack(1)
typedef struct _VPICD_IRQ_Descriptor
{
	WORD  IRQ_Number;
	WORD  Options;
	DWORD  Hw_Int_Proc;
	DWORD  Virt_Int_Proc;
	DWORD  EOI_Proc;
	DWORD  Mask_Change_Proc;
	DWORD  IRET_Proc;
	DWORD  IRET_Time_Out;
	DWORD  Hw_Int_Ref;
} VPICD_IRQ_Descriptor;
#pragma pack(pop)

#define VPICD_OPT_READ_HW_IRR	     0x01
#define VPICD_OPT_CAN_SHARE	       0x02
#define VPICD_OPT_VIRT_INT_REJECT	 0x10
#define VPICD_OPT_SHARE_PMODE_ONLY 0x20

#define VPICD__Get_Version               0
#define VPICD__Virtualize_IRQ            1
#define VPICD__Set_Int_Request           2
#define VPICD__Clear_Int_Request         3
#define VPICD__Phys_EOI                  4
#define VPICD__Get_Complete_Status       5
#define VPICD__Get_Status                6
#define VPICD__Test_Phys_Request         7
#define VPICD__Physically_Mask           8
#define VPICD__Physically_Unmask         9
#define VPICD__Set_Auto_Masking         10
#define VPICD__Get_IRQ_Complete_Status  11
#define VPICD__Convert_Handle_To_IRQ    12
#define VPICD__Convert_IRQ_To_Int       13
#define VPICD__Convert_Int_To_IRQ       14
#define VPICD__Call_When_Hw_Int         15
#define VPICD__Force_Default_Owner      16
#define VPICD__Force_Default_Behavior   17
#define VPICD__Auto_Mask_At_Inst_Swap   18
#define VPICD__Begin_Inst_Page_Swap     19
#define VPICD__End_Inst_Page_Swap       20
#define VPICD__Virtual_EOI              21
#define VPICD__Get_Virtualization_Count 22
#define VPICD__Post_Sys_Critical_Init   23
#define VPICD__VM_SlavePIC_Mask_Change  24
#define VPICD__Clear_IR_Bits            25
#define VPICD__Get_Level_Mask           26
#define VPICD__Set_Level_Mask           27
#define VPICD__Set_Irql_Mask            28
#define VPICD__Set_Channel_Irql         29
#define VPICD__Prepare_For_Shutdown     30
#define VPICD__Register_Trigger_Handler 31


#endif /* __VPICD_H__INCLUDED__ */
