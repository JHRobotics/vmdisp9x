/*****************************************************************************

Copyright (c) 2022  Michal Necasek
              2023  Philip Kelley
              2023  Jaroslav Hensl

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

/* Display driver mode management. */

#include "winhack.h"
#include <gdidefs.h>
#include <dibeng.h>
#include <valmode.h>
#include <minivdd.h>

#include "minidrv.h"
#include "boxv.h"

#ifdef SVGA
# include "svga_all.h"
#endif

#include "drvlib.h"
#include "dpmi.h"

#include <string.h> /* _fmemset */
#include <stdlib.h> /* abs */

/* Somewhat arbitrary max resolution. */
//#define RES_MAX_X   (5 * 1024)
//#define RES_MAX_Y   (5 * 768)
#define RES_MAX_X 5120
#define RES_MAX_Y 4096
/* ^ JH: 5K in 5:4 */

WORD wScreenX       = 0;
WORD wScreenY       = 0;
WORD BitBltDevProc  = 0;
WORD ScreenSelector = 0;
WORD wPDeviceFlags  = 0;

#ifdef SVGA
WORD wMesa3DEnabled = 0; /* 3D is enabled (by hypervisor) */
#endif

/* FBHDA structure pointers */
FBHDA __far * FBHDA_ptr = NULL;
DWORD FBHDA_linear = 0;

       DWORD    dwScreenFlatAddr = 0;   /* 32-bit flat address of VRAM. */
       DWORD    dwVideoMemorySize = 0;  /* Installed VRAM in bytes. */
static WORD     wScreenPitchBytes = 0;  /* Current scanline pitch. */
static DWORD    dwPhysVRAM = 0;         /* Physical LFB base address. */

/* These are currently calculated not needed in the absence of
 * offscreen video memory.
 */
static WORD wMaxWidth  = 0;
static WORD wMaxHeight = 0;

/* On Entry:
 * EAX   = Function code (VDD_DRIVER_REGISTER)
 * EBX   = This VM's handle
 * ECX   = Size of all visible scanlines in bytes
 * EDX   = Zero to tell VDD to try virtualizing
 * ES:DI = Pointer to function called when switching
 *         back from fullscreen.
 *
 * On Return:
 * EAX   = Amount of video memory used by VDD in bytes,
 *         or function code if VDD call failed.
 */
extern DWORD CallVDDRegister( WORD Function, WORD wPitch, WORD wHeight, void _far *fRHR );
#pragma aux CallVDDRegister =       \
    ".386"                          \
    "movzx  eax, ax"                \
    "movzx  edx, dx"                \
    "mul    edx"                    \
    "mov    ecx, eax"               \
    "movzx  eax, bx"                \
    "movzx  ebx, OurVMHandle"       \
    "xor    edx, edx"               \
    "call   dword ptr VDDEntryPoint"\
    "mov    edx, eax"               \
    "shr    edx, 16"                \
    parm [bx] [ax] [dx] [es di];


/* from control.c */
#ifdef SVGA
void SVGAHDA_init();
WORD SVGA_3DSupport();
void SVGAHDA_update(DWORD width, DWORD height, DWORD bpp, DWORD pitch);
#endif

#pragma code_seg( _INIT );

/* Take a mode descriptor and change it to be a valid
 * mode if it isn't already. Return zero if mode needed
 * fixing (wasn't valid).
 */
WORD FixModeInfo( LPMODEDESC lpMode )
{
    WORD    rc = 1; /* Assume valid mode. */

    /* First validate bits per pixel. */
    switch( lpMode->bpp ) {
    case 8:
    case 16:
    case 24:
    case 32:
        break;
    default:
        lpMode->bpp = 8;    /* Default to 8 bpp. */
        rc = 0;             /* Mode wasn't valid. */
    }

    /* Validate mode. If resolution is under 640x480 in
     * either direction, force 640x480.
     */
    if( lpMode->xRes < 640 || lpMode->yRes < 480 )
    {
        lpMode->xRes = 640; /* Force 640x480. */
        lpMode->yRes = 480;
        rc = 0;             /* Mode wasn't valid. */
    }

    /* Clip the resolution to something that probably won't make
     * Windows have a cow.
     */
    if( lpMode->xRes > RES_MAX_X ) {
        lpMode->xRes = RES_MAX_X;
        rc = 0;
    }
    if( lpMode->yRes > RES_MAX_Y ) {
        lpMode->yRes = RES_MAX_Y;
        rc = 0;
    }

    return( rc );
}


