/*****************************************************************************

Copyright (c) 2022  Michal Necasek

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

/* Screen switching support (C code). */

#include "winhack.h"
#include <gdidefs.h>
#include <dibeng.h>
#include "minidrv.h"
#include "3d_accel.h"

/* SwitchFlags bits. */
#define PREVENT_SWITCH  0x80    /* Don't allow screen switching. */
#define INT_2F_HOOKED   0x01    /* INT 2Fh is hooked. */

/* Accessed from sswhook.asm. */
BYTE SwitchFlags = 0;           /* Screen switch flags, see above. */

static BOOL bNoRepaint = 0;     /* Set when USER disables repaints. */
static BOOL bPaintPending = 0;  /* Set when repaint was postponed.*/

FARPROC RepaintFunc = 0;        /* Address of repaint callback. */

/* Screen switch hook for INT 2Fh. */
extern void __far SWHook( void );

#pragma code_seg( _INIT )

/* The pointer is in the code (_TEXT) segment, not data segment.
 * Defined in sswhook.asm.
 */
extern void __far * __based( __segname( "_TEXT" ) ) OldInt2Fh;


/* Call DOS to get an interrupt vector. */
void __far *DOSGetIntVec( BYTE bIntNo );
#pragma aux DOSGetIntVec =  \
    "mov    ah, 35h"        \
    "int    21h"            \
    parm [al] value [es bx];

/* Call DOS to set an interrupt vector. */
void DOSSetIntVec( BYTE bIntNo, void __far *NewVector );
#pragma aux DOSSetIntVec =  \
    "mov    ah, 25h"        \
    "push   ds"             \
    "push   es"             \
    "pop    ds"             \
    "int    21h"            \
    "pop    ds"             \
    parm [al] [es dx];


/* Ordinal of the repaint callback in USER. Of course undocumented. */
#define REPAINT_ORDINAL     275

/* Fun times. Undocumented function to create a writable alias of a code
 * selector. In MS KB Q67165, Microsoft claimed that the function "is not
 * documented and will not be supported in future versions of Windows".
 * Clearly they lied, as it happily works on Win9x.
 */
extern UINT WINAPI AllocCStoDSAlias( UINT selCode );

/* Internal function to install INT 2Fh hook. */
void HookInt2Fh( void )
{
    WORD                selAlias;
    void __far * __far  *OldInt2FhAlias;    /* This is fun! */

    /* If not already set, get repaing callback from USER. */
    if( !RepaintFunc ) {
        HMODULE hMod;

        hMod = GetModuleHandle( "USER" );
        RepaintFunc = GetProcAddress( hMod, (LPCSTR)REPAINT_ORDINAL );
    }
    dbg_printf( "HookInt2Fh: RepaintFunc=%WP\n", RepaintFunc );

    /* Now hook INT 2Fh. Since the address of the previous INT 2Fh handler
     * is in the code segment, we have to create a writable alias. Just
     * a little tricky.
     */
    selAlias = AllocCStoDSAlias( (__segment)&OldInt2Fh );

    SwitchFlags |= INT_2F_HOOKED;
    /* Build a pointer to OldInt2Fh using the aliased selector. */
    OldInt2FhAlias = selAlias :> (WORD)&OldInt2Fh;
    *OldInt2FhAlias = DOSGetIntVec( 0x2F );
    DOSSetIntVec( 0x2F, SWHook );

    FreeSelector( selAlias );
}

/* Internal function to unhook INT 2Fh. */
void UnhookInt2Fh( void )
{
    if( SwitchFlags & INT_2F_HOOKED ) {
        /* We have INT 2Fh hooked, undo that. */
        SwitchFlags &= ~INT_2F_HOOKED;
        DOSSetIntVec( 0x2F, OldInt2Fh );
    }
}

#ifdef SVGA
static void __far __loadds SVGA_background()
{
	SVGA_HW_disable();
}

static void __far __loadds SVGA_foreground()
{
	SVGA_HW_enable();
}
#endif /* SVGA */

#pragma code_seg( _TEXT )

/* Repaint screen or postpone for later.
 * Internal near call.
 */
void RepaintScreen( void )
{
    dbg_printf( "RepaintScreen: RepaintFunc=%WP, bNoRepaint=%u\n", RepaintFunc, bNoRepaint );
    
    /* Do we have a repaint callback? */
    if( RepaintFunc ) {
        /* Either do the work or set the pending flag. */
        if( !bNoRepaint )
            RepaintFunc();
        else
            bPaintPending = TRUE;
    }
}

/* This function is exported as DISPLAY.500 and called by USER
 * to inform the driver that screen repaint requests should not
 * be sent. When we need to repaint while repaints are disabled,
 * we set an internal flag and do the redraw as soon as repaints
 * are enabled again.
 */
VOID WINAPI __loadds UserRepaintDisable( BOOL bDisable )
{
    bNoRepaint = bDisable;
    if( !bDisable && bPaintPending ) {
        RepaintScreen();
        bPaintPending = 0;
    }
}

/* Called from INT 2Fh hook when the device is switching to the
 * background and needs to disable drawing.
 * Internal near call.
 */
void SwitchToBgnd( void )
{
    dbg_printf( "SwitchToBgnd\n" );
#ifdef SVGA
    SVGA_background();
#endif

    lpDriverPDevice->deFlags |= BUSY;   /// @todo Does this need to be a locked op?
}

/* Called from INT 2Fh hook when the device is switching to the
 * foreground and needs re-enable drawing, repaiting the screen
 * and possibly restoring the display mode.
 * Internal near call.
 */
void SwitchToFgnd( void )
{
    dbg_printf( "SwitchToFgnd\n" );
#ifdef SVGA
    SVGA_foreground();
#endif

    /* If the PDevice is busy, we need to reset the display mode. */
    if( lpDriverPDevice->deFlags & BUSY )
        RestoreDesktopMode(); /* Will clear the BUSY flag. */
    RepaintScreen();
}

/* This minidriver does not currently disable or enable switching.
 * Should the functions be needed, they can be enabled.
 */
#if 0

/* Called to prevent screen switching.
 * Internal near call.
 */
void ScrSwDisable( void )
{
    int_2Fh( ENTER_CRIT_REG );
    SwitchFlags |= PREVENT_SWITCH;
}

/* Called to re-enable screen switching again.
 * Internal near call.
 */
void ScrSwEnable( void )
{
    int_2Fh( EXIT_CRIT_REG );
    SwitchFlags &= ~PREVENT_SWITCH;
}

#endif

