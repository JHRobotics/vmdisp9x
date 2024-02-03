#ifndef __VXD_H__INCLUDED__
#define __VXD_H__INCLUDED__

/* Table of "known" drivers
 * https://fd.lod.bz/rbil/interrup/windows/2f1684.html#sect-4579
 * This looks like free
 */
#define VXD_DEVICE_SVGA_ID 0x4331
#define VXD_DEVICE_QEMU_ID 0x4332
#define VXD_DEVICE_VBE_ID  0x4333

#if defined(SVGA)
#define VXD_DEVICE_ID VXD_DEVICE_SVGA_ID
#define VXD_DEVICE_NAME "VMWSVXD"
#elif defined(QEMU)
#define VXD_DEVICE_ID VXD_DEVICE_QEMU_ID
#define VXD_DEVICE_NAME "QEMUVXD"
#else
#define VXD_DEVICE_ID VXD_DEVICE_VBE_ID
#define VXD_DEVICE_NAME "BOXVVXD"
#endif

#define VXD_MAJOR_VER 4 /* should be 4 for windows 95 and newer */
#define VXD_MINOR_VER 0

#define VXD_PM16_VERSION                      0
#define VXD_PM16_VMM_VERSION                  1

#endif /* __VXD_H__INCLUDED__ */
