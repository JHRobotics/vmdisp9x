#ifndef __VXD_SVGA_H__INCLUDED__
#define __VXD_SVGA_H__INCLUDED__

extern void *cmdbuf;
void wait_for_cmdbuf();
void submit_cmdbuf(DWORD cmdsize, DWORD flags, DWORD dx);
void *SVGA_cmd_ptr(DWORD *buf, DWORD *pOffset, DWORD cmd, DWORD cmdsize);
void *SVGA_cmd3d_ptr(DWORD *buf, DWORD *pOffset, DWORD cmd, DWORD cmdsize);
DWORD SVGA_pitch(DWORD width, DWORD bpp);
void SVGA_region_usage_reset();

extern BOOL  st_used;
extern DWORD st_flags;
BOOL st_memory_allocate(DWORD size, DWORD *out);
void st_defineScreen(DWORD w, DWORD h, DWORD bpp);
void st_destroyScreen();
void SVGA_OTable_load();
void SVGA_OTable_unload();

SVGA_DB_surface_t *SVGA_GetSurfaceInfo(DWORD sid);

#define ST_REGION_ID 1
#define ST_SURFACE_ID 1

#define ST_16BPP   1
#define ST_CURSOR  2
#define ST_CURSOR_HIDEABLE 4

BOOL st_useable(DWORD bpp);

DWORD map_pm16(DWORD vm, DWORD linear, DWORD size);

/* mouse */
BOOL SVGA_mouse_hw();
BOOL SVGA_mouse_load();
void SVGA_mouse_move(int x, int y);
void SVGA_mouse_show();
void SVGA_mouse_hide(BOOL invalidate);

#endif /* __VXD_SVGA_H__INCLUDED__ */
