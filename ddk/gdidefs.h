
/* Definitions for GDI drivers. */

/* Physical Bitmap structure. */
typedef struct {
    short int           bmType;
    unsigned short int  bmWidth;
    unsigned short int  bmHeight;
    unsigned short int  bmWidthBytes;
    BYTE                bmPlanes;
    BYTE                bmBitsPixel;
    BYTE FAR            *bmBits;
    unsigned long int   bmWidthPlanes;
    BYTE FAR            *bmlpPDevice;
    unsigned short int  bmSegmentIndex;
    unsigned short int  bmScanSegment;
    unsigned short int  bmFillBytes;
    unsigned short int  futureUse4;
    unsigned short int  futureUse5;
} BITMAP;

/* DIB structs also defined in windows.h. */
typedef struct {
    DWORD   bcSize;
    WORD    bcWidth;
    WORD    bcHeight;
    WORD    bcPlanes;
    WORD    bcBitCount;
} BITMAPCOREHEADER;
typedef BITMAPCOREHEADER FAR *LPBITMAPCOREHEADER;
typedef BITMAPCOREHEADER *PBITMAPCOREHEADER;

typedef struct {
    DWORD   biSize;
    DWORD   biWidth;
    DWORD   biHeight;
    WORD    biPlanes;
    WORD    biBitCount;
    DWORD   biCompression;
    DWORD   biSizeImage;
    DWORD   biXPelsPerMeter;
    DWORD   biYPelsPerMeter;
    DWORD   biClrUsed;
    DWORD   biClrImportant;
} BITMAPINFOHEADER;

typedef BITMAPINFOHEADER FAR *LPBITMAPINFOHEADER;
typedef BITMAPINFOHEADER *PBITMAPINFOHEADER;

typedef struct {
    BYTE    rgbtBlue;
    BYTE    rgbtGreen;
    BYTE    rgbtRed;
} RGBTRIPLE;

typedef struct {
    BYTE    rgbBlue;
    BYTE    rgbGreen;
    BYTE    rgbRed;
    BYTE    rgbReserved;
} RGBQUAD;

/* ICM Color Definitions */
typedef long    FXPT16DOT16, FAR *LPFXPT16DOT16;
typedef long    FXPT2DOT30,  FAR *LPFXPT2DOT30;

typedef struct tagCIEXYZ
{
    FXPT2DOT30  ciexyzX;
    FXPT2DOT30  ciexyzY;
    FXPT2DOT30  ciexyzZ;
} CIEXYZ;
typedef CIEXYZ  FAR *LPCIEXYZ;

typedef struct tagICEXYZTRIPLE
{
    CIEXYZ  ciexyzRed;
    CIEXYZ  ciexyzGreen;
    CIEXYZ  ciexyzBlue;
} CIEXYZTRIPLE;
typedef CIEXYZTRIPLE FAR    *LPCIEXYZTRIPLE;

typedef struct { 
    BITMAPCOREHEADER    bmciHeader;
    RGBQUAD             bmciColors[1];
} BITMAPCOREINFO;

typedef BITMAPCOREINFO FAR *LPBITMAPCOREINFO;
typedef BITMAPCOREINFO *PBITMAPCOREINFO;

typedef struct { 
    BITMAPINFOHEADER    bmiHeader;
    RGBQUAD             bmiColors[1];
} BITMAPINFO;

typedef BITMAPINFO FAR *LPBITMAPINFO;
typedef BITMAPINFO *PBITMAPINFO;

typedef struct {
    DWORD           bV4Size;
    LONG            bV4Width;
    LONG            bV4Height;
    WORD            bV4Planes;
    WORD            bV4BitCount;
    DWORD           bV4V4Compression;
    DWORD           bV4SizeImage;
    LONG            bV4XPelsPerMeter;
    LONG            bV4YPelsPerMeter;
    DWORD           bV4ClrUsed;
    DWORD           bV4ClrImportant;
    DWORD           bV4RedMask;
    DWORD           bV4GreenMask;
    DWORD           bV4BlueMask;
    DWORD           bV4AlphaMask;
    DWORD           bV4CSType;
    CIEXYZTRIPLE    bV4Endpoints;
    DWORD           bV4GammaRed;
    DWORD           bV4GammaGreen;
    DWORD           bV4GammaBlue;
} BITMAPV4HEADER, FAR *LPBITMAPV4HEADER, *PBITMAPV4HEADER;

