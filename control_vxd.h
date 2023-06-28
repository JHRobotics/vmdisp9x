#ifndef __CONTROL_VXD_H__INCLUDED__
#define __CONTROL_VXD_H__INCLUDED__

BOOL VXD_load();
BOOL VXD_CreateRegion(uint32_t nPages, uint32_t __far *lpLAddr, uint32_t __far *lpPPN, uint32_t __far *lpPGBLKAddr);
BOOL VXD_FreeRegion(uint32_t LAddr, uint32_t PGBLKAddr);
void VXD_zeromem(uint32_t LAddr, uint32_t size);
uint32_t VXD_apiver();

#endif
