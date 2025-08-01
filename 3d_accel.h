/*****************************************************************************

Copyright (c) 2024 Jaroslav Hensl <emulator@emulace.cz>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.

*****************************************************************************/

/* const and types for GPU acceleration */

#ifndef __3D_ACCEL_H__
#define __3D_ACCEL_H__

#ifdef __WATCOMC__
#ifndef VXD32
#define FBHDA_SIXTEEN
#endif
#endif

#define API_3DACCEL_VER 20250702

#define ESCAPE_DRV_NT         0x1103 /* (4355) */

/* function codes */
#define OP_FBHDA_SETUP        0x110B /* VXD, DRV, ExtEscape, VxDCall */
#define OP_FBHDA_ACCESS_BEGIN 0x110C /* VXD, DRV, ESCAPE_DRV_NT */
#define OP_FBHDA_ACCESS_END   0x110D /* VXD, DRV, ESCAPE_DRV_NT */
#define OP_FBHDA_SWAP         0x110E /* VXD, DRV, ESCAPE_DRV_NT */
#define OP_FBHDA_CLEAN        0x110F /* VXD, DRV, ESCAPE_DRV_NT */
#define OP_FBHDA_PALETTE_SET  0x1110 /* VXD, DRV, ESCAPE_DRV_NT */
#define OP_FBHDA_PALETTE_GET  0x1111 /* VXD, DRV, ESCAPE_DRV_NT */
#define OP_FBHDA_ACCESS_RECT  0x1112 /* VXD, DRV, ESCAPE_DRV_NT */
#define OP_FBHDA_OVERLAY_SETUP  0x1113 /* VXD, VxDCall, ESCAPE_DRV_NT */
#define OP_FBHDA_OVERLAY_LOCK   0x1114 /* VXD, VxDCall, ESCAPE_DRV_NT */
#define OP_FBHDA_OVERLAY_UNLOCK 0x1115 /* VXD, VxDCall, ESCAPE_DRV_NT */

#define OP_FBHDA_GAMMA_SET    0x1116 /* VXD, DRV, ESCAPE_DRV_NT */
#define OP_FBHDA_GAMMA_GET    0x1117 /* VXD, DRV, ESCAPE_DRV_NT */

#define OP_FBHDA_PAGE_MOD     0x1118 /* VXD */
#define OP_FBHDA_MODE_QUERY   0x1119 /* VXD */

#define OP_SVGA_VALID         0x2000  /* VXD, DRV, ESCAPE_DRV_NT */
#define OP_SVGA_SETMODE       0x2001  /* DRV */
#define OP_SVGA_VALIDMODE     0x2002  /* DRV */
#define OP_SVGA_HW_ENABLE     0x2003  /* DRV */
#define OP_SVGA_HW_DISABLE    0x2004  /* DRV */
#define OP_SVGA_CMB_ALLOC     0x2005  /* VXD, DRV, ESCAPE_DRV_NT */
#define OP_SVGA_CMB_FREE      0x2006  /* VXD, DRV, ESCAPE_DRV_NT */
#define OP_SVGA_CMB_SUBMIT    0x2007  /* VXD, DRV, ESCAPE_DRV_NT */
#define OP_SVGA_FENCE_GET     0x2008  /* VXD */
#define OP_SVGA_FENCE_QUERY   0x2009  /* VXD */
#define OP_SVGA_FENCE_WAIT    0x200A  /* VXD */
#define OP_SVGA_REGION_CREATE 0x200B  /* VXD */
#define OP_SVGA_REGION_FREE   0x200C  /* VXD */
#define OP_SVGA_QUERY         0x200D  /* VXD */
#define OP_SVGA_QUERY_VECTOR  0x200E  /* VXD */
#define OP_SVGA_DB_SETUP      0x200F  /* VXD */
#define OP_SVGA_OT_SETUP      0x2010  /* VXD */
#define OP_SVGA_FLUSHCACHE    0x2011  /* VXD */
#define OP_SVGA_VXDCMD        0x2012  /* VXD */

