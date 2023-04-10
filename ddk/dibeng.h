/* DIB Engine interface. */

#define BRUSHSIZE       8
#define VER_DIBENG      0x400
#define TYPE_DIBENG     0x5250  /* 'RP' */

#define deCursorExclude     deBeginAccess
#define deCursorUnexclude   deEndAccess

/* DIB Engine PDevice structure. The deType field will be 'DI' when GDI
 * calls the Engine; deType will be null or a selector if a minidriver is
 * calls the DIB Engine.
 */
typedef struct {
    WORD         deType;                    /* TYPE_DIBENG or zero.  */
    WORD         deWidth;                   /* DIB width in pixels. */
    WORD         deHeight;                  /* DIB height in pixels. */
    WORD         deWidthBytes;              /* Scanline length in bytes. */
    BYTE         dePlanes;                  /* Number of bit planes. */
    BYTE         deBitsPixel;               /* Number of bits per pixel. */
    DWORD        deReserved1;               /* Not used. */
    DWORD        deDeltaScan;               /* Scanline delta, can be negative.*/
    LPBYTE       delpPDevice;               /* Associated PDevice pointer. */
    DWORD        deBitsOffset;              /* 48-bit pointer to */
    WORD         deBitsSelector;            /* DIB bits storage. */
    WORD         deFlags;                   /* More flags. */
    WORD         deVersion;                 /* Major/minor (0400h = 4.0). */
    LPBITMAPINFO deBitmapInfo;              /* Bitmapinfo header pointer. */
    void         (WINAPI *deBeginAccess)(); /* Surface access begin callback. */
    void         (WINAPI *deEndAccess)();   /* Surface access end callback. */
    DWORD        deDriverReserved;          /* Reserved for minidrivers. */
} DIBENGINE, FAR *LPDIBENGINE;

/* if pdType is zero, structure is identical to PBITMAP */
typedef struct tagPDEVICE {
    short pdType;
} PDEVICE;

/* DIBEngine.deFlags */
#define MINIDRIVER      0x0001  /* Mini display driver. */
#define PALETTIZED      0x0002  /* Has a palette. */
#define SELECTEDDIB     0x0004  /* DIB Section. */
#define OFFSCREEN       0x0008  /* Offscreen surface in VRAM. */
#define BUSY            0x0010  /* Busy. */
#define NOT_FRAMEBUFFER 0x0020  /* No FB access (like 8514/A). */
#define FIVE6FIVE       0x0040  /* 5-6-5 16bpp mode. */
#define NON64KBANK      0x0080  /* Bank size != 64K. */
#define VRAM            0x8000	/* Have VRAM access. */
#define BANKEDVRAM      0x4000  /* VFlatD simulating LFB. */
#define BANKEDSCAN      0x2000  /* VFlatD with scanlines crossing banks. */
#define PALETTE_XLAT    0x1000  /* Bkgnd palette translation. */
#define VGADITHER       0x0800  /* Dither to VGA colors. */
#define CTCHANGE        0x0400  /* Color table changed/ */
#define DITHER256       0x0200  /* Dither to 256 colors. */

#define BUSY_BIT        4       /* Number of bit to test for busy. */

/* DIB_Brush??.dp??BrushFlags */
#define COLORSOLID      0x01    /* Color part solid. */
#define MONOSOLID       0x02    /* Mono part solid. */
#define PATTERNMONO     0x04    /* Brush originated from mono bitmap. */
#define MONOVALID       0x08    /* Mono part valid. */
#define MASKVALID       0x10    /* Mask valid. */
#define PRIVATEDATA     0x20    /* Vendor defined bit. */

/* Fake typedefs to avoid conflicts between windows.h and gdidefs.h. */
typedef LPVOID LPPDEVICE;
typedef LPVOID LPPPEN;
typedef LPVOID LPPBRUSH;
typedef LPVOID LPBRUSH;
typedef LPVOID LPPCOLOR;
typedef LPINT  LPSHORT;