typedef struct { 
    BITMAPV4HEADER  bmv4Header;
    RGBQUAD         bmv4Colors[1];
} BITMAPV4INFO;

typedef BITMAPV4INFO FAR *LPBITMAPV4INFO;
typedef BITMAPV4INFO *PBITMAPV4INFO;

/* currently, if the low byte of biCompression is non zero, 
 * it must be one of following */

#define BI_RGB              0x00
#define BI_RLE8             0x01
#define BI_RLE4             0x02
#define BI_BITFIELDS        0x03

#define BITMAP_SELECTED     0x01
#define BITMAP_64K          0x01

#define DIBSIGNATURE        0x4944

/* Point types are optional. */
#ifndef NOPTRC

typedef     struct {
    short int xcoord;
    short int ycoord;
} PTTYPE;
typedef PTTYPE *PPOINT;
typedef PTTYPE FAR *LPPOINT;

#define     POINT   PTTYPE

typedef struct {
    short int   left;
    short int   top;
    short int   right;
    short int   bottom;
} RECT;
typedef RECT    *PRECT;

#endif

typedef struct {
    PTTYPE  min;
    PTTYPE  ext;
} BOXTYPE;
typedef RECT FAR    *LPRECT;

/* Object definitions used by GDI support routines written in C */

#define OBJ_PEN         1
#define OBJ_BRUSH       2
#define OBJ_FONT        3

typedef struct {
    unsigned short int  lbStyle;
    unsigned long int   lbColor;
    unsigned short int  lbHatch;
    unsigned long int   lbBkColor;
    unsigned long int   lbhcmXform;
} LOGBRUSH;

#define lbPattern       lbColor

/* Brush Style definitions. */
#define     BS_SOLID            0
#define     BS_HOLLOW           1
#define     BS_HATCHED          2
#define     BS_PATTERN          3

#define     MaxBrushStyle       3

/* Hatch Style definitions. */
#define     HS_HORIZONTAL       0       /* ----- */
#define     HS_VERTICAL         1       /* ||||| */
#define     HS_FDIAGONAL        2       /* ///// */
#define     HS_BDIAGONAL        3       /* \\\\\ */
#define     HS_CROSS            4       /* +++++ */
#define     HS_DIAGCROSS        5       /* xxxxx */

#define     MaxHatchStyle       5


/* Logical Pen Structure. */
typedef struct {
    unsigned short int lopnStyle;
    PTTYPE             lopnWidth;
    unsigned long int  lopnColor;
    unsigned short int lopnStyle2;
    unsigned long int  lopnhcmXform;
} LOGPEN;

/* Line Style definitions. */
#define     LS_SOLID            0
#define     LS_DASHED           1
#define     LS_DOTTED           2
#define     LS_DOTDASHED        3
#define     LS_DASHDOTDOT       4
#define     LS_NOLINE           5
#define     LS_INSIDEFRAME      6
#define     MaxLineStyle        LS_NOLINE

#define     LS_ENDCAP_FLAT      0x01
#define     LS_ENDCAP_ROUND     0x02
#define     LS_ENDCAP_SQUARE    0x04
#define     LS_JOIN_BEVEL       0x08
#define     LS_JOIN_MITER       0x10
#define     LS_JOIN_ROUND       0x20


/* The size to allocate for the lfFaceName field in the logical font. */
#ifndef     LF_FACESIZE
#define     LF_FACESIZE     32
#endif

/* Various constants for defining a logical font. */
#define OUT_DEFAULT_PRECIS      0
#define OUT_STRING_PRECIS       1
#define OUT_CHARACTER_PRECIS    2
#define OUT_STROKE_PRECIS       3
#define OUT_TT_PRECIS           4
#define OUT_DEVICE_PRECIS       5
#define OUT_RASTER_PRECIS       6
#define OUT_TT_ONLY_PRECIS      7