/* Calculate pitch for a given horizontal resolution and bpp. */
WORD CalcPitch( WORD x, WORD bpp )
{
    WORD    wPitch;

    /* Valid BPP must be a multiple of 8 so it's simple. */
    wPitch = x * (bpp / 8);

    /* Align to 32 bits. */
    wPitch = (wPitch + 3) & ~3;

    return( wPitch );
}


/* Return non-zero if given mode is supported. */
static int IsModeOK( WORD wXRes, WORD wYRes, WORD wBpp )
{
    MODEDESC    mode;
    DWORD       dwModeMem;

    mode.bpp  = wBpp;
    mode.xRes = wXRes;
    mode.yRes = wYRes;

    /* If mode needed fixing, it's not valid. */
    if( !FixModeInfo( &mode ) )
        return( 0 );

#ifdef SVGA
    /* not working in vmware, in vbox is working without acceleration, so only confusing users */
    if(wBpp == 24)
    {
      return 0;
    }
    
    /* some implementations not support 8 and 16 bpp */
    if(gSVGA.only32bit && wBpp != 32)
    {
    	return 0;
    }
#endif
    
    /* Make sure there's enough VRAM for it. */
    dwModeMem = (DWORD)CalcPitch( wXRes, wBpp ) * wYRes;
    if( dwModeMem > dwVideoMemorySize )
        return( 0 );

#ifdef SVGA
    /* even if SVGA support other bpp, they are not usable
       if they're larger than traceable size */
    if(wBpp != 32)
    {
        if(dwModeMem > SVGA_FB_MAX_TRACEABLE_SIZE)
            return 0;
    }
#endif

    return( 1 );
}


/* Clear the visible screen by setting it to all black (zeros).
 * NB: Assumes there is no off-screen region to the right of
 * the visible area.
 */
static void ClearVisibleScreen( void )
{
	LPDWORD     lpScr;
  WORD        wLines = wScreenY;
  WORD        i;

  lpScr = ScreenSelector :> 0;
  while( wLines-- ) {
    for( i = 0; i < wScreenPitchBytes / 4; ++i )
    {
      lpScr[i] = 0;
    }
    lpScr += wScreenPitchBytes; // Scanline pitch.
  }
}

/* Map physical memory to memory space, lpLinAddress is option pointer to store
 * mapped linear address
 */
static DWORD AllocLinearSelector(DWORD dwPhysAddr, DWORD dwSize, DWORD __far * lpLinAddress)
{
    WORD    wSel;
    DWORD   dwLinear;

    wSel = DPMI_AllocLDTDesc( 1 );  /* One descriptor, please. */
    if( !wSel )
        return( 0 );

    /* Map the framebuffer physical memory. */
    dwLinear = DPMI_MapPhys( dwPhysAddr, dwSize );

    /* Now set the allocated selector to point to VRAM. */
    DPMI_SetSegBase( wSel, dwLinear );
    DPMI_SetSegLimit( wSel, dwSize - 1 );
    
    if(lpLinAddress != NULL)
    {
    	*lpLinAddress = dwLinear;
    }

    return( wSel );
}

#ifdef SVGA
/*
 * Define the screen for accelerated rendering.
 * Color depth can by select by set: screen.backingStore.pitch
 */
