#ifndef __VXDCALL_H__INCLUDED__
#define __VXDCALL_H__INCLUDED__

BOOL VXD_load();
void VXD_zeromem(DWORD LAddr, DWORD size);

#ifdef SVGA
BOOL VXD_CreateRegion(DWORD nPages, DWORD __far *lpLAddr, DWORD __far *lpPPN, DWORD __far *lpPGBLKAddr);
BOOL VXD_FreeRegion(DWORD LAddr, DWORD PGBLKAddr);
DWORD VXD_apiver();
void CB_start();
void CB_stop();
void VXD_get_addr(DWORD __far *lpLinFB, DWORD __far *lpLinFifo, DWORD __far *lpLinFifoBounce);
void VXD_get_flags(DWORD __far *lpFlags);
BOOL VXD_FIFOCommit(DWORD bytes, BOOL sync);

#define VXD_FIFOCommitAll() \
	if(VXD_FIFOCommit(gSVGA.fifo.reservedSize, FALSE)){gSVGA.fifo.reservedSize = 0;}

//	if(VXD_FIFOCommit(gSVGA.fifo.reservedSize, FALSE)){gSVGA.fifo.reservedSize = 0;}else{SVGA_FIFOCommitAll();}

#define VXD_FIFOCommitSync() \
	VXD_FIFOCommit(gSVGA.fifo.reservedSize, TRUE);gSVGA.fifo.reservedSize = 0
#endif /* SVGA */

#ifdef QEMU
BOOL VXD_QEMUFX_supported();
#endif

#endif /* __VXDCALL_H__INCLUDED__ */
