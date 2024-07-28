#ifndef __VXD_SVGA_H__INCLUDED__
#define __VXD_SVGA_H__INCLUDED__

/* consts */
#define ST_REGION_ID 1
#define ST_SURFACE_ID 1

#define ST_16BPP   1
#define ST_CURSOR  2
#define ST_CURSOR_HIDEABLE 4


/* VM handle */
extern DWORD ThisVM;

extern void *cmdbuf;
extern void *ctlbuf;
void wait_for_cmdbuf();
void submit_cmdbuf(DWORD cmdsize, DWORD flags, DWORD dx);
void *SVGA_cmd_ptr(DWORD *buf, DWORD *pOffset, DWORD cmd, DWORD cmdsize);
void *SVGA_cmd3d_ptr(DWORD *buf, DWORD *pOffset, DWORD cmd, DWORD cmdsize);
DWORD SVGA_pitch(DWORD width, DWORD bpp);
void SVGA_region_usage_reset();
BOOL SVGA_fence_is_passed(DWORD fence_id);
DWORD SVGA_fence_passed();
DWORD SVGA_GetDevCap(DWORD search_id);

#ifdef DBGPRINT
void SVGA_fence_wait_dbg(DWORD fence_id, int line);

#define SVGA_fence_wait(_fence) SVGA_fence_wait_dbg(_fence, __LINE__)

#endif


/* screen target */
extern BOOL  st_used;
extern DWORD st_flags;
BOOL st_memory_allocate(DWORD size, DWORD *out);
void st_defineScreen(DWORD w, DWORD h, DWORD bpp);
void st_destroyScreen();

SVGA_DB_surface_t *SVGA_GetSurfaceInfo(DWORD sid);
BOOL st_useable(DWORD bpp);

/* VDD */
DWORD map_pm16(DWORD vm, DWORD linear, DWORD size);

void SVGA_Sync();
void SVGA_Flush_CB();
void SVGA_Flush_CB_critical();

/* mouse */
BOOL SVGA_mouse_hw();
BOOL SVGA_mouse_load();
void SVGA_mouse_move(int x, int y);
void SVGA_mouse_show();
void SVGA_mouse_hide(BOOL invalidate);

/* memory */
void set_fragmantation_limit();
void SVGA_OTable_load();
void SVGA_OTable_alloc(BOOL screentargets);
void SVGA_OTable_unload();
void cache_init();
void cache_enable(BOOL enabled);

/* CB */
DWORD *SVGA_CMB_alloc_size(DWORD datasize);
void SVGA_CMB_free(DWORD *cmb);
void SVGA_CB_start();
void SVGA_CB_stop();
void SVGA_CB_restart();
void SVGA_CMB_wait_update();

void mob_cb_alloc();
void *mob_cb_get();

#endif /* __VXD_SVGA_H__INCLUDED__ */
