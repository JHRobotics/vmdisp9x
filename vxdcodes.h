#ifndef __VMWSVXD_H__INCLUDED__
#define __VMWSVXD_H__INCLUDED__

/* Table of "known" drivers
 * https://fd.lod.bz/rbil/interrup/windows/2f1684.html#sect-4579
 * This looks like free
 */
#define VXD_DEVICE_SVGA_ID 0x4331
#define VXD_DEVICE_QEMU_ID 0x4332

#ifdef SVGA
#define VXD_DEVICE_ID VXD_DEVICE_SVGA_ID
#endif

#ifdef QEMU
#define VXD_DEVICE_ID VXD_DEVICE_QEMU_ID
#endif

#define VXD_MAJOR_VER 4 /* should be 4 for windows 95 and newer */
#define VXD_MINOR_VER 0

#define VXD_PM16_VERSION                      0
#define VXD_PM16_VMM_VERSION                  1
#define VXD_PM16_ZEROMEM                      6

/* SVGA specific functions */
#define VMWSVXD_PM16_CREATE_REGION            2
#define VMWSVXD_PM16_DESTROY_REGION           3
#define VMWSVXD_PM16_APIVER                   7
#define VMWSVXD_PM16_CB_START                 8
#define VMWSVXD_PM16_CB_STOP                  9
#define VMWSVXD_PM16_GET_ADDR                 10
#define VMWSVXD_PM16_FIFO_COMMIT              11
#define VMWSVXD_PM16_FIFO_COMMIT_SYNC         12
#define VMWSVXD_PM16_GET_FLAGS                13

/* QEMU specific function */
#define QEMUVXD_QEMUFX                        14

#endif /* __VMWSVXD_H__INCLUDED__ */
