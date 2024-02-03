
static BOOL calc_save(int x, int y)
{
	mouse_swap_w = mouse_w;
	mouse_swap_x = x - mouse_pointx;
	
	if(mouse_swap_x < 0)
	{
		mouse_swap_w -= x;
		mouse_swap_x = 0;
	}
	
	if(mouse_swap_x + mouse_swap_w > hda->width)
		mouse_swap_w = hda->width - mouse_swap_x;
	
	mouse_swap_h = mouse_h;
	mouse_swap_y = y - mouse_pointy;
	
	if(mouse_swap_y < 0)
	{
		mouse_swap_h -= y;
		mouse_swap_y = 0;
	}
	
	if(mouse_swap_y + mouse_swap_h > hda->height)
		mouse_swap_h = hda->height - mouse_swap_y;
	
	if(mouse_swap_w <= 0 || mouse_swap_h <= 0)
	{
		mouse_swap_valid = FALSE;
		return FALSE;
	}
	
	return TRUE;
}

#define DEF_SAVE(_bpp, _type) \
static void save ## _bpp(int mx, int my){ \
	int x, y; \
	_type *screen; \
	_type *buf = mouse_swap_data; \
	if(!calc_save(mx, my)){ return; } \
	screen = (_type*)((BYTE*)hda->vram_pm32 + hda->pitch * mouse_swap_y); \
	for(y = 0; y < mouse_swap_h; y++) { \
		for(x = 0; x < mouse_swap_w; x++) { \
			*buf = screen[mouse_swap_x + x]; \
			buf++; \
		} \
		screen = (_type*)((BYTE*)screen+hda->pitch); \
	} \
	mouse_swap_valid = TRUE; }

DEF_SAVE( 8,  BYTE)
DEF_SAVE(16,  WORD)
DEF_SAVE(32, DWORD)

static void save24(int mx, int my)
{
	int x, y;
	BYTE *screen;
	BYTE *buf = mouse_swap_data;
	if(!calc_save(mx, my)){ return; }
	screen = (BYTE*)hda->vram_pm32 + hda->pitch * mouse_swap_y;
	for(y = 0; y < mouse_swap_h; y++)
	{
		for(x = 0; x < mouse_swap_w; x++)
		{
			*(buf++) = screen[(mouse_swap_x + x)*3    ];
			*(buf++) = screen[(mouse_swap_x + x)*3 + 1];
			*(buf++) = screen[(mouse_swap_x + x)*3 + 2];
		}
		screen += hda->pitch;
	}
	mouse_swap_valid = TRUE;
}

#define DEF_RESTORE(_bpp, _type) \
static void restore ## _bpp () { \
	int x, y; \
	_type *screen; \
	_type *buf = mouse_swap_data; \
	if(!mouse_swap_valid){ return; } \
	screen = (_type*)((BYTE*)hda->vram_pm32 + hda->pitch * mouse_swap_y); \
	for(y = 0; y < mouse_swap_h; y++) { \
		for(x = 0; x < mouse_swap_w; x++) { \
			screen[mouse_swap_x + x] = *buf; buf++; \
		} \
		screen = (_type*)((BYTE*)screen+hda->pitch); \
	} \
	mouse_swap_valid = FALSE; }

DEF_RESTORE( 8, BYTE)
DEF_RESTORE(16, WORD)
DEF_RESTORE(32, DWORD)

void restore24()
{
	int x, y;
	BYTE *screen;
	BYTE *buf = mouse_swap_data;
	if(!mouse_swap_valid){ return; }
	screen = (BYTE*)hda->vram_pm32 + hda->pitch * mouse_swap_y;
	for(y = 0; y < mouse_swap_h; y++)
	{
		for(x = 0; x < mouse_swap_w; x++)
		{
			screen[(mouse_swap_x + x)*3    ] = *(buf++);
			screen[(mouse_swap_x + x)*3 + 1] = *(buf++);
			screen[(mouse_swap_x + x)*3 + 2] = *(buf++);
		}
		screen += hda->pitch;
	}
	mouse_swap_valid = FALSE;
}

#define DEF_BLIT(_bpp, _type) \
static void blit ## _bpp (int mx, int my) { \
	int x, y; \
	_type *screen_line; \
	_type *and_line; \
	_type *xor_line; \
	int real_x = mx - mouse_pointx; \
	int real_y = my - mouse_pointy; \
	int real_xx, real_yy; \
	for(y = 0; y < mouse_h; y++) { \
		real_yy = real_y+y; \
		if(real_yy >= 0 && real_yy < hda->height) { \
			screen_line = (_type*)(((BYTE*)hda->vram_pm32) + hda->pitch * (real_yy)); \
			and_line    = ((_type*)mouse_andmask_data + mouse_w * y); \
			xor_line    = ((_type*)mouse_xormask_data + mouse_w * y); \
			for(x = 0; x < mouse_w; x++) { \
				real_xx = real_x+x; \
				if(real_xx >= 0 && real_xx < hda->width) { \
					screen_line[real_xx] &= and_line[x]; \
					screen_line[real_xx] ^= xor_line[x]; \
} } } } }

