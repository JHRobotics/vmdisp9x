/*****************************************************************************

Copyright (c) 2025 Jaroslav Hensl <emulator@emulace.cz>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.

*****************************************************************************/

#ifndef __MTRR_H__INCLUDED__
#define __MTRR_H__INCLUDED__

#define	MTRR_DEVICE_ID DDS_DEVICE_ID

#define MTRR__Get_Version 0
#define MTRR__MTRRSetPhysicalCacheTypeRange 1
#define MTRR__MTRRIsPatSupported 2
#define MTRR__MTRR_PowerState_Change 3

DWORD __cdecl MTRR_GetVersion();
DWORD __cdecl MTRR_SetPhysicalCacheTypeRange(DWORD PhysicalAddress, DWORD reserved, DWORD NumberOfBytes, DWORD CacheType);

#define MTRR_STATUS_SUCCESS         0x00000000UL
#define MTRR_STATUS_NO_SUCH_DEVICE  0xC000000EUL
#define MTRR_STATUS_UNSUCCESSFUL    0xC0000001UL
#define MTRR_STATUS_INTERNAL_ERROR  0xC00000E5UL

#define MTRR_NONCACHED               0
#define MTRR_CACHED	                 1
#define MTRR_FRAMEBUFFERCACHED       2
#define MTRR_HARDWARECOHERENTCACHED  3
#define MTRR_MAXIMUMCACHETYPE        4

#endif /* __MTRR_H__INCLUDED__ */
