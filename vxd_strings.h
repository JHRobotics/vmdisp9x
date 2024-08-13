/*
 * Open Watcom generate invalid code when using inline const strings,
 * so this code doesn't work:
 *   void somefunc(){
 *     printf("Hello %s!", "World");
 *   }
 * Workaround is following:
 *   static char str_hello[] = "Hello %s!";
 *   static char str_world[] = "World";
 *   void somefunc(){
 *     printf(str_hello, str_world);
 *   }
 * 
 * And for these string is this file...
 *
 */
#ifdef DBGPRINT

#ifdef VXD_MAIN
#define DSTR(_n, _s) char _n[] = _s
#else
#define DSTR(_n, _s) extern char _n[]
#endif

DSTR(dbg_hello, "Hello world!\n");
DSTR(dbg_version, "VMM version: ");
DSTR(dbg_region_err, "region create error\n");

DSTR(dbg_Device_Init_proc, "Device_Init_proc\n");
DSTR(dbg_Device_Init_proc_succ, "Device_Init_proc success\n");
DSTR(dbg_dic_ring, "DeviceIOControl: Ring\n");
DSTR(dbg_dic_sync, "DeviceIOControl: Sync\n");
DSTR(dbg_dic_unknown, "DeviceIOControl: Unknown: %d\n");
DSTR(dbg_dic_system, "DeviceIOControl: System code: %d\n");
DSTR(dbg_get_ppa, "%lx -> %lx\n");
DSTR(dbg_get_ppa_beg, "Virtual: %lx\n");
DSTR(dbg_mob_allocate, "Allocated OTable row: %d\n");

DSTR(dbg_str, "%s\n");

DSTR(dbg_submitcb_fail, "CB submit FAILED\n");
DSTR(dbg_submitcb, "CB submit %d\n");
DSTR(dbg_lockcb, "Reused CB (%d) with status: %d\n");

DSTR(dbg_lockcb_lasterr, "Error command: %lX\n");

DSTR(dbg_cb_on, "CB supported and allocated\n");
DSTR(dbg_gb_on, "GB supported and allocated\n");
DSTR(dbg_cb_ena, "CB context 0 enabled\n");

DSTR(dbg_region_info_1, "Region id = %d\n");
DSTR(dbg_region_info_2,"Region address = %lX, PPN = %lX, GMRBLK = %lX\n");

DSTR(dbg_mapping, "Memory mapping:\n");
DSTR(dbg_mapping_map, "  %X -> %X\n");
DSTR(dbg_destroy, "Driver destroyed\n");

DSTR(dbg_siz, "Size of gSVGA(2) = %d %d\n");

DSTR(dbg_test, "test %d\n");

DSTR(dbg_SVGA_Init, "SVGA_Init: %d\n");

DSTR(dbg_update, "Update screen: %d %d %d\n");

DSTR(dbg_cmd_on, "SVGA_CMB_submit: ptr=%X, first cmd=%X, flags=%X, size=%d\n");
DSTR(dbg_cmd_off, "SVGA_CMB_submit: end - cmd: %X\n");
DSTR(dbg_cmd_error, "CB error: %d, first cmd %X (error at %d)\n");

DSTR(dbg_deviceiocontrol, "I%x\n");
DSTR(dbg_deviceiocontrol_leave, "IL\n");

DSTR(dbg_gmr, "GMR: %ld at %X\n");
DSTR(dbg_gmr_succ, "GMR ALLOC: %ld (size: %ld)\n");

DSTR(dbg_region_simple, "GMR is continous, memory maped: %X, user memory: %X\n");
DSTR(dbg_region_fragmented, "GMR is fragmented\n");

DSTR(dbg_pages, "GMR: size: %ld pages: %ld P_SIZE: %ld\n");

DSTR(dbg_fence_overflow, "fence overflow\n");

DSTR(dbg_pagefree, "_PageFree: %X\n");

DSTR(dbg_vbe_fail, "Bochs VBE detection failure!\n");

DSTR(dbg_vbe_init, "Bochs VBE: vram: %X, size: %ld\n");

DSTR(dbg_vbe_lfb, "LFB at %X\n");

DSTR(dbg_mouse_cur, "Mouse %d %d %d %d\n");

DSTR(dbg_flatptr, "Flatptr: %X\n");

DSTR(dbg_vxd_api, "VXD_API_Proc, service: %X\n");

DSTR(dbg_cursor_empty, "new cursor: empty %X\n");