static void SVGA_defineScreen(unsigned wXRes, unsigned wYRes, unsigned wBpp)
{
  SVGAFifoCmdDefineScreen __far *screen;
   
  /* create screen 0 */
  screen = SVGA_FIFOReserveCmd(SVGA_CMD_DEFINE_SCREEN, sizeof(SVGAFifoCmdDefineScreen));
  
  if(screen)
  {
	  _fmemset(screen, 0, sizeof(SVGAFifoCmdDefineScreen));
	  screen->screen.structSize = sizeof(SVGAScreenObject);
	  screen->screen.id = 0;
	  screen->screen.flags = SVGA_SCREEN_MUST_BE_SET | SVGA_SCREEN_IS_PRIMARY;
	  screen->screen.size.width = wXRes;
	  screen->screen.size.height = wYRes;
	  screen->screen.root.x = 0;
	  screen->screen.root.y = 0;
	  screen->screen.cloneCount = 0;
	  
	  screen->screen.backingStore.pitch = CalcPitch(wXRes, wBpp);
	  
	  SVGA_FIFOCommitAll();
	}
}

/* Check if screen acceleration is available */
static BOOL SVGA_hasAccelScreen()
{
  if(SVGA_HasFIFOCap(SVGA_FIFO_CAP_SCREEN_OBJECT | SVGA_FIFO_CAP_SCREEN_OBJECT_2))
  {
    return TRUE;
  }
  
  return FALSE;
}

static DWORD SVGA_partial_update_cnt = 0;

/* Update screen rect if its relevant */
extern void __loadds SVGA_UpdateRect(LONG x, LONG y, LONG w, LONG h)
{
	/* SVGA commands works only for 32 bpp surfaces */
	if(wBpp != 32)
	{
		return;
	}
	
	if(SVGA_partial_update_cnt++ > SVGA_PARTIAL_UPDATE_MAX)
	{
		SVGA_Update(0, 0, wScreenX, wScreenY);
		SVGA_partial_update_cnt = 0;
		return;
	}
	
	if(x < 0) x = 0;
	if(y < 0) y = 0;
	if(x+w > wScreenX) w = wScreenX - x;
	if(y+h > wScreenY) y = wScreenY - h;
		
	if(w > 0 && h > 0)
	{
		SVGA_Update(x, y, w, h);
	}
	
	/* test if it's full update */
	if(x == 0 && y == 0 && w == wScreenX && h == wScreenY)
	{
		SVGA_partial_update_cnt = 0;
	}
}

/* Initialize SVGA structure and map FIFO to memory */
static int __loadds SVGA_full_init()
{
  int rc = 0;
  DWORD fifosel = 0; 
  
  dbg_printf("VMWare SVGA-II init\n");
  
  rc = SVGA_Init(FALSE);
  
  if(rc != 0)
  {
    return rc;
  }
  
  /* gSVGA.fifoMem now holding physical address, map it to linear and convert this address to far pointer */
  gSVGA.fifoPhy = (uint32)gSVGA.fifoMem;
  fifosel = AllocLinearSelector(gSVGA.fifoPhy, gSVGA.fifoSize, &gSVGA.fifoLinear);
  if(fifosel == 0)
  {
    dbg_printf("SVGA: Failed to map FIFO memory!\n");
    return 2;
  }
  
  dbg_printf("SVGA: memory selector for fifo: %lX (physical: %lX, size: %lX)\n", fifosel, gSVGA.fifoMem, gSVGA.fifoSize);
  gSVGA.fifoMem = fifosel :> 0x0;
  
  /* from PM16 cannot be accesed full fifo buffer properly, so allocate selector to maping as 64K sector */
  gSVGA.fifoSel = DPMI_AllocLDTDesc(1);
  if(gSVGA.fifoSel)
  {
	  DPMI_SetSegLimit(gSVGA.fifoSel, 0xFFFF);
	  
	  gSVGA.fifoAct = 0;
	  DPMI_SetSegBase(gSVGA.fifoSel, gSVGA.fifoLinear);
	}
  
  /* sets FIFO */
  SVGA_Enable();
  
  return 0;
}
#endif

/* Set the currently configured mode (wXRes/wYRes) in hardware.
 * If bFullSet is non-zero, then also reinitialize globals.
 * When re-establishing a previously set mode (e.g. coming
 * back from fullscreen), bFullSet will be zero.
 * NB: BPP won't change at runtime.
 */