#define CLIP_DEFAULT_PRECIS     0
#define CLIP_CHARACTER_PRECIS   1
#define CLIP_STROKE_PRECIS      2
#define CLIP_MASK               0x0F
#define CLIP_LH_ANGLES          0x10
#define CLIP_TT_ALWAYS          0x20
#define CLIP_EMBEDDED           0x80

#define DEFAULT_QUALITY         0
#define DRAFT_QUALITY           1
#define PROOF_QUALITY           2

#define DEFAULT_PITCH           0
#define FIXED_PITCH             1
#define VARIABLE_PITCH          2

#define ANSI_CHARSET            0
#define DEFAULT_CHARSET         1
#define SYMBOL_CHARSET          2
#define MAC_CHARSET             77
#define SHIFTJIS_CHARSET        128
#define HANGEUL_CHARSET         129
#define CHINESEBIG5_CHARSET     136
#define OEM_CHARSET             255


/*      GDI font families.                                              */
#define FF_DONTCARE     (0<<4)  /* Don't care or don't know.            */
#define FF_ROMAN        (1<<4)  /* Variable stroke width, serifed.      */
                                /* Times Roman, Century Schoolbook, etc.*/
#define FF_SWISS        (2<<4)  /* Variable stroke width, sans-serifed. */
                                /* Helvetica, Swiss, etc.               */
#define FF_MODERN       (3<<4)  /* Constant stroke width, serifed or sans-serifed. */
                                /* Pica, Elite, Courier, etc.           */
#define FF_SCRIPT       (4<<4)  /* Cursive, etc.                        */
#define FF_DECORATIVE   (5<<4)  /* Old English, etc.                    */


/* Font weights lightest to heaviest. */
#define FW_DONTCARE             0
#define FW_THIN                 100
#define FW_EXTRALIGHT           200
#define FW_LIGHT                300
#define FW_NORMAL               400
#define FW_MEDIUM               500
#define FW_SEMIBOLD             600
#define FW_BOLD                 700
#define FW_EXTRABOLD            800
#define FW_HEAVY                900

#define FW_ULTRALIGHT           FW_EXTRALIGHT
#define FW_REGULAR              FW_NORMAL
#define FW_DEMIBOLD             FW_SEMIBOLD
#define FW_ULTRABOLD            FW_EXTRABOLD
#define FW_BLACK                FW_HEAVY

/* Enumeration font types. */
#define     RASTER_FONTTYPE         1
#define     DEVICE_FONTTYPE         2



typedef     struct  {
    short int   lfHeight;
    short int   lfWidth;
    short int   lfEscapement;
    short int   lfOrientation;
    short int   lfWeight;
    BYTE        lfItalic;
    BYTE        lfUnderline;
    BYTE        lfStrikeOut;
    BYTE        lfCharSet;
    BYTE        lfOutPrecision;
    BYTE        lfClipPrecision;
    BYTE        lfQuality;
    BYTE        lfPitchAndFamily;
    BYTE        lfFaceName[LF_FACESIZE];
} LOGFONT;


#define     InquireInfo     0x01        /* Inquire Device GDI Info         */
#define     EnableDevice    0x00        /* Enable Device                   */
#define     InfoContext     0x8000      /* Inquire/Enable for info context */

/* Device Technology types */
#define     DT_PLOTTER          0       /* Vector plotter          */
#define     DT_RASDISPLAY       1       /* Raster display          */
#define     DT_RASPRINTER       2       /* Raster printer          */
#define     DT_RASCAMERA        3       /* Raster camera           */
#define     DT_CHARSTREAM       4       /* Character-stream, PLP   */
#define     DT_METAFILE         5       /* Metafile, VDM           */
#define     DT_DISPFILE         6       /* Display-file            */
#define     DT_JUMBO            11      /* SPAG LJ cool thing      */

/* Curve Capabilities */
#define     CC_NONE         0x0000      /* Curves not supported    */
#define     CC_CIRCLES      0x0001      /* Can do circles          */
#define     CC_PIE          0x0002      /* Can do pie wedges       */
#define     CC_CHORD        0x0004      /* Can do chord arcs       */
#define     CC_ELLIPSES     0x0008      /* Can do ellipese         */
#define     CC_WIDE         0x0010      /* Can do wide lines       */
#define     CC_STYLED       0x0020      /* Can do styled lines     */
#define     CC_WIDESTYLED   0x0040      /* Can do wide styled lines*/
#define     CC_INTERIORS    0x0080      /* Can do interiors        */
#define     CC_ROUNDRECT    0x0100      /* Can do round rectangles */
#define     CC_POLYBEZIER   0x0200      /* Can do polybeziers      */

