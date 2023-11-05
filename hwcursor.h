#ifndef __HWCURSOR_INCLUDED__
#define __HWCURSOR_INCLUDED__

void SVGA_UpdateLongRect(longRECT __far *rect);
BOOL hwcursor_move(WORD absX, WORD absY);
BOOL hwcursor_load(CURSORSHAPE __far *lpCursor);
void hwcursor_update();
BOOL hwcursor_available();

#endif