/* DIB Engine functions. */
/* NB: Based on DDK documentation which may be inaccurate. */
//extern void     WINAPI  DIB_Control( void );
extern LONG     WINAPI  DIB_Control(LPVOID lpDevice, UINT function, LPVOID lpInput, LPVOID lpOutput);
extern WORD     WINAPI  DIB_EnumObjExt( LPPDEVICE lpDestDev, WORD wStyle, FARPROC lpCallbackFunc,
                                        LPVOID lpClientData, LPPDEVICE lpDisplayDev );
extern VOID     WINAPI  DIB_CheckCursorExt( LPPDEVICE lpDevice );
extern WORD     WINAPI  DIB_Output( LPPDEVICE lpDestDev, WORD wStyle, WORD wCount,
                                    LPPOINT lpPoints, LPPPEN lpPPen, LPPBRUSH lpPBrush,
                                    LPDRAWMODE lpDrawMode, LPRECT lpClipRect);
extern DWORD    WINAPI  DIB_RealizeObject( LPPDEVICE lpDestDev, WORD wStyle, LPVOID lpInObj,
                                           LPVOID lpOutObj, LPTEXTXFORM lpTextXForm );
extern WORD     WINAPI  DIB_RealizeObjectExt( LPPDEVICE lpDestDev, WORD wStyle, LPVOID lpInObj,
                                              LPVOID lpOutObj, LPTEXTXFORM lpTextXForm,
                                              LPPDEVICE lpDisplayDev );
extern BOOL     WINAPI  DIB_BitBlt( LPPDEVICE lpDestDev, WORD wDestX, WORD wDestY, LPPDEVICE lpSrcDev,
                                    WORD wSrcX, WORD wSrcY, WORD wXext, WORD wYext, DWORD dwRop3,
                                    LPBRUSH lpPBrush, LPDRAWMODE lpDrawMode );
extern BOOL     WINAPI  DIB_BitmapBits( LPPDEVICE lpDevice, DWORD fFlags, DWORD dwCount, LPSTR lpBits );
extern VOID     WINAPI  DIB_DibBlt( LPPDEVICE lpBitmap, WORD fGet, WORD iStart, WORD cScans, LPSTR lpDIBits,
                                    LPBITMAPINFO lpBitmapInfo, LPDRAWMODE lpDrawMode, LPINT lpTranslate );
extern WORD     WINAPI  DIB_DibToDevice( LPPDEVICE lpDestDev, WORD X, WORD Y, WORD iScan, WORD cScans,
                                         LPRECT lpClipRect, LPDRAWMODE lpDrawMode, LPSTR lpDIBits,
                                         LPBITMAPINFO lpBitmapInfo, LPINT lpTranslate );
extern DWORD    WINAPI  DIB_Pixel( LPPDEVICE lpDestDev, WORD X, WORD Y, DWORD dwPhysColor, LPDRAWMODE lpDrawMode );
extern WORD     WINAPI  DIB_ScanLR( LPPDEVICE lpDestDev, WORD X, WORD Y, DWORD dwPhysColor, WORD wStyle );
extern BOOL     WINAPI  DIB_SelectBitmap(LPPDEVICE lpDevice, LPBITMAP lpPrevBitmap, LPBITMAP lpBitmap, DWORD fFlags );
extern BOOL     WINAPI  DIB_StretchBlt( LPPDEVICE lpDestDev, WORD wDestX, WORD wDestY, WORD wDestWidth,
                                        WORD wDestHeight, LPPDEVICE lpSrcDev, WORD wSrcX, WORD wSrcY,
                                        WORD wSrcWidth, WORD wtSrcHeight, DWORD dwRop3, LPBRUSH lpPBrush,
                                        LPDRAWMODE lpDrawMode, LPRECT lpClipRect );