/* Line Capabilities */
#define     LC_NONE         0x0000      /* Lines not supported     */
#define     LC_POLYSCANLINE 0x0001      /* Poly Scanlines supported*/
#define     LC_POLYLINE     0x0002      /* Can do polylines        */
#define     LC_MARKER       0x0004      /* Can do markers          */
#define     LC_POLYMARKER   0x0008      /* Can do polymarkers      */
#define     LC_WIDE         0x0010      /* Can do wide lines       */
#define     LC_STYLED       0x0020      /* Can do styled lines     */
#define     LC_WIDESTYLED   0x0040      /* Can do wide styled lines*/
#define     LC_INTERIORS    0x0080      /* Can do interiors        */

/* Polygonal Capabilities */
#define     PC_NONE         0x0000      /* Polygonals not supported*/
#define     PC_ALTPOLYGON   0x0001      /* Can do even odd polygons*/
#define     PC_POLYGON      0x0001      /* old name for ALTPOLYGON */
#define     PC_RECTANGLE    0x0002      /* Can do rectangles       */
#define     PC_WINDPOLYGON  0x0004      /* Can do winding polygons */
#define     PC_TRAPEZOID    0x0004      /* old name for WINDPOLYGON*/
#define     PC_SCANLINE     0x0008      /* Can do scanlines        */
#define     PC_WIDE         0x0010      /* Can do wide borders     */
#define     PC_STYLED       0x0020      /* Can do styled borders   */
#define     PC_WIDESTYLED   0x0040      /* Can do wide styled borders*/
#define     PC_INTERIORS    0x0080      /* Can do interiors        */
#define     PC_POLYPOLYGON  0x0100      /* Can do PolyPolygons     */

/* Clipping Capabilities */
#define     CP_NONE         0x0000      /* no clipping of Output   */
#define     CP_RECTANGLE    0x0001      /* Output clipped to Rects */
#define     CP_REGION       0x0002      /* not supported           */
#define     CP_REGION32     0x0004      /* Output clipped to regions */

/* Text Capabilities */
#define TC_OP_CHARACTER 0x0001          /* Can do OutputPrecision    CHARACTER      */
#define TC_OP_STROKE    0x0002          /* Can do OutputPrecision    STROKE         */
#define TC_CP_STROKE    0x0004          /* Can do ClipPrecision      STROKE         */
#define TC_CR_90        0x0008          /* Can do CharRotAbility     90             */
#define TC_CR_ANY       0x0010          /* Can do CharRotAbility     ANY            */
#define TC_SF_X_YINDEP  0x0020          /* Can do ScaleFreedom       X_YINDEPENDENT */
#define TC_SA_DOUBLE    0x0040          /* Can do ScaleAbility       DOUBLE         */
#define TC_SA_INTEGER   0x0080          /* Can do ScaleAbility       INTEGER        */
#define TC_SA_CONTIN    0x0100          /* Can do ScaleAbility       CONTINUOUS     */
#define TC_EA_DOUBLE    0x0200          /* Can do EmboldenAbility    DOUBLE         */
#define TC_IA_ABLE      0x0400          /* Can do ItalisizeAbility   ABLE           */
#define TC_UA_ABLE      0x0800          /* Can do UnderlineAbility   ABLE           */
#define TC_SO_ABLE      0x1000          /* Can do StrikeOutAbility   ABLE           */
#define TC_RA_ABLE      0x2000          /* Can do RasterFontAble     ABLE           */
#define TC_VA_ABLE      0x4000          /* Can do VectorFontAble     ABLE           */
#define TC_RESERVED     0x8000          /* Reserved. Must be returned zero.        */

