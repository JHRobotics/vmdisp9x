;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; Copyright (c) 2022  Michal Necasek
;
; Permission is hereby granted, free of charge, to any person obtaining a copy
; of this software and associated documentation files (the "Software"), to deal
; in the Software without restriction, including without limitation the rights
; to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
; copies of the Software, and to permit persons to whom the Software is
; furnished to do so, subject to the following conditions:
;
; The above copyright notice and this permission notice shall be included in
; all copies or substantial portions of the Software.
;
; THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
; IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
; FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
; AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
; LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
; OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
; THE SOFTWARE.
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; Screen switching hook written in assembler.

; See minidrv.h for more.
SCREEN_SWITCH_OUT	equ	4001h
SCREEN_SWITCH_IN    	equ	4002h

; Defined by us, must match minidrv.h
PREVENT_SWITCH		equ	80h

; Carry flag bit on the stack
FLAG_CF			equ	0001h

public	_OldInt2Fh
public	SWHook_

; Callbacks written in C
extrn	SwitchToBgnd_ : far
extrn	SwitchToFgnd_ : far

_DATA   segment public 'DATA'

; Defined in C code
extrn   _SwitchFlags : byte

; Dispatch table. Only used locally.
dsp_tbl	label	word
	dw	SwitchToBgnd_	; SCREEN_SWITCH_OUT handler
	dw	SwitchToFgnd_	; SCREEN_SWITCH_IN handler

_DATA   ends

DGROUP  group   _DATA

_TEXT   segment public 'CODE'

; Defined in code segment so that we can chain to the old interrupt
; handler without needing to mess with DS.
_OldInt2Fh	dd	0

	.386

; Screen switching hook for INT 2Fh. We need to handle screen switch
; notifications ourselves and let the previous handler deal with the
; rest.
SWHook_	proc	far

	; Cannot assume anything
	assume	ds:nothing, es:nothing

	; Check if the subfunction is one of those we're
	; interested in and chain to previous handler if not.
	cmp	ax, SCREEN_SWITCH_OUT
	jz	handle_fn
	cmp	ax, SCREEN_SWITCH_IN
	jz	handle_fn
	jmp	_OldInt2Fh		; Registers undisturbed (except flags)

	; We are going to handle this interrupt and not chain.
handle_fn:
	push	ds			; Save what we can
	pushad

	; Establish data segment addressing
	mov	bx, _DATA		; We'll overwrite BX later
	mov	ds, bx
	assume	ds:DGROUP

	mov	bx, ax			; We will need the function later
	mov	bp, sp			; So we can address stack

	; See if switching is disabled
	mov	al, _SwitchFlags
	test	al, PREVENT_SWITCH
	jz	dispatch		; If not set, do do the work

	; It's disabled; set carry flag and return.

	; To get to the saved flags, we need to skip the
	; return address (4 bytes), saved DS (2 bytes), and
	; saved registers (8 * 4 bytes)
FLG_OFS	equ	4 + 2 + 8 * 4

	or	word ptr [bp + FLG_OFS], FLAG_CF
	jmp	exit

	; Screen switching is not disabled; clear carry flag,
	; process the screen switch notification, and return.
dispatch:
	and	word ptr [bp + FLG_OFS], NOT FLAG_CF
	and	bx, 2			; Bit 1 of the subfunction
	call	dsp_tbl[bx]		; Call handler in C

exit:
	popad				; Restore and return
	pop	ds
	iret

SWHook_	endp


_TEXT	ends
	end
