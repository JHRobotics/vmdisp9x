#ifndef __CONTROL_VXD_H__INCLUDED__
#define __CONTROL_VXD_H__INCLUDED__

BOOL VXD_load();
BOOL VXD_CreateRegion(DWORD nPages, DWORD __far *lpLAddr, DWORD __far *lpPPN, DWORD __far *lpPGBLKAddr);
BOOL VXD_FreeRegion(DWORD LAddr, DWORD PGBLKAddr);
void VXD_zeromem(DWORD LAddr, DWORD size);
DWORD VXD_apiver();
void CB_start();
void CB_stop();
void VXD_get_addr(DWORD __far *lpLinFB, DWORD __far *lpLinFifo, DWORD __far *lpLinFifoBounce);
void VXD_get_flags(DWORD __far *lpFlags);

BOOL VXD_FIFOCommit(DWORD bytes);

#define VXD_FIFOCommitAll() \
	if(VXD_FIFOCommit(gSVGA.fifo.reservedSize)){gSVGA.fifo.reservedSize = 0;}else{SVGA_FIFOCommitAll();}

#endif