static int SetDisplayMode( WORD wXRes, WORD wYRes, int bFullSet )
{
    dbg_printf( "SetDisplayMode: wXRes=%u wYRes=%u\n", wXRes, wYRes );

    /* Inform the VDD that the mode is about to change. */
    CallVDD( VDD_PRE_MODE_CHANGE );

#ifdef SVGA
    /* Make sure, that we drain full FIFO */
    SVGA_Flush(); 
    
    SVGA_SetMode(wXRes, wYRes, wBpp); /* setup by legacy registry */
    wMesa3DEnabled = 0;
    if(SVGA3D_Init())
    {
    	wMesa3DEnabled = SVGA_3DSupport();
    }
    
    /* setting screen by fifo, this method is required in VB 6.1 */
    if(SVGA_hasAccelScreen())
    {
      SVGA_defineScreen(wXRes, wYRes, wBpp);
    }
    
    /*
     * JH: this is a bit stupid = all SVGA command cannot work with non 32 bpp. 
     * SVGA_CMD_UPDATE included. So if we're working in 32 bpp, we'll disable
     * traces and updating framebuffer changes with SVGA_CMD_UPDATE.
     * On non 32 bpp we just enable SVGA_REG_TRACES.
     *
     * QEMU hasn't SVGA_REG_TRACES register and framebuffer cannot be se to
     * 16 or 8 bpp = we supporting only 32 bpp moders if we're running under it.
     */
    if(wBpp == 32)
    {
    	SVGA_WriteReg(SVGA_REG_TRACES, FALSE);
    }
    else
    {
    	SVGA_WriteReg(SVGA_REG_TRACES, TRUE);
    }
    
    SVGA_WriteReg(SVGA_REG_ENABLE, TRUE);
    SVGA_Flush();
    
    dbg_printf("Pitch: %lu\n", SVGA_ReadReg(SVGA_REG_BYTES_PER_LINE));
    
    SVGAHDA_update(wScrX, wScrY, wBpp, SVGA_ReadReg(SVGA_REG_BYTES_PER_LINE));
#else
    BOXV_ext_mode_set( 0, wXRes, wYRes, wBpp, wXRes, wYRes );

#endif

    if(FBHDA_ptr)
    {
        FBHDA_ptr->width  = wScrX;
        FBHDA_ptr->height = wScrY;
        FBHDA_ptr->bpp    = wBpp;
#ifdef SVGA
        FBHDA_ptr->pitch = SVGA_ReadReg(SVGA_REG_BYTES_PER_LINE);
        FBHDA_ptr->flags = FBHDA_NEED_UPDATE;
#else
        FBHDA_ptr->pitch  = CalcPitch( wScrX, wBpp );
        FBHDA_ptr->flags = 0;
#endif
    }
    
    if( bFullSet ) {
        wScreenX = wXRes;
        wScreenY = wYRes;
#ifdef SVGA
        wScreenPitchBytes = SVGA_ReadReg(SVGA_REG_BYTES_PER_LINE);
#else
        wScreenPitchBytes = CalcPitch( wXRes, wBpp );
#endif

        BitBltDevProc     = NULL;       /* No acceleration implemented. */

        wPDeviceFlags     = MINIDRIVER | VRAM | OFFSCREEN;
        if( wBpp == 16 ) {
            wPDeviceFlags |= FIVE6FIVE; /* Needed for 16bpp modes. */
        }
        
        wMaxWidth  = wScreenPitchBytes / (wBpp / 8);    /* We know bpp is a multiple of 8. */
        wMaxHeight = dwVideoMemorySize / wScreenPitchBytes;

        /* Offscreen regions could be calculated here. We do not use those. */
    }
    return( 1 );
}


/* Forward declaration. */
void __far RestoreDesktopMode( void );

