#ifndef __VMWSVXD_H__INCLUDED__
#define __VMWSVXD_H__INCLUDED__

/* Table of "known" drivers
 * https://fd.lod.bz/rbil/interrup/windows/2f1684.html#sect-4579
 * This looks like free
 */
#define VMWSVXD_DEVICE_ID 0x7fe7

#define VMWSVXD_MAJOR_VER 2
#define VMWSVXD_MINOR_VER 0

#define VMWSVXD_PM16_VERSION                      0
#define VMWSVXD_PM16_VMM_VERSION                  1
#define VMWSVXD_PM16_CREATE_REGION                2
#define VMWSVXD_PM16_DESTROY_REGION               3
#define VMWSVXD_PM16_CREATE_MOB                   4
#define VMWSVXD_PM16_DESTROY_MOB                  5
#define VMWSVXD_PM16_ZEROMEM                      6
#define VMWSVXD_PM16_APIVER                       7

#endif
