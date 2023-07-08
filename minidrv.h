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

/* First definitions that don't appear to be in any "official" header. */

#define DRV_VERSION         0x0400      /* Windows 4.0 aka Windows 95. */

/* Int 2Fh subfunctions. */
#define STOP_IO_TRAP        0x4000      /* Stop trapping video I/O. */
#define SCREEN_SWITCH_OUT   0x4001      /* DOS session going to background. */
#define SCREEN_SWITCH_IN    0x4002      /* DOS session going to foreground. */
#define ENTER_CRIT_REG      0x4003      /* Enter critical section notification. */
#define EXIT_CRIT_REG       0x4004      /* Leave critical section notification. */
#define START_IO_TRAP       0x4007      /* Start trapping video I/O. */

/* Internal interfaces within minidriver. */

/* A simple mode descriptor structure. */
typedef struct {
    WORD    xRes;
    WORD    yRes;
    WORD    bpp;
} MODEDESC, FAR *LPMODEDESC;


extern WORD FixModeInfo( LPMODEDESC lpMode );
extern int PhysicalEnable( void );
extern void PhysicalDisable( void );
extern void FAR SetRAMDAC_far( UINT bStart, UINT bCount, RGBQUAD FAR *lpPal );
#ifdef QEMU
extern DWORD ReadDisplayConfig( void );
#else
extern void ReadDisplayConfig( void );
#endif
extern void FAR RestoreDesktopMode( void );
extern FARPROC RepaintFunc;
extern void HookInt2Fh( void );
extern void UnhookInt2Fh( void );

#ifdef DBGPRINT
extern void dbg_printf( const char *s, ... );
#else
/* The "Meaningless use of an expression" warning gets too annoying. */
//#pragma disable_message( 111 );
//#define dbg_printf  1 ? (void)0 : (void)
#define dbg_printf(...)
#endif

extern LPDIBENGINE lpDriverPDevice; /* DIB Engine PDevice. */
extern WORD ScreenSelector;         /* Selector of video memory. */
extern WORD wPalettized;            /* Non-zero if palettized device. */
extern WORD wPDeviceFlags;          /* Current GDI device flags. */
extern WORD wDpi;                   /* Current DPI. */
extern WORD wBpp;                   /* Current bits per pixel. */
extern WORD wScrX;                  /* Configured X resolution. */
extern WORD wScrY;                  /* Configured Y resolution. */
extern WORD wScreenX;               /* Screen width in pixels. */
extern WORD wScreenY;               /* Screen height in pixels. */
extern WORD wEnabled;               /* PDevice enabled flag. */
extern RGBQUAD FAR *lpColorTable;   /* Current color table. */

extern DWORD    dwScreenFlatAddr;   /* Linear address of frame buffer */
extern DWORD    dwVideoMemorySize;  /* Installed VRAM in bytes. */

extern DWORD    VDDEntryPoint;
extern WORD     OurVMHandle;
#ifdef QEMU
extern DWORD    LfbBase;
#endif

extern WORD wMesa3DEnabled;         /* Is possible to accelerate though Mesa3D SVGA */

WORD CalcPitch(WORD x, WORD bpp);

typedef void (__far * FastBitBlt_t) (unsigned dx, unsigned dy, unsigned sx, unsigned sy, unsigned w, unsigned h);
extern FastBitBlt_t FastBitBlt;

/*
 * frame buffer direct hadrware access, allow user space programs/drivers write
 * to device frame buffer directly
 */
#pragma pack(push)
#pragma pack(1)
typedef struct _FBHDA
{
	DWORD width;
	DWORD height;
	DWORD bpp;
	DWORD pitch;
	DWORD        fb_pm32; /* eq. linear address, mapped to shared or kernel space*/
	void __far * fb_pm16; /* usable in this driver */
	DWORD flags;
} FBHDA;
#pragma pack(pop)

#define FBHDA_NEED_UPDATE 1

extern FBHDA __far * FBHDA_ptr;
extern DWORD FBHDA_linear;

/* Inlines needed in multiple modules. */

void int_2Fh( unsigned ax );
#pragma aux int_2Fh =   \
    "int    2Fh"        \
    parm [ax];

/* NB: It's unclear of EAX/EBX really need preserving. */
extern void CallVDD( unsigned Function );
#pragma aux CallVDD =               \
    ".386"                          \
    "push   eax"                    \
    "push   ebx"                    \
    "movzx  eax, ax"                \
    "movzx  ebx, OurVMHandle"       \
    "call dword ptr VDDEntryPoint"  \
    "pop    ebx"                    \
    "pop    eax"                    \
    parm [ax];

/* DirectDraw support */
BOOL DDCreateDriverObject(int bReset);

/* 9x VRAM limit */
#define MAX_VRAM 0x8000000UL /* 128 MB */

#ifdef SVGA
#define SVGA_PARTIAL_UPDATE_MAX 16
extern void SVGA_UpdateRect(LONG x, LONG y, LONG w, LONG h);
#endif
