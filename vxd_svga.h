#ifndef __VXD_SVGA_H__INCLUDED__
#define __VXD_SVGA_H__INCLUDED__

extern void *cmdbuf;
void wait_for_cmdbuf();
void *SVGA_cmd_ptr(DWORD *buf, DWORD *pOffset, DWORD cmd, DWORD cmdsize);
void *SVGA_cmd3d_ptr(DWORD *buf, DWORD *pOffset, DWORD cmd, DWORD cmdsize);
DWORD SVGA_pitch(DWORD width, DWORD bpp);

extern BOOL st_used;
BOOL st_memory_allocate(DWORD size, DWORD *out);
void st_defineScreen(DWORD w, DWORD h, DWORD bpp);
void st_destroyScreen();
void SVGA_OTable_load();
SVGA_DB_surface_t *SVGA_GetSurfaceInfo(uint32 sid);

#define ST_REGION_ID 1
#define ST_SURFACE_ID 1

#define st_useable(_bpp) (st_used && (_bpp) == 32)

DWORD map_pm16(DWORD vm, DWORD linear, DWORD size);

#endif /* __VXD_SVGA_H__INCLUDED__ */