#define OP_VBE_VALID          0x3000 /* VXD, DRV, ESCAPE_DRV_NT */
#define OP_VBE_SETMODE        0x3001 /* DRV */
#define OP_VBE_VALIDMODE      0x3002 /* DRV */

#define OP_VESA_VALID         0x4000 /* VXD, DRV, ESCAPE_DRV_NT */
#define OP_VESA_SETMODE       0x4001 /* DRV */
#define OP_VESA_VALIDMODE     0x4002 /* DRV */
#define OP_VESA_HIRES         0x4003 /* DRV */

#define OP_MOUSE_BUFFER       0x1F00 /* DRV */
#define OP_MOUSE_LOAD         0x1F01 /* DRV */         
#define OP_MOUSE_MOVE         0x1F02 /* DRV */
#define OP_MOUSE_SHOW         0x1F03 /* DRV */
#define OP_MOUSE_HIDE         0x1F04 /* DRV */

/* VXDCall */
#define FBHDA_DEVICE_ID       0x4333
#define FBHDA_SERVICE_TABLE_OFFSET 0x110A
#define FBHDA__GET_VERSION 0
#define FBHDA__SETUP (OP_FBHDA_SETUP-FBHDA_SERVICE_TABLE_OFFSET)
#define FBHDA__OVERLAY_SETUP (OP_FBHDA_OVERLAY_SETUP-FBHDA_SERVICE_TABLE_OFFSET)
#define FBHDA__OVERLAY_LOCK (OP_FBHDA_OVERLAY_LOCK-FBHDA_SERVICE_TABLE_OFFSET)
#define FBHDA__OVERLAY_UNLOCK (OP_FBHDA_OVERLAY_UNLOCK-FBHDA_SERVICE_TABLE_OFFSET)

#pragma pack(push)
#pragma pack(1)

#ifdef FBHDA_SIXTEEN
# define FBPTR __far*
#else
# define FBPTR *
#endif

#define FBHDA_OVERLAYS_MAX 16
//#define FBHDA_ROW_ALIGN 8
#define FBHDA_ROW_ALIGN 4

typedef struct FBHDA_overlay
{
#ifndef FBHDA_SIXTEEN
	void *ptr;
#else
	DWORD ptr32;
#endif
	DWORD size;
} FBHDA_overlay_t;

typedef struct FBHDA
{
	         DWORD cb;
           DWORD flags;
           DWORD version;
	volatile DWORD width;
	volatile DWORD height;
	volatile DWORD bpp;
	volatile DWORD pitch;
	volatile DWORD surface;
	volatile DWORD stride;
#ifndef FBHDA_SIXTEEN
	         void *vram_pm32;
	         DWORD vram_pm16;
#else
           DWORD       vram_pm32;
           void __far *vram_pm16;
#endif
	         DWORD vram_size; /* real r/w memory size */
	         char vxdname[16]; /* file name or "NT" */
	         DWORD overlay;
	         FBHDA_overlay_t overlays[FBHDA_OVERLAYS_MAX];
	         DWORD overlays_size;
	         DWORD gamma; /* fixed decimal point, 65536 = 1.0 */
	         DWORD system_surface;
	         DWORD palette_update; /* INC by one everytime when the palette is updated */
	         DWORD gamma_update; /* INC by one everytime when the pallete is updated */
	         DWORD gpu_mem_total;
	         DWORD gpu_mem_used;
	         /* heap allocator */
#ifndef FBHDA_SIXTEEN
	         DWORD *heap_info; /* info block (id of first block, or ~0 on free) */
	         BYTE  *heap_start; /* minimal address (but allocation from end to beginning) */
	         BYTE  *heap_end;   /* maximal address + 1 */
#else
	         DWORD heap_info;
	         DWORD heap_start;
	         DWORD heap_end;
#endif
	         DWORD heap_count; /* number of blocks = heap_size_in_bytes / FB_VRAM_HEAP_GRANULARITY */
	         DWORD heap_length; /* maximum usable block with current framebuffer */
	         DWORD vram_bar_size; /* PCI region size, may be larger then vram_size */
	         DWORD res2;
	         DWORD res3;
} FBHDA_t;