DSTR(dbg_spare_region, "used spare region = pages: %ld, address: %X\n");

DSTR(dbg_free_as_spare, "Saved region = address %X as spare\n");

DSTR(dbg_cache_insert, "CACHE: region saved(%ld): size %ld\n");

DSTR(dbg_cache_used, "CACHE: region used(%ld): size %ld\n");

DSTR(dbg_pagefree_end, "GMR  FREE: %ld (size: %ld, cached: %ld)\n");

DSTR(dbg_cache_search, "CACHE try to find: %ld\n");

DSTR(dbg_cache_delete, "CACHE: deleted old = %d\n");

DSTR(dbg_mobonly, "GMR/MOB only: %d\n");

DSTR(dbg_cache, "Cache enabled: %d\n");

DSTR(dbg_mob_size, "sizeof(SVGA3dCmdDefineGBMob) = %d\n");

DSTR(dbg_map_pm16, "map_pm16: vm = %ld, linear = %lX, size = %ld\n");

DSTR(dbg_map_pm16_sel, "map_pm16: selector %lX\n");

DSTR(dbg_register, "register: ebx = %ld, ecx = %ld, VM = %lX\n");

DSTR(dbg_map_pm16_qw, "map_pm16: high=%lX, low=%lX\n");

DSTR(dbg_lock_cb, "Lock CB buffer: %ld (line: %ld)\n");
DSTR(dbg_lock_fb, "Lock FB buffer: %ld (line: %ld)\n");

DSTR(dbg_cpu_lock, "surface cpu lock for SID: %ld\n");

DSTR(dbg_hw_mouse_move, "hw mouse: moving: %ld x %ld (visible: %d, valid: %d)\n");
DSTR(dbg_hw_mouse_show, "hw mouse: show (valid: %d)\n");
DSTR(dbg_hw_mouse_hide, "hw mouse: hide\n");

DSTR(dbg_cb_flags, "CB: flags = %lX\n");

DSTR(dbg_wait_cb, "... Waiting for CB at line: %ld\n");

DSTR(dbg_ctr_start, "CB CTRL: ");

DSTR(dbg_cb_stop_status,  "stop (status %ld)\n");
DSTR(dbg_cb_start_status, "start (status %ld)\n");

DSTR(dbg_irq, "IRQ!\n");

DSTR(dbg_irq_install, "IRQ(%d) trap installed\n");
DSTR(dbg_irq_install_fail, "IRQ(%d) found, but cannot be traped\n");

DSTR(dbg_no_irq, "No IRQ enabled\n");

DSTR(dbg_disable, "HW disable\n");

DSTR(dbg_pt_build, "PT_build(%ld): BASE=%lX TYPE=%ld USER=%lp\n");
DSTR(dbg_pt_build_2, "PT_build(%ld): pt1_entries=%ld, pt2_entries=%ld\n");

DSTR(dbg_cb_suc, "submit SVGA_CB_SYNC success\n");

DSTR(dbg_region_1, "SVGA_region_create #1: %ld, max: %ld\n");
DSTR(dbg_region_2, "SVGA_region_create #2: %ld\n");

DSTR(dbg_cb_error, "Error (%ld): offset %ld, error command: %ld\n");

DSTR(dbg_cs_underflow, "WARNING: closing inactive CS!\n");
DSTR(dbg_cs_active, "WARNING: CS is still active!\n");

DSTR(dbg_fence_wait, "SVGA_fence_wait(%lu) from line %ld\n");

DSTR(dbg_queue_check, "CB_queue_check\n");

DSTR(dbg_cb_valid_err, "ERROR - %s: %lX, cmdbuf: %lX\n");

DSTR(dbg_err_double_insert, "double_insert");
DSTR(dbg_err_pop, "not pull out");

DSTR(dbg_cb_valid_status, " -> status = %ld\n");

DSTR(dbg_trace_insert, "<=  %lX\n");
DSTR(dbg_trace_remove, " => %lX\n");

DSTR(dbg_mouse_invalidate, "MOUSE = invalidate\n");
DSTR(dbg_mouse_load, "MOUSE = load ");
DSTR(dbg_mouse_move, "MOUSE = xy(%ld, %ld)\n");

DSTR(dbg_mouse_no_mem, "no mem\n");
DSTR(dbg_mouse_status, " valid=%ld, visible=%ld, empty=%ld\n");

DSTR(dbg_mouse_hide, "MOUSE = hide\n");
DSTR(dbg_mouse_show, "MOUSE = show\n");

#undef DSTR

#endif