DEF_BLIT(8,  BYTE)
DEF_BLIT(16, WORD)
DEF_BLIT(32, DWORD)

static void blit24(int mx, int my)
{
	int x, y;
	BYTE *screen_line;
	BYTE *and_line;
	BYTE *xor_line;
	int real_x = mx - mouse_pointx;
	int real_y = my - mouse_pointy;
	int real_xx, real_yy;
	int real_xx3, x3;
	for(y = 0; y < mouse_h; y++)
	{
		real_yy = real_y+y;
		if(real_yy >= 0 && real_yy < hda->height)
		{
			screen_line = ((BYTE*)hda->vram_pm32) + hda->pitch * real_yy;
			and_line    = ((BYTE*)mouse_andmask_data) + mouse_w * y * 3;
			xor_line    = ((BYTE*)mouse_xormask_data) + mouse_w * y * 3;
			for(x = 0; x < mouse_w; x++)
			{
				real_xx = real_x+x;
				if(real_xx >= 0 && real_xx < hda->width)
				{
					real_xx3 = real_xx*3;
					x3 = x*3;
					screen_line[real_xx3  ] &= and_line[x3  ]; screen_line[real_xx3  ] ^= xor_line[x3  ];
					screen_line[real_xx3+1] &= and_line[x3+1]; screen_line[real_xx3+1] ^= xor_line[x3+1];
					screen_line[real_xx3+2] &= and_line[x3+2]; screen_line[real_xx3+2] ^= xor_line[x3+2];
				}
			}
		}
	}
}

#define EXPAND_BIT(_dsttype, _bit) (~(((_dsttype)(_bit))-1))

#define DEF_CONVMASK(_bpp, _type) \
static void convmask ## _bpp(CURSORSHAPE *lpCursor, DWORD cbWidth, void *src, void *dst) { \
	_type *ptr = dst; \
	int x, y, xb, x8; \
	BYTE pix8; \
	for(y = 0; y < lpCursor->cy; y++) { \
		for(x = 0; x < cbWidth; x++) { \
			pix8 = ((BYTE *)src)[cbWidth*y + x]; \
			x8 = x*8; \
			for(xb = 7; xb >= 0 && x8 < lpCursor->cx; xb--,x8++) { \ 
				*(ptr++) = EXPAND_BIT(_type, ((pix8 >> xb) & 0x1)); \
} } } }

DEF_CONVMASK( 8,  BYTE)
DEF_CONVMASK(16,  WORD)
DEF_CONVMASK(32, DWORD)

static void convmask24(CURSORSHAPE *lpCursor, DWORD cbWidth, void *src, void *dst)
{
	BYTE *ptr = dst;
	int x, y, xb, x8;
	BYTE pix8;
	BYTE mask;
	for(y = 0; y < lpCursor->cy; y++)
	{
		for(x = 0; x < cbWidth; x++)
		{
			pix8 = ((BYTE *)src)[cbWidth*y + x];
			x8 = x*8;
			for(xb = 7; xb >= 0 && x8 < lpCursor->cx; xb--,x8++)
			{
				mask = EXPAND_BIT(BYTE, ((pix8 >> xb) & 0x1));
				*(ptr++) = mask;
				*(ptr++) = mask;
				*(ptr++) = mask;
			}
		}
	}
}

static void draw_save(int mx, int my)
{
	switch(mouse_ps)
	{
		case 1:  save8(mx, my); return;
		case 2: save16(mx, my); return;
		case 3: save24(mx, my); return;
		case 4: save32(mx, my); return;
	}
}

static void draw_restore()
{
	switch(mouse_ps)
	{
		case 1:  restore8(); return;
		case 2: restore16(); return;
		case 3: restore24(); return;
		case 4: restore32(); return;
	}
}

static void draw_blit(int mx, int my)
{
	switch(mouse_ps)
	{
		case 1:  blit8(mx, my); return;
		case 2: blit16(mx, my); return;
		case 3: blit24(mx, my); return;
		case 4: blit32(mx, my); return;
	}
}

static void convmask(CURSORSHAPE *lpCursor, DWORD cbWidth, void *src, void *dst)
{
	switch(mouse_ps)
	{
		case 1:  convmask8(lpCursor, cbWidth, src, dst); return;
		case 2: convmask16(lpCursor, cbWidth, src, dst); return;
		case 3: convmask24(lpCursor, cbWidth, src, dst); return;
		case 4: convmask32(lpCursor, cbWidth, src, dst); return;
	}
}

static BOOL cursor_is_empty()
{
	DWORD s = mouse_w * mouse_h * mouse_ps;
	BYTE *and_mask = (BYTE *)mouse_andmask_data;
	BYTE *xor_mask = (BYTE *)mouse_xormask_data;
	
	while(s--)
	{
		if((*and_mask) != 0xFF || (*xor_mask) != 0x00)
		{
			return FALSE;
		}
		and_mask++;
		xor_mask++;
	}
	
	return TRUE;
}
