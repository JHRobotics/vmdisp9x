#ifndef __CURSOR_H__INCLUDED__
#define __CURSOR_H__INCLUDED__

/* from DDK98 */
#pragma pack(push)
#pragma pack(1)
typedef struct _CURSORSHAPE
{
   short   xHotSpot, yHotSpot;
   short   cx, cy;
   short   cbWidth;
   BYTE    Planes;
   BYTE    BitsPixel;
} CURSORSHAPE;
#pragma pack(pop)

#endif /* __CURSOR_H__INCLUDED__ */