extern BOOL     WINAPI  DIB_StretchDIBits( LPPDEVICE lpDestDev, WORD fGet, WORD wDestX, WORD wDestY, WORD wDestWidth,
                                           WORD wDestHeight, WORD wSrcX, WORD wSrcY, WORD wSrcWidth, WORD wSrcHeight,
                                           VOID *lpBits, LPBITMAPINFO lpInfo, LPINT lpTranslate, DWORD dwRop3,
                                           LPBRUSH lpPBrush, LPDRAWMODE lpDrawMode, LPRECT lpClipRect );
extern DWORD    WINAPI  DIB_ExtTextOut( LPPDEVICE lpDestDev, WORD wDestXOrg, WORD wDestYOrg, LPRECT lpClipRect,
                                        LPSTR lpString, int wCount, LPFONTINFO lpFontInfo, LPDRAWMODE lpDrawMode,
                                        LPTEXTXFORM lpTextXForm, LPSHORT lpCharWidths, LPRECT lpOpaqueRect, WORD wOptions );
extern DWORD    WINAPI  DIB_ExtTextOutExt( LPPDEVICE lpDestDev, WORD wDestXOrg, WORD wDestYOrg, LPRECT lpClipRect,
                                           LPSTR lpString, WORD wCount, LPFONTINFO lpFontInfo, LPDRAWMODE lpDrawMode,
                                           LPTEXTXFORM lpTextXForm, LPSHORT lpCharWidths, LPRECT lpOpaqueRect,
                                           WORD wOptions, LPVOID *lpDrawTextBitmap, LPVOID *lpDrawRect );
extern WORD     WINAPI  DIB_GetCharWidth( LPPDEVICE lpDestDev, LPWORD lpBuffer, WORD wFirstChar, WORD wLastChar,
                                          LPFONTINFO lpFontInfo, LPDRAWMODE lpDrawMode, LPTEXTXFORM lpFontTrans );

extern DWORD    WINAPI  DIB_StrBlt( LPPDEVICE lpDestDev, WORD wDestXOrg, WORD wDestYOrg, LPRECT lpClipRect, LPSTR lpString,
                                    WORD wCount, LPFONTINFO lpFontInfo, LPDRAWMODE lpDrawMode, LPTEXTXFORM lpTextXForm );
extern DWORD    WINAPI  DIB_ColorInfo( LPPDEVICE lpDestDev, DWORD dwColorin, LPPCOLOR lpPColor );
extern VOID     WINAPI  DIB_GetPalette( WORD nStartIndex, WORD nNumEntries, RGBQUAD lpPalette );
extern VOID     WINAPI  DIB_GetPaletteExt( WORD nStartIndex, WORD nNumEntries, RGBQUAD lpPalette, LPPDEVICE lpDIBEngine );
extern VOID     WINAPI  DIB_GetPaletteTranslate( LPWORD lpIndexes );
extern VOID     WINAPI  DIB_GetPaletteTranslateExt( LPWORD lpIndexes, LPPDEVICE lpDIBEngine );
extern VOID     WINAPI  DIB_SetPalette( WORD nStartIndex, WORD nNumEntries, LPVOID lpPalette );
extern VOID     WINAPI  DIB_SetPaletteExt( WORD nStartIndex, WORD nNumEntries, LPVOID lpPalette, LPPDEVICE lpDIBEngine );
extern VOID     WINAPI  DIB_SetPaletteTranslate( LPWORD lpIndexes );
extern VOID     WINAPI  DIB_SetPaletteTranslateExt( LPWORD lpIndexes, LPPDEVICE lpDIBEngine );
extern VOID     WINAPI  DIB_UpdateColorsExt( WORD wStartX, WORD wStart, WORD wExtX, WORD wExtY,
                                             LPWORD lpTranslate, LPPDEVICE lpDIBEngine );