typedef struct FBHDA_mode
{
	         DWORD cb;
	         DWORD width;
	         DWORD height;
	         DWORD bpp;
	         DWORD refresh;
} FBHDA_mode_t;

#define FB_VRAM_HEAP_GRANULARITY (4*32)
/* minimum of vram allocation (32px at 32bpp, or 64px at 16bpp) */

#define FB_SUPPORT_FLIPING     1
#define FB_ACCEL_VIRGE         2
#define FB_ACCEL_CHROMIUM      4
#define FB_ACCEL_QEMU3DFX      8
#define FB_ACCEL_VMSVGA        16
#define FB_ACCEL_VMSVGA3D      32
#define FB_ACCEL_VMSVGA10      64
#define FB_MOUSE_NO_BLIT      128
#define FB_FORCE_SOFTWARE     256
#define FB_ACCEL_VMSVGA10_ST  512 /* not used */
#define FB_BUG_VMWARE_UPDATE 1024
#define FB_ACCEL_GPUMEM      2048
#define FB_VESA_MODES        8192
#define FB_SUPPORT_VSYNC    16384
#define FB_SUPPORT_CLOCK    32768
#define FB_SUPPORT_TRIPLE   65536

/* for internal use in RING-0 by VXD only */
BOOL FBHDA_init_hw(); 
void FBHDA_release_hw();

/* for internal use by RING-3 application/driver */
void FBHDA_load();
void FBHDA_free();

#ifdef FBHDA_SIXTEEN
	void FBHDA_setup(FBHDA_t __far* __far* FBHDA, DWORD __far* FBHDA_linear);
#else
	FBHDA_t *FBHDA_setup();
#endif

#define FBHDA_ACCESS_RAW_BUFFERING 1
#define FBHDA_ACCESS_MOUSE_MOVE 2
#define FBHDA_ACCESS_SURFACE_DIRTY 4

#define FBHDA_SWAP_NOWAIT     1
#define FBHDA_SWAP_WAIT       2
#define FBHDA_SWAP_SCHEDULE   4

void FBHDA_access_begin(DWORD flags);
void FBHDA_access_end(DWORD flags);
void FBHDA_access_rect(DWORD left, DWORD top, DWORD right, DWORD bottom);
BOOL FBHDA_swap(DWORD offset);
BOOL FBHDA_swap_ex(DWORD offset, DWORD flags);
void FBHDA_clean();
void  FBHDA_palette_set(unsigned char index, DWORD rgb);
DWORD FBHDA_palette_get(unsigned char index);

/* return pitch or 0 when failed */
DWORD FBHDA_overlay_setup(DWORD overlay, DWORD width, DWORD height, DWORD bpp);
void  FBHDA_overlay_lock(DWORD left, DWORD top, DWORD right, DWORD bottom);
void  FBHDA_overlay_unlock(DWORD flags);

/* format simitar to WINAPI Set/GetDeviceGammaRamp */
BOOL FBHDA_gamma_get(VOID FBPTR ramp, DWORD buffer_size);
BOOL FBHDA_gamma_set(VOID FBPTR ramp, DWORD buffer_size);

/* mouse */
#ifdef FBHDA_SIXTEEN
void mouse_buffer(void __far* __far* pBuf, DWORD __far* pLinear);
#else
void *mouse_buffer();
#endif
BOOL mouse_load();
void mouse_move(int x, int y);
void mouse_show();
void mouse_hide();

/* vxd internal */
void mouse_invalidate(); 
BOOL mouse_blit();
void mouse_erase();

#define MOUSE_BUFFER_SIZE 65535

/* helper for some hacks */
BOOL FBHDA_page_modify(DWORD flat_address, DWORD size, const BYTE *new_data);

/* query resulutions + refresh rate (need FB_VESA_MODES flag set) */
BOOL FBHDA_mode_query(DWORD index, FBHDA_mode_t *mode);

/*
 * VMWare SVGA-II API
 */
#ifdef SVGA

