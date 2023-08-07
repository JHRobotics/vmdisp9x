#ifndef __CONTROL_VXD_H__INCLUDED__
#define __CONTROL_VXD_H__INCLUDED__

BOOL VXD_load();
BOOL VXD_CreateRegion(DWORD nPages, DWORD __far *lpLAddr, DWORD __far *lpPPN, DWORD __far *lpPGBLKAddr);
BOOL VXD_FreeRegion(DWORD LAddr, DWORD PGBLKAddr);
void VXD_zeromem(DWORD LAddr, DWORD size);
DWORD VXD_apiver();
void CB_start();
void CB_stop();

#endif
