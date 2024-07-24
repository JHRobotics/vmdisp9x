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

#define API_3DACCEL_VER 20240724

#define ESCAPE_DRV_NT         0x1103 /* (4355) */

/* function codes */
#define OP_FBHDA_SETUP        0x110B /* VXD, DRV, ExtEscape */
#define OP_FBHDA_ACCESS_BEGIN 0x110C /* VXD, DRV, ESCAPE_DRV_NT */
#define OP_FBHDA_ACCESS_END   0x110D /* VXD, DRV, ESCAPE_DRV_NT */
#define OP_FBHDA_SWAP         0x110E /* VXD, DRV, ESCAPE_DRV_NT */
#define OP_FBHDA_CLEAN        0x110F /* VXD, DRV, ESCAPE_DRV_NT */
#define OP_FBHDA_PALETTE_SET  0x1110 /* VXD, DRV, ESCAPE_DRV_NT */
#define OP_FBHDA_PALETTE_GET  0x1111 /* VXD, DRV, ESCAPE_DRV_NT */
#define OP_FBHDA_ACCESS_RECT  0x1112 /* VXD, DRV, ESCAPE_DRV_NT */

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

#define OP_MOUSE_BUFFER       0x1F00 /* DRV */
#define OP_MOUSE_LOAD         0x1F01 /* DRV */         
#define OP_MOUSE_MOVE         0x1F02 /* DRV */
#define OP_MOUSE_SHOW         0x1F03 /* DRV */
#define OP_MOUSE_HIDE         0x1F04 /* DRV */


#pragma pack(push)
#pragma pack(1)

#ifdef FBHDA_SIXTEEN
# define FBPTR __far*
#else
# define FBPTR *
#endif

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
	         DWORD vram_size;
	         char vxdname[16]; /* file name or "NT" */
} FBHDA_t;

#define FB_SUPPORT_FLIPING     1
#define FB_ACCEL_VIRGE         2
#define FB_ACCEL_CHROMIUM      4
#define FB_ACCEL_QEMU3DFX      8
#define FB_ACCEL_VMSVGA        16
#define FB_ACCEL_VMSVGA3D      32
#define FB_ACCEL_VMSVGA10      64
#define FB_MOUSE_NO_BLIT      128
#define FB_FORCE_SOFTWARE     256
#define FB_ACCEL_VMSVGA10_ST  512

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

#define FBHDA_IGNORE_CURSOR 1

void FBHDA_access_begin(DWORD flags);
void FBHDA_access_end(DWORD flags);
void FBHDA_access_rect(DWORD left, DWORD top, DWORD right, DWORD bottom);
BOOL FBHDA_swap(DWORD offset);
void FBHDA_clean();

void  FBHDA_palette_set(unsigned char index, DWORD rgb);
DWORD FBHDA_palette_get(unsigned char index);

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

#define SVGA_CB_USE_CONTEXT_DEVICE 0x80000000UL
#define SVGA_CB_SYNC               0x40000000UL
#define SVGA_CB_FORCE_FIFO         0x20000000UL
#define SVGA_CB_FORCE_FENCE        0x10000000UL
#define SVGA_CB_PRESENT_ASYNC      0x08000000UL
#define SVGA_CB_PRESENT_GPU        0x04000000UL

// SVGA_CB_FLAG_DX_CONTEXT

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

BOOL SVGA_vxdcmd(DWORD cmd);
#define SVGA_CMD_INVALIDATE_FB 1

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

void VESA_HW_enable();
void VESA_HW_disable();
BOOL VESA_valid();
BOOL VESA_validmode(DWORD w, DWORD h, DWORD bpp);
BOOL VESA_setmode(DWORD w, DWORD h, DWORD bpp);

#endif

#pragma pack(pop)

#endif /* __3D_ACCEL_H__ */