/* Raster Capabilities */
#define RC_NONE         0x0000          /* No Raster Capabilities       */
#define RC_BITBLT       0x0001          /* Can do bitblt                */
#define RC_BANDING      0x0002          /* Requires banding support     */
#define RC_SCALING      0x0004          /* does scaling while banding   */
#define RC_BITMAP64     0x0008          /* supports >64k bitmaps        */
#define RC_GDI20_OUTPUT 0x0010          /* has 2.0 output calls         */
#define RC_GDI20_STATE  0x0020          /* dc has a state block         */
#define RC_SAVEBITMAP   0x0040          /* can save bitmaps locally     */
#define RC_DI_BITMAP    0x0080          /* can do DIBs                  */
#define RC_PALETTE      0x0100          /* can do color pal management  */
#define RC_DIBTODEV     0x0200          /* can do SetDIBitsToDevice     */
#define RC_BIGFONT      0x0400          /* can do BIGFONTs              */
#define RC_STRETCHBLT   0x0800          /* can do StretchBlt            */
#define RC_FLOODFILL    0x1000          /* can do FloodFill             */
#define RC_STRETCHDIB   0x2000          /* can do StretchDIBits         */
#define RC_OP_DX_OUTPUT 0x4000          /* can do smart ExtTextOut w/dx */
#define RC_DEVBITS      0x8000          /* supports device bitmaps      */

/* DC Management Flags */
#define DC_SPDevice     0000001     /* Seperate PDevice required per device/filename */
#define DC_1PDevice     0000002     /* Only 1 PDevice allowed per device/filename    */
#define DC_IgnoreDFNP   0000004     /* Ignore device/filename pairs when matching    */

/* dpCaps1 capability bits  */
#define C1_TRANSPARENT  0x0001      /* supports transparency                */
#define TC_TT_ABLE      0x0002      /* can do TT through DDI or brute       */
#define C1_TT_CR_ANY    0x0004      /* can do rotated TT fonts              */
#define C1_EMF_COMPLIANT 0x0008     /* Win95 - supports metafile spooling   */
#define C1_DIBENGINE    0x0010      /* DIB Engine compliant driver          */
#define C1_GAMMA_RAMP   0x0020      /* supports gamma ramp setting          */
#define C1_ICM          0x0040      /* does some form of ICM support        */
#define C1_REINIT_ABLE  0x0080      /* Driver supports ReEnable             */
#define C1_GLYPH_INDEX  0x0100      /* Driver supports glyph index fonts    */
#define C1_BIT_PACKED   0x0200      /* Supports bit-packed glyphs           */
#define C1_BYTE_PACKED  0x0400      /* Supports byte-packed glyphs          */
#define C1_COLORCURSOR  0x0800      /* Driver supports color_cursors and async SetCursor */
#define C1_CMYK_ABLE    0x1000      /* Driver supports CMYK ColorRefs       */
#define C1_SLOW_CARD    0x2000      /* Little or no acceleration (VGA, etc.)*/

/* dpCapsFE capability bits */
#define FEC_TT_DBCS     0x0020      /* can output DBCS TT fonts correctly   */
#define FEC_WIFE_ABLE   0x0080      /* can handle WIFE font as Engine font  */

typedef struct {
    short int           dpVersion;
    short int           dpTechnology;
    short int           dpHorzSize;
    short int           dpVertSize;
    short int           dpHorzRes;
    short int           dpVertRes;
    short int           dpBitsPixel;
    short int           dpPlanes;
    short int           dpNumBrushes;
    short int           dpNumPens;
    short int           dpCapsFE;
    short int           dpNumFonts;
    short int           dpNumColors;
    short int           dpDEVICEsize;
    unsigned short int  dpCurves;
    unsigned short int  dpLines;
    unsigned short int  dpPolygonals;
    unsigned short int  dpText;
    unsigned short int  dpClip;
    unsigned short int  dpRaster;
    short int           dpAspectX;
    short int           dpAspectY;
    short int           dpAspectXY;
    short int           dpStyleLen;
    PTTYPE              dpMLoWin;
    PTTYPE              dpMLoVpt;
    PTTYPE              dpMHiWin;
    PTTYPE              dpMHiVpt;
    PTTYPE              dpELoWin;
    PTTYPE              dpELoVpt;
    PTTYPE              dpEHiWin;
    PTTYPE              dpEHiVpt;
    PTTYPE              dpTwpWin;
    PTTYPE              dpTwpVpt;
    short int           dpLogPixelsX;
    short int           dpLogPixelsY;
    short int           dpDCManage;
    unsigned short int  dpCaps1;
    short int           futureuse4;
    short int           futureuse5;
    short int           futureuse6;
    short int           futureuse7;
    WORD                dpNumPalReg;
    WORD                dpPalReserved;
    WORD                dpColorRes;
} GDIINFO;