extern VOID     WINAPI  DIB_SetCursorExt( LPPDEVICE lpDevice, LPVOID lpCursorShape );
extern VOID     WINAPI  DIB_CheckCursorExt( LPPDEVICE lpDevice );
extern VOID     WINAPI  DIB_MoveCursorExt( LPPDEVICE lpDevice, WORD absX, WORD absY );
extern VOID     WINAPI  DIB_Inquire( LPVOID lpCursorInfo );
extern VOID     WINAPI  DIB_BeginAccess( LPPDEVICE lpDevice, WORD wLeft, WORD wTop, WORD wRight, WORD wBottom, WORD wFlags );
extern VOID     WINAPI  DIB_EndAccess( LPPDEVICE lpDevice, WORD wFlags );
extern WORD     WINAPI  DIB_CreateDIBitmap( void );
extern WORD     WINAPI  DIB_DeviceBitmap( LPPDEVICE lpDestDev, WORD wCommand, LPBITMAP lpBitmap, LPSTR lpBits );
extern WORD     WINAPI  DIB_DeviceMode( HWND hWnd, HINSTANCE hInst, LPVOID lpDeviceType, LPVOID lpOutputFile );
extern UINT     WINAPI  DIB_Enable( LPPDEVICE lpDevice, WORD wStyle, LPSTR lpDeviceType, LPSTR lpOutputFile, LPVOID lpStuff );
extern VOID     WINAPI  DIB_Disable( LPPDEVICE lpDestDev );
extern WORD     WINAPI  DIB_EnumDFonts( LPPDEVICE lpDestDev, LPSTR lpFaceName, FARPROC lpCallbackFunc, LPVOID lpClientData );
extern WORD     WINAPI  DIB_SetAttribute( LPPDEVICE lpDevice, WORD wStateNum, WORD wIndex, DWORD dwAttribute );


/* WARNING: CreateDIBPDevice returns the result in EAX, not DX:AX! */
extern DWORD WINAPI CreateDIBPDevice( LPBITMAPINFO lpInfo, LPPDEVICE lpDevice, LPVOID lpBits, WORD wFlags );

#define FB_ACCESS	    0x0001
#define CURSOREXCLUDE	0x0008

#define GREY_BIT        0x40    /* Physical color MSB. */

/* DIB Engine Color Table entry. A lot like RGBQUAD. */
typedef struct {
    BYTE    dceBlue;
    BYTE    dceGreen;
    BYTE    dceRed;
    BYTE    dceFlags;
} DIBColorEntry;

/* DIBColorEntry.dceFlags */
#define NONSTATIC       0x80	  
#define MAPTOWHITE      0x01	  


/* DIB Engine Physical Object Definitions */

typedef struct {
    WORD    dpPenStyle;
    BYTE    dpPenFlags;
    BYTE    dpPenBpp;
    DWORD   dpPenMono;
    DWORD   dpPenColor;
} DIB_Pen;

typedef struct {
    BYTE    dp1BrushFlags;              /* Accelerator for solids            */
    BYTE    dp1BrushBpp;                /* Brush Bits per pixel format       */
    WORD    dp1BrushStyle;              /* Style of the brush                */
    DWORD   dp1FgColor;                 /* Physical fg color                 */
    WORD    dp1Hatch;                   /* Hatching style                    */
    DWORD   dp1BgColor;                 /* Physical bg color                 */
    BYTE    dp1BrushMono[BRUSHSIZE*4];  /* Mono portion                      */
    BYTE    dp1BrushMask[BRUSHSIZE*4];  /* transparency mask (hatch pattern) */
    BYTE    dp1BrushBits[BRUSHSIZE*4];  /* 8 rows, 8 columns of 1 bit/pixel  */
} DIB_Brush1;