typedef struct SVGA_region_info
{
	DWORD   region_id;
	DWORD   size;
	void   *address; /* user memory */
	void   *region_address;
	DWORD   region_ppn;
	void*   mob_address;
	DWORD   mob_ppn;
	DWORD   mob_pt_depth;
	DWORD   is_mob;
	DWORD   mobonly;
} SVGA_region_info_t;

typedef struct SVGA_CMB_status
{
	volatile DWORD  *qStatus;
	         DWORD   sStatus;
	         DWORD   fifo_fence_used;
	         DWORD   fifo_fence_last;
} SVGA_CMB_status_t;

#define SVGA_PROC_NONE         0
#define SVGA_PROC_COMPLETED    1
#define SVGA_PROC_QUEUED       2
#define SVGA_PROC_ERROR        3
#define SVGA_PROC_FENCE     0xFF

typedef struct SVGA_DB_region
{
	DWORD pid;
	SVGA_region_info_t info;
	DWORD pad1;
	DWORD pad2;
} SVGA_DB_region_t;

typedef struct SVGA_DB_context
{
	DWORD pid;
	void *cotable;
	DWORD gmrId; /* mob id for context */
	DWORD pad2;
} SVGA_DB_context_t;

typedef struct SVGA_DB_surface
{
	DWORD pid;
	DWORD format; /* format of surface (SVGA3dSurfaceFormat) */
	DWORD width;
	DWORD height;
	DWORD bpp;
	DWORD gmrId; /* != 0 for GB surfaces */
	DWORD gmrMngt; /* 1 when auto destroy MOB and region when releasing surface */
	DWORD size; /* surface size in bytes */
	DWORD flags;
} SVGA_DB_surface_t;

typedef struct SVGA_DB
{
	SVGA_DB_region_t   *regions;
	SVGA_DB_context_t  *contexts;
	SVGA_DB_surface_t  *surfaces;
	DWORD               regions_cnt;
	DWORD               contexts_cnt;
	DWORD               surfaces_cnt;
	DWORD              *regions_map;
	DWORD              *contexts_map;
	DWORD              *surfaces_map;
	char                mutexname[64];
	DWORD               stat_regions_usage;
	DWORD               pad1;
	DWORD               pad2;
} SVGA_DB_t;

/* internal VXD only */
BOOL SVGA_init_hw();

BOOL SVGA_valid();

#define SVGA_CB_USE_CONTEXT_DEVICE 0x80000000UL /* CB control command */
#define SVGA_CB_SYNC               0x40000000UL /* wait to command complete */
#define SVGA_CB_FORCE_FIFO         0x20000000UL /* insert this command to FIFO, event the CB is working */
#define SVGA_CB_FORCE_FENCE        0x10000000UL /* force insert fence */
#define SVGA_CB_PRESENT            0x08000000UL /* this is 'present' command: WAIT for previous 'present' and 'render' to complete */
#define SVGA_CB_DIRTY_SURFACE      0x04000000UL /* need reread GPU SURFACE first, can combine with SVGA_CB_PRESENT */
#define SVGA_CB_RENDER             0x02000000UL /* this is 'render' cmd, WAIT for 'present', 'update' */
#define SVGA_CB_UPDATE             0x01000000UL /* this is 'update' cmd, updates screen on HOST, WAIT for 'update', 'present' */

/* SVGA_CB_FLAG_DX_CONTEXT */

BOOL SVGA_setmode(DWORD w, DWORD h, DWORD bpp);
BOOL SVGA_validmode(DWORD w, DWORD h, DWORD bpp);
#ifdef FBHDA_SIXTEEN
  void SVGA_CMB_alloc(DWORD FBPTR cmb, DWORD cmb_linear);
#else
  DWORD FBPTR SVGA_CMB_alloc();
#endif
void SVGA_CMB_free(DWORD FBPTR cmb);

/* for data exchange by DeviceIoControl */
typedef struct SVGA_CMB_submit_io
{
	DWORD *cmb;
	DWORD  cmb_size;
	DWORD flags;
	DWORD DXCtxId;
} SVGA_CMB_submit_io_t;

void SVGA_CMB_submit(DWORD FBPTR cmb, DWORD cmb_size, SVGA_CMB_status_t FBPTR status, DWORD flags, DWORD DXCtxId);