/* This bit in the dfType field signals that the dfBitsOffset field is an
   absolute memory address and should not be altered. */
#define PF_BITS_IS_ADDRESS  4

/* This bit in the dfType field signals that the font is device realized. */
#define PF_DEVICE_REALIZED  0x80

/* These bits in the dfType give the fonttype -
       raster, vector, other1, other2. */
#define PF_RASTER_TYPE      0
#define PF_VECTOR_TYPE      1
#define PF_OTHER1_TYPE      2
#define PF_OTHER2_TYPE      3
#define PF_GLYPH_INDEX   0x20
#define PF_WIFE_TYPE     0x08

/* Glyph types for EngineGetGlyphBmp */
#define EGB_BITMAP          1
#define EGB_OUTLINE         2
#define EGB_GRAY2_BITMAP    8
#define EGB_GRAY4_BITMAP    9
#define EGB_GRAY8_BITMAP   10


/* The size to allocate for the dfMaps field in the physical font. */
#ifndef     DF_MAPSIZE
#define     DF_MAPSIZE      1
#endif

/* Font structure. */
typedef     struct  {
    short int           dfType;
    short int           dfPoints;
    short int           dfVertRes;
    short int           dfHorizRes;
    short int           dfAscent;
    short int           dfInternalLeading;
    short int           dfExternalLeading;
    BYTE                dfItalic;
    BYTE                dfUnderline;
    BYTE                dfStrikeOut;
    short int           dfWeight;
    BYTE                dfCharSet;
    short int           dfPixWidth;
    short int           dfPixHeight;
    BYTE                dfPitchAndFamily;
    short int           dfAvgWidth;
    short int           dfMaxWidth;
    BYTE                dfFirstChar;
    BYTE                dfLastChar;
    BYTE                dfDefaultChar;
    BYTE                dfBreakChar;
    short int           dfWidthBytes;
    unsigned long int   dfDevice;
    unsigned long int   dfFace;
    unsigned long int   dfBitsPointer;
    unsigned long int   dfBitsOffset;
    BYTE                dfReservedByte;
    unsigned short      dfMaps[DF_MAPSIZE];
} FONTINFO;


typedef struct {
    short int           erType;
    short int           erPoints;
    short int           erVertRes;
    short int           erHorizRes;
    short int           erAscent;
    short int           erInternalLeading;
    short int           erExternalLeading;
    BYTE                erItalic;
    BYTE                erUnderline;
    BYTE                erStrikeOut;
    short int           erWeight;
    BYTE                erCharSet;
    short int           erPixWidth;
    short int           erPixHeight;
    BYTE                erPitchAndFamily;
    short int           erAvgWidth;
    short int           erMaxWidth;
    BYTE                erFirstChar;
    BYTE                erLastChar;
    BYTE                erDefaultChar;
    BYTE                erBreakChar;
    short int           erWidthBytes;
    unsigned long int   erDevice;
    unsigned long int   erFace;
    unsigned long int   erBitsPointer;
    unsigned long int   erBitsOffset;
    BYTE                erReservedByte;
    short int           erUnderlinePos;
    short int           erUnderlineThick;
    short int           erStrikeoutPos;
    short int           erStrikeoutThick;
} SCALABLEFONTINFO;


typedef struct {
    short int           ftHeight;
    short int           ftWidth;
    short int           ftEscapement;
    short int           ftOrientation;
    short int           ftWeight;
    BYTE                ftItalic;
    BYTE                ftUnderline;
    BYTE                ftStrikeOut;
    BYTE                ftOutPrecision;
    BYTE                ftClipPrecision;
    unsigned short int  ftAccelerator;
    short int           ftOverhang;
} TEXTXFORM;


