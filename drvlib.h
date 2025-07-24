#ifndef __DRVLIB_H__INCLUDED__
#define __DRVLIB_H__INCLUDED__

void drv_memcpy(void __far *dst, void __far *src, long size);
void __far *drv_malloc(DWORD dwSize, DWORD __far *lpLinear);
void drv_memset_large(DWORD dwLinearBase, DWORD dwOffset, UINT value, DWORD dwNum);
BOOL drv_is_p2();

#endif /* __DRVLIB_H__INCLUDED__ */
