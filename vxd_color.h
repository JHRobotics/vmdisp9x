#ifndef __VXD_COLOR_H__INCLUDED__
#define __VXD_COLOR_H__INCLUDED__

DWORD palette_emulation[256] = {0};

static inline void blit16(
	void *src, DWORD src_pitch,
	void *dst, DWORD dst_pitch,
	DWORD blit_x, DWORD blit_y, DWORD blit_w, DWORD blit_h)
{
	const DWORD bottom = blit_y+blit_h;
	DWORD x, y;
	for(y = blit_y; y < bottom; y++)
	{
		WORD *src_ptr = ((WORD*)(((BYTE*)src) + src_pitch*y))+blit_x;
		DWORD *dst_ptr = ((DWORD*)(((BYTE*)dst) + dst_pitch*y))+blit_x;		
		
		for(x = 0; x < blit_w; x++)
		{
			DWORD px = *src_ptr;
			*dst_ptr = ((px & 0xF800) << 8) | ((px & 0x07E0) << 5) | ((px & 0x001F) << 3);
			src_ptr++;
			dst_ptr++;
		}
	}
}

static inline void blit8(
	void *src, DWORD src_pitch,
	void *dst, DWORD dst_pitch,
	DWORD blit_x, DWORD blit_y, DWORD blit_w, DWORD blit_h)
{
	const DWORD bottom = blit_y+blit_h;
	DWORD x, y;
	for(y = blit_y; y < bottom; y++)
	{
		BYTE *src_ptr = (((BYTE*)src) + src_pitch*y)+blit_x;
		DWORD *dst_ptr = ((DWORD*)(((BYTE*)dst) + dst_pitch*y))+blit_x;		
		
		for(x = 0; x < blit_w; x++)
		{
			*dst_ptr = palette_emulation[*src_ptr];
			src_ptr++;
			dst_ptr++;
		}
	}
}

static inline void readback16(
	void *src, DWORD src_pitch,
	void *dst, DWORD dst_pitch,
	DWORD blit_x, DWORD blit_y, DWORD blit_w, DWORD blit_h)
{
	const DWORD bottom = blit_y+blit_h;
	DWORD x, y;
	for(y = blit_y; y < bottom; y++)
	{
		DWORD *src_ptr = ((DWORD*)(((BYTE*)src) + src_pitch*y))+blit_x;
		WORD  *dst_ptr =  ((WORD*)(((BYTE*)dst) + dst_pitch*y))+blit_x;		
		
		for(x = 0; x < blit_w; x++)
		{
			DWORD px = *src_ptr;
			*dst_ptr = ((px & 0xF80000) >> 8) | ((px & 0xFC00) >> 5) | ((px & 0x00F8) >> 3);
			src_ptr++;
			dst_ptr++;
		}
	}
}

#endif /* __VXD_COLOR_H__INCLUDED__ */