typedef struct {
    short int   tmHeight;
    short int   tmAscent;
    short int   tmDescent;
    short int   tmInternalLeading;
    short int   tmExternalLeading;
    short int   tmAveCharWidth;
    short int   tmMaxCharWidth;
    short int   tmWeight;
    BYTE        tmItalic;
    BYTE        tmUnderlined;
    BYTE        tmStruckOut;
    BYTE        tmFirstChar;
    BYTE        tmLastChar;
    BYTE        tmDefaultChar;
    BYTE        tmBreakChar;
    BYTE        tmPitchAndFamily;
    BYTE        tmCharSet;
    short int   tmOverhang;
    short int   tmDigitizedAspectX;
    short int   tmDigitizedAspectY;
} TEXTMETRIC;

typedef struct {
    short int           Rop2;
    short int           bkMode;
    unsigned long int   bkColor;
    unsigned long int   TextColor;
    short int           TBreakExtra;
    short int           BreakExtra;
    short int           BreakErr;
    short int           BreakRem;
    short int           BreakCount;
    short int           CharExtra;
    unsigned long int   LbkColor;
    unsigned long int   LTextColor;
    DWORD               ICMCXform;
    short               StretchBltMode;
    DWORD               eMiterLimit;
} DRAWMODE;


/* Background Mode definitions. */
#define TRANSPARENT             1
#define OPAQUE                  2

#define BKMODE_TRANSPARENT      1
#define BKMODE_OPAQUE           2
#define BKMODE_LEVEL1           3
#define BKMODE_LEVEL2           4
#define BKMODE_LEVEL3           5
#define BKMODE_TRANSLATE        6

/* StretchBlt Mode definitions. */
#define STRETCH_ANDSCANS        1
#define STRETCH_ORSCANS         2
#define STRETCH_DELETESCANS     3
#define STRETCH_HALFTONE        4

#define SBM_BLACKONWHITE        STRETCH_ANDSCANS
#define SBM_WHITEONBLACK        STRETCH_ORSCANS
#define SBM_COLORONCOLOR        STRETCH_DELETESCANS
#define SBM_HALFTONE            STRETCH_HALFTONE

typedef struct {
    short int   scnPntCnt;
    short int   scnPntTop;
    short int   scnPntBottom;
    short int   scnPntX[2];
    short int   scnPntCntToo;
} SCAN, FAR* LPSCAN;

typedef struct {
    DWORD   cbSize;
    LPVOID  lpDestDev;
    DWORD   nEscape;
    DWORD   cbInput;
    LPVOID  lpInput;
    POINT   ptOrigin;
    DWORD   dwUniq;
    RECT    rcBBox;
    DWORD   cScans;
    LPSCAN  lpScan;
} DRAWESCAPE, FAR* LPDRAWESCAPE;

typedef struct {
    WORD    id;
    WORD    cbSize;
    LPRECT  lprcClip;
    DWORD   dwUniq;
    RECT    rcBBox;
    DWORD   cScans;
    LPSCAN  lpScan;
} REGION, FAR* LPREGION;


/* Output Style definitions. */

#define OS_POLYBEZIER       1
#define OS_ARC              3
#define OS_SCANLINES        4
#define OS_POLYSCANLINE     5
#define OS_RECTANGLE        6
#define OS_ELLIPSE          7
#define OS_MARKER           8
#define OS_POLYLINE         18
#define OS_TRAPEZOID        20
#define OS_WINDPOLYGON      OS_TRAPEZOID
#define OS_POLYGON          22
#define OS_ALTPOLYGON       OS_POLYGON
#define OS_PIE              23
#define OS_POLYMARKER       24
#define OS_CHORD            39
#define OS_CIRCLE           55

#define OS_POLYPOLYGON      0x4000  /* ORed with OS_WIND/ALTPOLYGON. */

#define OS_BEGINNSCAN       80
#define OS_ENDNSCAN         81

#define OEM_FAILED          0x80000000L

