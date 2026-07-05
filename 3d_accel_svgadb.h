/*****************************************************************************

Copyright (c) 2026 Jaroslav Hensl <emulator@emulace.cz>

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
#ifndef __3D_ACCEL_SVGADB_H__INCLUDED__
#define __3D_ACCEL_SVGADB_H__INCLUDED__

#define BSTEP (sizeof(DWORD)*8)

static inline DWORD map_lookup_and_set(DWORD *bitmap, DWORD start, DWORD max)
{
	DWORD i = start / BSTEP;
	DWORD ii = start % BSTEP;
	
	bitmap += i;
	
	for(; i < max; i += BSTEP)
	{
		DWORD tmp = *bitmap;
		if(tmp != 0) /* skip all-occupied double words */
		{
			for(; ii < BSTEP; ii++)
			{
				if(((tmp >> ii) & 0x1) != 0)
				{
					*bitmap &= ~((DWORD)1 << ii); /* set the bit in bitmap (to zero) */
					return i+ii;
				}
			}
			ii = 0;
		}
		bitmap++;
	}
	
	return max;
}

static inline void map_reset(DWORD *bitmap, DWORD id)
{
	DWORD i = id / BSTEP;
	DWORD ii = id % BSTEP;
	
	bitmap += i;
	*bitmap |= ((DWORD)1 << ii);
}

#endif /* __3D_ACCEL_SVGADB_H__INCLUDED__ */
