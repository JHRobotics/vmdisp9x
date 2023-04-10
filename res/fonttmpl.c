#include "winhack.h"
/* Trick to get a variable-size LOGFONT structure. */
#define LF_FACESIZE
#include <gdidefs.h>

LOGFONT OEMFixed = {
    OEM_FNT_HEIGHT,
    OEM_FNT_WIDTH,
    0,
    0,
    0,
    0,
    0,
    0,
    255,        /* Charset. */
    0,
    2,          /* Clip precision. */
    2,          /* Quality. */
    1,          /* Pitch. */
    "Terminal"  /* Font face. */
};

LOGFONT ANSIFixed = {
    12,         /* Width. */
    9,          /* Height. */
    0,
    0,
    0,
    0,
    0,
    0,
    0,          /* Charset. */
    0,
    2,          /* Clip precision. */
    2,          /* Quality. */
    1,          /* Pitch. */
    "Courier"   /* Font face. */
};

LOGFONT ANSIVar = {
    12,         /* Width. */
    9,          /* Height. */
    0,
    0,
    0,
    0,
    0,
    0,
    0,          /* Charset. */
    0,
    2,          /* Clip precision. */
    2,          /* Quality. */
    2,          /* Pitch. */
    "Helv"      /* Font face. */
};
