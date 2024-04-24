#ifndef __VERSION_H__INCLUDED__
#define __VERSION_H__INCLUDED__

#define DRV_STR_(x) #x
#define DRV_STR(x) DRV_STR_(x)

/* on binaries equals 1 and for INF is 1 = separate driver, 2 = softgpu pack */
#define DRV_VER_MAJOR 4

/* the YEAR */
#define DRV_VER_MINOR 2024

/* build version is deducted fom GIT */
#define DRV_VER_BUILD_FAILBACK 1

#ifndef DRV_VER_BUILD
#define DRV_VER_BUILD DRV_VER_BUILD_FAILBACK
#endif

#define DRV_VER_STR_BUILD(_ma, _mi, _pa, _bd) \
	_ma "." _mi "." _pa "." _bd

#define DRV_VER_STR DRV_VER_STR_BUILD( \
	DRV_STR(DRV_VER_MAJOR), \
	DRV_STR(DRV_VER_MINOR), \
	DRV_STR(0), \
	DRV_STR(DRV_VER_BUILD))

#define DRV_VER_NUM DRV_VER_MAJOR,DRV_VER_MINOR,0,DRV_VER_BUILD

#endif