typedef struct {
    BYTE    dp4BrushFlags;              /* Accelerator for solids            */
    BYTE    dp4BrushBpp;                /* Brush Bits per pixel format       */
    WORD    dp4BrushStyle;              /* Style of the brush                */
    DWORD   dp4FgColor;                 /* Physical fg color                 */
    WORD    dp4Hatch;                   /* Hatching style                    */
    DWORD   dp4BgColor;                 /* Physical bg color                 */
    BYTE    dp4BrushMono[BRUSHSIZE*4];  /* Mono portion                      */
    BYTE    dp4BrushMask[BRUSHSIZE*4];  /* transparency mask (hatch pattern) */
    BYTE    dp4BrushBits[BRUSHSIZE*4];  /* 8 rows, 8 columns of 4 bit/pixel  */
} DIB_Brush4;

typedef struct {
    BYTE    dp8BrushFlags;              /* Accelerator for solids            */
    BYTE    dp8BrushBpp;                /* Brush Bits per pixel format       */
    WORD    dp8BrushStyle;              /* Style of the brush                */
    DWORD   dp8FgColor;                 /* Physical fg color                 */
    WORD    dp8Hatch;                   /* Hatching style                    */
    DWORD   dp8BgColor;                 /* Physical bg color                 */
    BYTE    dp8BrushMono[BRUSHSIZE*4];  /* Mono portion                      */
    BYTE    dp8BrushMask[BRUSHSIZE*4];  /* transparency mask (hatch pattern) */
    BYTE    dp8BrushBits[BRUSHSIZE*8];  /* 8 rows,8 columns of 8 bit/pixel   */
} DIB_Brush8;

typedef struct {
    BYTE    dp16BrushFlags;             /* Accelerator for solids            */
    BYTE    dp16BrushBpp;               /* Brush Bits per pixel format       */
    WORD    dp16BrushStyle;             /* Style of the brush                */
    DWORD   dp16FgColor;                /* Physical fg color                 */
    WORD    dp16Hatch;                  /* Hatching style                    */
    DWORD   dp16BgColor;                /* Physical bg color                 */
    BYTE    dp16BrushMono[BRUSHSIZE*4]; /* Mono portion                      */
    BYTE    dp16BrushMask[BRUSHSIZE*4]; /* transparency mask (hatch pattern) */
    BYTE    dp16BrushBits[BRUSHSIZE*16];/* 8 rows,8 columns of 16 bit/pixel  */
} DIB_Brush16;

typedef struct {
    BYTE    dp24BrushFlags;             /* Accelerator for solids            */
    BYTE    dp24BrushBpp;               /* Brush Bits per pixel format       */
    WORD    dp24BrushStyle;             /* Style of the brush                */
    DWORD   dp24FgColor;                /* Physical fg color                 */
    WORD    dp24Hatch;                  /* Hatching style                    */
    DWORD   dp24BgColor;                /* Physical bg color                 */
    BYTE    dp24BrushMono[BRUSHSIZE*4]; /* Mono portion                      */
    BYTE    dp24BrushMask[BRUSHSIZE*4]; /* transparency mask (hatch pattern) */
    BYTE    dp24BrushBits[BRUSHSIZE*24];/* 8 rows,8 columns of 24 bit/pixel  */
} DIB_Brush24;

typedef struct {
    BYTE    dp32BrushFlags;             /* Accelerator for solids            */
    BYTE    dp32BrushBpp;               /* Brush Bits per pixel format       */
    WORD    dp32BrushStyle;             /* Style of the brush                */
    DWORD   dp32FgColor;                /* Physical fg color                 */
    WORD    dp32Hatch;                  /* Hatching style                    */
    DWORD   dp32BgColor;                /* Physical bg color                 */
    BYTE    dp32BrushMono[BRUSHSIZE*4]; /* Mono portion                      */
    BYTE    dp32BrushMask[BRUSHSIZE*4]; /* transparency mask (hatch pattern) */
    BYTE    dp32BrushBits[BRUSHSIZE*32];/* 8 rows,8 columns of 32 bit/pixel  */
} DIB_Brush32;
