#ifndef __VXD_HALLOC__INCLUDED__
#define __VXD_HALLOC__INCLUDED__

BOOL vxd_hinit();
BOOL vxd_halloc(DWORD pages, void **flat/*, DWORD *phy*/);
void vxd_hfree(void *flat);

void vxd_hstats_update();

#endif /* __VXD_HALLOC__INCLUDED__ */