int PhysicalEnable( void )
{
    DWORD   dwRegRet;

    if( !ScreenSelector ) {
#ifdef SVGA
        int rc = 0;
        dbg_printf("PhysicalEnable: entry mode %ux%u\n", wScrX, wScrY);
        rc = SVGA_full_init();
        
        /* Extra work if driver hasn't yet been initialized. */
        if(rc != 0)
        {
          dbg_printf("SVGA_Init() failure: %d, wrong device?\n", rc);
          return 0;
        }
        
        dwVideoMemorySize = SVGA_ReadReg(SVGA_REG_VRAM_SIZE);        
        dwPhysVRAM = SVGA_ReadReg(SVGA_REG_FB_START);
#else
        int     iChipID;

        /* Extra work if driver hasn't yet been initialized. */
        iChipID = BOXV_detect( 0, &dwVideoMemorySize );
        if( !iChipID ) {
            return( 0 );
        }
# ifdef QEMU
        dwPhysVRAM = LfbBase;
# else
        dwPhysVRAM = BOXV_get_lfb_base( 0 );
# endif
#endif
        /* limit vram size */
        if(dwVideoMemorySize > MAX_VRAM)
        {
        	dwVideoMemorySize = MAX_VRAM;
        }

        dbg_printf( "PhysicalEnable: Hardware detected, dwVideoMemorySize=%lX dwPhysVRAM=%lX\n", dwVideoMemorySize, dwPhysVRAM );
    }
    
    dbg_printf("PhysicalEnable: continue with %ux%u\n", wScrX, wScrY);
    if( !IsModeOK( wScrX, wScrY, wBpp ) ) {
        /* Can't find mode, oopsie. */
        dbg_printf( "PhysicalEnable: Mode not valid! wScrX=%u wScrY=%u wBpp=%u\n", wScrX, wScrY, wBpp );
        return( 0 );
    }

    if( !SetDisplayMode( wScrX, wScrY, 1 ) ) {
        /* This is not good. */
        dbg_printf( "PhysicalEnable: SetDisplayMode failed! wScrX=%u wScrY=%u wBpp=%u\n", wScrX, wScrY, wBpp );
        return( 0 );
    }

    /* Allocate an LDT selector for the screen. */
    if( !ScreenSelector ) {
    	  //ScreenSelector = AllocLinearSelector( dwPhysVRAM, dwVideoMemorySize );
        ScreenSelector = AllocLinearSelector(dwPhysVRAM, dwVideoMemorySize, &dwScreenFlatAddr);
        if( !ScreenSelector ) {
            dbg_printf( "PhysicalEnable: AllocScreenSelector failed!\n" );
            return( 0 );
        }

#ifdef SVGA   
        gSVGA.fbLinear = dwScreenFlatAddr;
        gSVGA.fbPhy = dwPhysVRAM;
        gSVGA.fbMem = ScreenSelector :> 0;
        	
        /* init userspace hardware access */
        SVGAHDA_init();
#endif
    }
    
    /* NB: Currently not used. DirectDraw would need the segment base. */
    /* JH: we need it for FIFO (SVGA) or direct FB rendering, but retuned by AllocLinearSelector in one CALL */
    //dwScreenFlatAddr = DPMI_GetSegBase( ScreenSelector );   /* Not expected to fail. */

    dbg_printf( "PhysicalEnable: RestoreDesktopMode is at %WP\n", RestoreDesktopMode );
    dwRegRet = CallVDDRegister( VDD_DRIVER_REGISTER, wScreenPitchBytes, wScreenY, RestoreDesktopMode );
    if( dwRegRet != VDD_DRIVER_REGISTER ) {
        /* NB: It's not fatal if CallVDDRegister() fails. */
        /// @todo What can we do with the returned value?
    }
    
    /* allocate and fill FBHDA */
    if(FBHDA_ptr == NULL)
    {
    	FBHDA_ptr = drv_malloc(sizeof(FBHDA), &FBHDA_linear);
    	if(FBHDA_ptr)
    	{
    		FBHDA_ptr->width  = wScrX;
    		FBHDA_ptr->height = wScrY;
    		FBHDA_ptr->bpp    = wBpp;
    		FBHDA_ptr->pitch  = wScreenPitchBytes;
    		
    		FBHDA_ptr->fb_pm32 = dwScreenFlatAddr;
    		FBHDA_ptr->fb_pm16 = ScreenSelector :> 0;
    	}
    	else
    	{
    		dbg_printf( "FBHDA_ptr = drv_malloc FAIL\n" );
    	}
    }

#ifdef SVGA
    /* update SVGAHDA */
    SVGAHDA_update(wScrX, wScrY, wBpp, wScreenPitchBytes);
#endif

    /* Let the VDD know that the mode changed. */
    CallVDD( VDD_POST_MODE_CHANGE );
    CallVDD( VDD_SAVE_DRIVER_STATE );

    ClearVisibleScreen();

    return( 1 );    /* All good. */
}


