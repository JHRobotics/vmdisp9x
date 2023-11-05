#ifndef __SWCURSOR_H__INCLUDED__
#define __SWCURSOR_H__INCLUDED__

/* from dibcall.c */
extern LONG cursorX;
extern LONG cursorY;

/* from DDK98 */
#pragma pack(push)
#pragma pack(1)
typedef struct
{
   int     xHotSpot, yHotSpot;
   int     cx, cy;
   int     cbWidth;
   BYTE    Planes;
   BYTE    BitsPixel;
} CURSORSHAPE;
#pragma pack(pop)

typedef struct _SWCURSOR SWCURSOR;

BOOL cursor_init();
BOOL cursor_load(CURSORSHAPE __far *lpCursor, longRECT __far *changes);
void cursor_unload(longRECT __far *changes);
void cursor_coordinates(long curx, long cury);
void cursor_erase(longRECT __far *changes);
void cursor_blit(longRECT __far *changes);
void cursor_move(longRECT __far *changes);
void cursor_merge_rect(longRECT __far *r1, longRECT __far *r2);

void cursor_lock();
void cursor_unlock();

extern BOOL sw_cursor_enabled;
extern SWCURSOR __far *sw_cursor;

#endif /* __SWCURSOR_H__INCLUDED__ */
