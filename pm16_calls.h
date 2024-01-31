#ifndef __VXDCALL_H__INCLUDED__
#define __VXDCALL_H__INCLUDED__

BOOL VXD_VM_connect();

#ifdef QEMU
BOOL VXD_QEMUFX_supported();
#endif

#endif /* __VXDCALL_H__INCLUDED__ */