/* Check if the requested mode can be set. Return yes, no (with a reason),
 * or maybe.                                                           .
 * Must be exported by name, recommended ordinal 700.
 * NB: Can be called when the driver is not the current display driver.
 */
#pragma aux ValidateMode loadds;    /* __loadds not operational due to prototype in valmode.h */
UINT WINAPI __loadds ValidateMode( DISPVALMODE FAR *lpValMode )
{
    UINT        rc = VALMODE_YES;

    //dbg_printf( "ValidateMode: X=%u Y=%u bpp=%u\n", lpValMode->dvmXRes, lpValMode->dvmYRes, lpValMode->dvmBpp );
    do {
        if( !ScreenSelector ) {
#ifdef SVGA
            int svga_rc = SVGA_Init(FALSE); /* only load registers but not enabling device yet */
            
            /* Extra work if driver hasn't yet been initialized. */
            if(svga_rc != 0)
            {
              rc = VALMODE_NO_WRONGDRV;
              break;
            }
            
            dwVideoMemorySize = SVGA_ReadReg(SVGA_REG_VRAM_SIZE);
            dwPhysVRAM = SVGA_ReadReg(SVGA_REG_FB_START);
#else
            int     iChipID;

            /* Additional checks if driver isn't running. */
            iChipID = BOXV_detect( 0, &dwVideoMemorySize );
            if( !iChipID ) {
                rc = VALMODE_NO_WRONGDRV;
                break;
            }
# ifdef QEMU
            dwPhysVRAM = LfbBase;
# else
            dwPhysVRAM = BOXV_get_lfb_base( 0 );
# endif
#endif
            dbg_printf( "ValidateMode: Hardware detected, dwVideoMemorySize=%lX dwPhysVRAM=%lX\n", dwVideoMemorySize, dwPhysVRAM );
        }

        if( !IsModeOK( lpValMode->dvmXRes, lpValMode->dvmYRes, lpValMode->dvmBpp ) ) {
            rc = VALMODE_NO_NOMEM;
        }
    } while( 0 );

    //dbg_printf( "ValidateMode: rc=%u\n", rc );
    return( rc );
}

void PhysicalDisable(void)
{
#ifdef SVGA
  SVGA_Disable();
#endif
}

#pragma code_seg( _TEXT );

/* Called by the VDD in order to restore the video state when switching back
 * to the system VM. Function is in locked memory (_TEXT segment).
 * NB: Must reload DS, but apparently need not save/restore DS.
 */
#pragma aux RestoreDesktopMode loadds;
void __far RestoreDesktopMode( void )
{
    dbg_printf( "RestoreDesktopMode: %ux%u, wBpp=%u\n", wScreenX, wScreenY, wBpp );

    /* Set the current desktop mode again. */
    SetDisplayMode( wScreenX, wScreenY, 0 );

    /* Reprogram the DAC if relevant. */
    if( wBpp <= 8 ) {
        UINT    wPalCnt;

        switch( wBpp ) {
        case 8:
            wPalCnt = 256;
            break;
        case 4:
            wPalCnt = 16;
            break;
        default:
            wPalCnt = 2;
            break;
        }
        SetRAMDAC_far( 0, wPalCnt, lpColorTable );
    }

    /* Clear the busy flag. Quite important. */
    lpDriverPDevice->deFlags &= ~BUSY;

    /* Poke the VDD now that everything is restored. */
    CallVDD( VDD_SAVE_DRIVER_STATE );

    ClearVisibleScreen();
}