DWORD SVGA_fence_get();
void SVGA_fence_query(DWORD FBPTR ptr_fence_passed, DWORD FBPTR ptr_fence_last);
void SVGA_fence_wait(DWORD fence_id);
BOOL SVGA_region_create(SVGA_region_info_t FBPTR rinfo);
void SVGA_region_free(SVGA_region_info_t FBPTR rinfo);

#define SVGA_QUERY_REGS 1
#define SVGA_QUERY_FIFO 2
#define SVGA_QUERY_CAPS 3

DWORD SVGA_query(DWORD type, DWORD index);
void SVGA_query_vector(DWORD type, DWORD index_start, DWORD count, DWORD *out);

void SVGA_HW_enable();
void SVGA_HW_disable();

SVGA_DB_t *SVGA_DB_setup();

void SVGA_DB_lock();
void SVGA_DB_unlock();

#define SVGA_OT_FLAG_ALLOCATED 1
#define SVGA_OT_FLAG_ACTIVE    2
#define SVGA_OT_FLAG_DIRTY     4

typedef struct SVGA_OT_info_entry
{
	DWORD   ppn;
	void   *lin;
	DWORD   size;
	DWORD   flags;
	DWORD   pt_depth;
} SVGA_OT_info_entry_t;

SVGA_OT_info_entry_t *SVGA_OT_setup();

void SVGA_flushcache();

BOOL SVGA_vxdcmd(DWORD cmd, DWORD arg);
#define SVGA_CMD_INVALIDATE_FB 1
#define SVGA_CMD_CLEANUP 2

#endif /* SVGA */

/*
 * Bochs VBE Extensions API
 */
#ifdef VBE

BOOL VBE_init_hw(); /* internal for VXD only */

void VBE_HW_enable();
void VBE_HW_disable();
BOOL VBE_valid();
BOOL VBE_validmode(DWORD w, DWORD h, DWORD bpp);
BOOL VBE_setmode(DWORD w, DWORD h, DWORD bpp);

#endif /* VBE */

/*
 * VESA Video API
 */
#ifdef VESA

BOOL VESA_init_hw(); /* internal for VXD only */

void VESA_HIRES_enable();
void VESA_HIRES_disable();
BOOL VESA_valid();
BOOL VESA_validmode(DWORD w, DWORD h, DWORD bpp);
BOOL VESA_setmode(DWORD w, DWORD h, DWORD bpp, DWORD rr_min, DWORD rr_max);

#endif

#pragma pack(pop)

/* DLL handlers */
#define VMDISP9X_LIB "vmdisp9x.dll"

typedef FBHDA_t *(__cdecl *FBHDA_setup_t)();
typedef void (__cdecl *FBHDA_access_begin_t)(DWORD flags);
typedef void (__cdecl *FBHDA_access_end_t)(DWORD flags);
typedef void (__cdecl *FBHDA_access_rect_t)(DWORD left, DWORD top, DWORD right, DWORD bottom);
typedef BOOL (__cdecl *FBHDA_swap_t)(DWORD offset);
typedef BOOL (__cdecl *FBHDA_page_modify_t)(DWORD flat_address, DWORD size, const BYTE *new_data);
typedef void (__cdecl *FBHDA_clean_t)(void);
typedef BOOL (__cdecl *FBHDA_mode_query_t)(DWORD index, FBHDA_mode_t *mode);

typedef struct _fbhda_lib_t
{
	HMODULE lib;
	LONG lock;
	FBHDA_setup_t pFBHDA_setup;
	FBHDA_access_begin_t pFBHDA_access_begin;
	FBHDA_access_end_t pFBHDA_access_end;
	FBHDA_access_rect_t pFBHDA_access_rect;
	FBHDA_swap_t pFBHDA_swap;
	FBHDA_page_modify_t pFBHDA_page_modify;
	FBHDA_clean_t pFBHDA_clean;
	FBHDA_mode_query_t pFBHDA_mode_query;
} fbhda_lib_t;

#endif /* __3D_ACCEL_H__ */
