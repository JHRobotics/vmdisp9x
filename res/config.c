#include "winhack.h"
#include <gdidefs.h>

/* Typedef from Windows 3.1 DDK. */

typedef struct {

    /* machine-dependent parameters */
    short   VertThumHeight; /* vertical thumb height (in pixels)  */
    short   HorizThumWidth; /* horizontal thumb width (in pixels) */
    short   IconXRatio;     /* icon width (in pixels)             */
    short   IconYRatio;     /* icon height (in pixels)            */
    short   CurXRatio;      /* cursor width (in pixels)           */
    short   CurYRatio;      /* cursor height (in pixels)          */
    short   Reserved;       /* reserved                           */
    short   XBorder;        /* vertical-line width                */
    short   YBorder;        /* horizontal-line width              */

    /* default-system color values */
    RGBQUAD clrScrollbar;
    RGBQUAD clrDesktop;
    RGBQUAD clrActiveCaption;
    RGBQUAD clrInactiveCaption;
    RGBQUAD clrMenu;
    RGBQUAD clrWindow;
    RGBQUAD clrWindowFrame;
    RGBQUAD clrMenuText;
    RGBQUAD clrWindowText;
    RGBQUAD clrCaptionText;
    RGBQUAD clrActiveBorder;
    RGBQUAD clrInactiveBorder;
    RGBQUAD clrAppWorkspace;
    RGBQUAD clrHiliteBk;
    RGBQUAD clrHiliteText;
    RGBQUAD clrBtnFace;
    RGBQUAD clrBtnShadow;
    RGBQUAD clrGrayText;
    RGBQUAD clrBtnText;
    RGBQUAD clrInactiveCaptionText;
} CONFIG_BIN;


CONFIG_BIN Config = {
    17,
    17,
    2,
    2,
    1,
    1,
    0,
    1,
    1,

    192, 192, 192, 0,
    192, 192, 192, 0,
    000, 000, 128, 0,
    255, 255, 255, 0,
    255, 255, 255, 0,
    255, 255, 255, 0,
    000, 000, 000, 0,
    000, 000, 000, 0,
    000, 000, 000, 0,
    255, 255, 255, 0,
    192, 192, 192, 0,
    192, 192, 192, 0,
    255, 255, 255, 0,
    000, 000, 128, 0,
    255, 255, 255, 0,
    192, 192, 192, 0,
    128, 128, 128, 0,
    192, 192, 192, 0,
    000, 000, 000, 0,
    000, 000, 000, 0
};
