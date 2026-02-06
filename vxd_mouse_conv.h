
#define EXPAND_BIT(_dsttype, _bit) (~(((_dsttype)(_bit))-1))

#define DEF_CONVMASK(_bpp, _type) \
static void convmask ## _bpp(CURSORSHAPE *lpCursor, DWORD cbWidth, void *src, void *dst) { \
	int x, y, xb, x8; \
	BYTE pix8; \
	for(y = 0; y < lpCursor->cy; y++) { \
		_type *ptr = (_type *)(((DWORD*)dst)+CURSOR_DMAX*y); \
		for(x = 0; x < cbWidth; x++) { \
			pix8 = ((BYTE *)src)[cbWidth*y + x]; \
			x8 = x*8; \
			for(xb = 7; xb >= 0 && x8 < lpCursor->cx; xb--,x8++) { \ 
				*(ptr++) = EXPAND_BIT(_type, ((pix8 >> xb) & 0x1)); \
} } } }

//DEF_CONVMASK( 8,  BYTE)
//DEF_CONVMASK(16,  WORD)
DEF_CONVMASK(32, DWORD)

static void expandmask16(CURSORSHAPE *lpCursor, DWORD cbWidth, void *src, void *dst)
{
	LONG y, x;
	WORD px16;
	pixel_32_t px;
	px.dw = 0;

	for(y = 0; y < lpCursor->cy; y++)
	{
		DWORD *ptr = ((DWORD*)dst)+CURSOR_DMAX*y;
		for(x = 0; x < cbWidth; x += 2)
		{
			px16 = *((WORD*)(((BYTE*)src)+cbWidth*y + x*2));
			px.c.r = px16 >> 11;
			px.c.g = (px16 >> 5) & 0xFC;
			px.c.b = px16 & 0xF8;
			
			/* expand by last 2 LSB bits */
			if((px.c.r & 0x18) == 0x18) px.c.r |= 0x7;
			if((px.c.g & 0x0C) == 0x0C) px.c.r |= 0x3;
			if((px.c.b & 0x18) == 0x18) px.c.b |= 0x7;
				
			*ptr = px.dw;
			ptr++;
		}
	}
}

static void expandmask15(CURSORSHAPE *lpCursor, DWORD cbWidth, void *src, void *dst)
{
	LONG y, x;
	WORD px16;
	pixel_32_t px;
	px.dw = 0;

	for(y = 0; y < lpCursor->cy; y++)
	{
		DWORD *ptr = ((DWORD*)dst)+CURSOR_DMAX*y;
		for(x = 0; x < cbWidth; x+= 2)
		{
			px16 = *((WORD*)(((BYTE*)src)+cbWidth*y + x));
			px.c.r = (px16 >> 10) & 0xF8;
			px.c.g = (px16 >>  5) & 0xF8;
			px.c.b =  px16 & 0xF8;
			
			/* expand by last 2 LSB bits */
			if((px.c.r & 0x18) == 0x18) px.c.r |= 0x7;
			if((px.c.g & 0x18) == 0x18) px.c.r |= 0x7;
			if((px.c.b & 0x18) == 0x18) px.c.b |= 0x7;
				
			*ptr = px.dw;
			ptr++;
		}
	}
}

static void expandmask8(CURSORSHAPE *lpCursor, DWORD cbWidth, void *src, void *dst)
{
	LONG y, x;
	for(y = 0; y < lpCursor->cy; y++)
	{
		DWORD *ptr = ((DWORD*)dst)+CURSOR_DMAX*y;
		for(x = 0; x < cbWidth; x++)
		{
			BYTE index = ((BYTE*)src)[cbWidth*y + x];
			*ptr = wram->palette[index].dw;
			ptr++;
		}
	}
}

static void expandmask24(CURSORSHAPE *lpCursor, DWORD cbWidth, void *src, void *dst)
{
	LONG y, x;
	pixel_32_t px;
	px.dw = 0;

	for(y = 0; y < lpCursor->cy; y++)
	{
		DWORD *ptr = ((DWORD*)dst)+CURSOR_DMAX*y;
		for(x = 0; x < cbWidth; x += 3)
		{
			px.c.r = ((BYTE*)src)[cbWidth*y + x];
			px.c.g = ((BYTE*)src)[cbWidth*y + x + 1];
			px.c.b = ((BYTE*)src)[cbWidth*y + x + 2];
			*ptr = px.dw;
			ptr++;
		}
	}
}

static void expandmask32(CURSORSHAPE *lpCursor, DWORD cbWidth, void *src, void *dst)
{
	LONG y, x;
	for(y = 0; y < lpCursor->cy; y++)
	{
		DWORD *ptr = ((DWORD*)dst)+CURSOR_DMAX*y;
		for(x = 0; x < cbWidth; x += 4)
		{
			*ptr = *((DWORD*)(((BYTE*)src)+cbWidth*y + x));
			ptr++;
		}
	}
}

static void expandmask(CURSORSHAPE *lpCursor, DWORD cbWidth, void *src, void *dst, int bits)
{
	switch(bits)
	{
		case 1:
			convmask32(lpCursor, cbWidth, src, dst);
			break;
		case 8:
			expandmask8(lpCursor, cbWidth, src, dst);
			break;
		case 15:
			expandmask15(lpCursor, cbWidth, src, dst);
			break;
		case 16:
			expandmask16(lpCursor, cbWidth, src, dst);
			break;
		case 24:
			expandmask24(lpCursor, cbWidth, src, dst);
			break;
		case 32:
			expandmask32(lpCursor, cbWidth, src, dst);
			break;
	}
}

static BOOL cursor_is_empty()
{
	LONG x, y;	
	for(y = 0; y < wram->regs.s.cursor_h; y++)
	{
		for(x = 0; x < wram->regs.s.cursor_w; x++)
		{
			if(wram->cursor.andmask[CURSOR_DMAX*y + x] != 0xFF ||
				wram->cursor.xormask[CURSOR_DMAX*y + x] != 0x00)
			{
				return FALSE;
			}
		}
	}
	return TRUE;
}