#define NEWFRAME                     1
#define ABORTDOC                     2
#define NEXTBAND                     3
#define SETCOLORTABLE                4
#define GETCOLORTABLE                5
#define FLUSHOUTPUT                  6
#define DRAFTMODE                    7
#define QUERYESCSUPPORT              8
#define SETPRINTERDC                 9          /* DDK: between GDI and Driver. */
#define SETABORTPROC                 9          /* SDK: between application and GDI. */
#define STARTDOC                     10
#define ENDDOC                       11
#define GETPHYSPAGESIZE              12
#define GETPRINTINGOFFSET            13
#define GETSCALINGFACTOR             14
#define MFCOMMENT                    15
#define GETPENWIDTH                  16
#define SETCOPYCOUNT                 17
#define SELECTPAPERSOURCE            18
#define DEVICEDATA                   19
#define PASSTHROUGH                  19
#define GETTECHNOLGY                 20
#define GETTECHNOLOGY                20
#define SETLINECAP                   21
#define SETLINEJOIN                  22
#define SETMITERLIMIT                23
#define BANDINFO                     24
#define DRAWPATTERNRECT              25
#define GETVECTORPENSIZE             26
#define GETVECTORBRUSHSIZE           27
#define ENABLEDUPLEX                 28
#define GETSETPAPERBINS              29
#define GETSETPRINTORIENT            30
#define ENUMPAPERBINS                31
#define SETDIBSCALING                32
#define EPSPRINTING                  33
#define ENUMPAPERMETRICS             34
#define GETSETPAPERMETRICS           35
#define GETVERSION                   36
#define POSTSCRIPT_DATA              37
#define POSTSCRIPT_IGNORE            38
#define QUERYROPSUPPORT              40
#define GETDEVICEUNITS               42
#define RESETDEVICE                  128
#define GETEXTENDEDTEXTMETRICS       256
#define GETEXTENTTABLE               257
#define GETPAIRKERNTABLE             258
#define GETTRACKKERNTABLE            259
#define EXTTEXTOUT                   512
#define GETFACENAME                  513
#define DOWNLOADFACE                 514
#define ENABLERELATIVEWIDTHS         768
#define ENABLEPAIRKERNING            769
#define SETKERNTRACK                 770
#define SETALLJUSTVALUES             771
#define SETCHARSET                   772
#define STRETCHBLT                   2048
#define QUERYDIBSUPPORT              3073
#define QDI_SETDIBITS                0x0001
#define QDI_GETDIBITS                0x0002
#define QDI_DIBTOSCREEN              0x0004
#define QDI_STRETCHDIB               0x0008
#define DCICOMMAND                   3075
#define BEGIN_PATH                   4096
#define CLIP_TO_PATH                 4097
#define END_PATH                     4098
#define EXT_DEVICE_CAPS              4099
#define RESTORE_CTM                  4100
#define SAVE_CTM                     4101
#define SET_ARC_DIRECTION            4102
#define SET_BACKGROUND_COLOR         4103
#define SET_POLY_MODE                4104
#define SET_SCREEN_ANGLE             4105
#define SET_SPREAD                   4106
#define TRANSFORM_CTM                4107
#define SET_CLIP_BOX                 4108
#define SET_BOUNDS                   4109
#define OPENCHANNEL                  4110
#define DOWNLOADHEADER               4111
#define CLOSECHANNEL                 4112
#define SETGDIXFORM                  4113
#define RESETPAGE                    4114
#define POSTSCRIPT_PASSTHROUGH       4115
#define ENCAPSULATED_POSTSCRIPT      4116   


typedef FONTINFO    FAR *LPFONTINFO;
typedef DRAWMODE    FAR *LPDRAWMODE;
typedef TEXTXFORM   FAR *LPTEXTXFORM;
typedef TEXTMETRIC  FAR *LPTEXTMETRIC;
typedef LOGFONT     FAR *LPLOGFONT;
typedef LOGPEN      FAR *LPLOGPEN;
typedef LOGBRUSH    FAR *LPLOGBRUSH;
typedef BITMAP      FAR *LPBITMAP;
typedef FARPROC     FAR *LPFARPROC;
typedef GDIINFO     FAR *LPGDIINFO;
typedef SCALABLEFONTINFO FAR * LPSCALABLEFONTINFO;

