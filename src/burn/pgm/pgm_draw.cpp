#include "pgm.h"

static unsigned short *pTempDraw;

/* PGM Palette */ 

inline static unsigned int CalcCol(unsigned short nColour)
{
	int r, g, b;

	r = (nColour & 0x7C00) >> 7;  // Red 
	r |= r >> 5;
	g = (nColour & 0x03E0) >> 2;	// Green
	g |= g >> 5;
	b = (nColour & 0x001F) << 3;	// Blue
	b |= b >> 5;

	return BurnHighCol(r, g, b, 0);
}

static void pgm_prepare_sprite(int wide, int high,int palt, int boffset)
{
	unsigned char * bdata = PGMSPRMaskROM;
	unsigned char * adata = PGMSPRColROM;
	int bdatasize = nPGMSPRMaskMaskLen;
	int adatasize = nPGMSPRColMaskLen;
	int xcnt, ycnt;

	UINT32 aoffset;
	UINT16 msk;

	aoffset = (bdata[(boffset+3) & bdatasize] << 24) | (bdata[(boffset+2) & bdatasize] << 16) | (bdata[(boffset+1) & bdatasize] << 8) | (bdata[(boffset+0) & bdatasize] << 0);
	aoffset = aoffset >> 2; aoffset *= 3;

	boffset += 4; /* because the first dword is the a data offset */

	for (ycnt = 0 ; ycnt < high ; ycnt++) {
		for (xcnt = 0 ; xcnt < wide ; xcnt++) {
			int x;

			msk = (( bdata[(boffset+1) & bdatasize] << 8) |( bdata[(boffset+0) & bdatasize] << 0) );

			for (x=0;x<16;x++)
			{
				if (!(msk & 0x0001))
				{
					pTempDraw[(ycnt*(wide*16))+(xcnt*16+x)]  = adata[aoffset & adatasize]+ palt*32;
					aoffset++;
				}
				else
				{
					pTempDraw[(ycnt*(wide*16))+(xcnt*16+x)] = 0x8000;
				}
				msk >>=1;
			}

			boffset+=2;
		}
	}
}

static void draw_sprite_line(int wide, UINT16* dest, int xzoom, int xgrow, int yoffset, int flip, int xpos)
{
	int xcnt,xcntdraw;
	int xzoombit;
	int xoffset;
	int xdrawpos = 0;

	xcnt = 0;
	xcntdraw = 0;
	while (xcnt < wide*16)
	{
		UINT32 srcdat;
		if (!(flip&0x01)) xoffset = xcnt;
		else xoffset = (wide*16)-xcnt-1;

		srcdat = pTempDraw[yoffset+xoffset];
		xzoombit = (xzoom >> (xcnt&0x1f))&1;

		if (xzoombit == 1 && xgrow ==1)
		{ // double this column
			xdrawpos = xpos + xcntdraw;
			if (!(srcdat&0x8000))
			{
				if ((xdrawpos >= 0) && (xdrawpos < 448))  dest[xdrawpos] = srcdat;
			}
			xcntdraw++;

			xdrawpos = xpos + xcntdraw;

			if (!(srcdat&0x8000))
			{
				if ((xdrawpos >= 0) && (xdrawpos < 448))  dest[xdrawpos] = srcdat;
			}
			xcntdraw++;
		}
		else if (xzoombit ==1 && xgrow ==0)
		{
			/* skip this column */
		}
		else //normal column
		{
			xdrawpos = xpos + xcntdraw;
			if (!(srcdat&0x8000))
			{
				if ((xdrawpos >= 0) && (xdrawpos < 448))  dest[xdrawpos] = srcdat;
			}
			xcntdraw++;
		}

		xcnt++;

		if (xdrawpos == 448) xcnt = wide*16;
	}
}

static void pgm_draw_sprite_nozoom(int wide, int high, int palt, int boffset, int xpos, int ypos, int flipx, int flipy)
{
	unsigned char * bdata = PGMSPRMaskROM;
	unsigned char * adata = PGMSPRColROM;
	int bdatasize = nPGMSPRMaskMaskLen;
	int adatasize = nPGMSPRColMaskLen;
	int yoff, xoff;

	UINT16 msk;

	unsigned int aoffset = (bdata[(boffset+3) & bdatasize] << 24) | (bdata[(boffset+2) & bdatasize] << 16) | (bdata[(boffset+1) & bdatasize] << 8) | (bdata[boffset & bdatasize] << 0);
	aoffset = (aoffset >> 2) * 3;

	boffset += 4;

	palt <<= 5;
	if (flipx) flipx = (wide * 16)-1; // -1 for olegend background

	for (int ycnt = 0; ycnt < high; ycnt++) {
		if (flipy) {
			yoff = ypos + ((high-1) - ycnt); // -1 for drw2 background
			if (yoff < 0) break;
		} else {
			yoff = ypos + ycnt;
			if (yoff >= nScreenHeight) break;
		}

		for (int xcnt = 0; xcnt < wide*16; xcnt+=8)
		{
			msk = bdata[boffset & bdatasize];

			for (int x = 0; x < 8; x++)
			{
				if (~msk & 0x0001)
				{
					if (flipx) {
						xoff = xpos + (flipx - (xcnt + x));
					} else {
						xoff = xpos + (xcnt + x);
					}
			
					if (yoff >= 0 && yoff < nScreenHeight && xoff >= 0 && xoff < nScreenWidth) {
						pTransDraw[(yoff * nScreenWidth) + xoff] = adata[aoffset & adatasize] | palt;
					}

					aoffset++;
				}

				msk >>= 1;
			}

			boffset++;
		}
	}
}

/* this just loops over our decoded bitmap and puts it on the screen */
static void draw_sprite_new_zoomed(int wide, int high, int xpos, int ypos, int palt, int boffset, int flip, UINT32 xzoom, int xgrow, UINT32 yzoom, int ygrow )
{
	int ycnt;
	int ydrawpos;
	UINT16 *dest;
	int yoffset;
	int ycntdraw;
	int yzoombit;

	if (yzoom == 0 && xzoom == 0) {
		pgm_draw_sprite_nozoom(wide, high, palt, boffset, xpos, ypos, flip & 1, flip & 2);
		return;
	}

	pgm_prepare_sprite( wide,high, palt, boffset );

	/* now draw it */
	ycnt = 0;
	ycntdraw = 0;
	while (ycnt < high)
	{
		yzoombit = (yzoom >> (ycnt&0x1f))&1;

		if (yzoombit == 1 && ygrow == 1) // double this line
		{
			ydrawpos = ypos + ycntdraw;

			if (!(flip&0x02)) yoffset = (ycnt*(wide*16));
			else yoffset = ( (high-ycnt-1)*(wide*16));
			if ((ydrawpos >= 0) && (ydrawpos < 224))
			{
				dest = pTransDraw + ydrawpos * nScreenWidth;
				draw_sprite_line(wide, dest, xzoom, xgrow, yoffset, flip, xpos);
			}
			ycntdraw++;

			ydrawpos = ypos + ycntdraw;
			if (!(flip&0x02)) yoffset = (ycnt*(wide*16));
			else yoffset = ( (high-ycnt-1)*(wide*16));
			if ((ydrawpos >= 0) && (ydrawpos < 224))
			{
				dest = pTransDraw + ydrawpos * nScreenWidth;
				draw_sprite_line(wide, dest, xzoom, xgrow, yoffset, flip, xpos);
			}
			ycntdraw++;

			if (ydrawpos ==224) ycnt = high;
		}
		else if (yzoombit ==1 && ygrow == 0)
		{
			/* skip this line */
			/* we should process anyway if we don't do the pre-decode.. */
		}
		else /* normal line */
		{
			ydrawpos = ypos + ycntdraw;

			if (!(flip&0x02)) yoffset = (ycnt*(wide*16));
			else yoffset = ( (high-ycnt-1)*(wide*16));
			if ((ydrawpos >= 0) && (ydrawpos < 224))
			{
				dest = pTransDraw + ydrawpos * nScreenWidth;
				draw_sprite_line(wide, dest, xzoom, xgrow, yoffset, flip, xpos);
			}
			ycntdraw++;

			if (ydrawpos ==224) ycnt = high;
		}

		ycnt++;
	}
}

static void pgm_drawsprites(unsigned short *source, unsigned short *finish)
{
	while (finish >= source)
	{
		int high = source[4] & 0x01ff;
		if (high == 0) break; /* is this right? */

		int xpos = source[0] & 0x07ff;
		int ypos = source[1] & 0x03ff;
		int xzom = (source[0] & 0x7800) >> 11;
		int xgrow = (source[0] & 0x8000) >> 15;
		int yzom = (source[1] & 0x7800) >> 11;
		int ygrow = (source[1] & 0x8000) >> 15;
		int palt = (source[2] & 0x1f00) >> 8;
		int flip = (source[2] & 0x6000) >> 13;
		int boff = ((source[2] & 0x007f) << 16) | (source[3] & 0xffff);
		int wide = (source[4] & 0x7e00) >> 9;

		UINT32 xzoom, yzoom;

		UINT16 *zoomtable = &RamVReg[0x1000/2];

		if (xgrow)
		{
			xzom = 0x10-xzom;
		}

		if (ygrow)
		{
			yzom = 0x10-yzom;
		}

		xzoom = (zoomtable[xzom*2]<<16)|zoomtable[xzom*2+1];
		yzoom = (zoomtable[yzom*2]<<16)|zoomtable[yzom*2+1];

		boff *= 2;
		if (xpos > 0x3ff) xpos -=0x800;
		if (ypos > 0x1ff) ypos -=0x400;

		draw_sprite_new_zoomed(wide, high, xpos, ypos, palt, boff, flip, xzoom,xgrow, yzoom,ygrow);

		source += 5;
	}
}

static void draw_text()
{
	unsigned short *vram = (unsigned short*)RamTx;

	int scrollx = ((signed short)RamVReg[0x6000 / 2]) & 0x1ff;
	int scrolly = ((signed short)RamVReg[0x5000 / 2]) & 0x0ff;

	for (int offs = 0; offs < 64 * 32; offs++)
	{
		int code = vram[offs * 2];
		if (code == 0) continue;

		int sx = (offs & 0x3f) << 3;
		int sy = (offs >> 6) << 3;

		sx -= scrollx;
		if (sx < -7) sx += 512;
		sy -= scrolly;
		if (sy < -7) sy += 256;

		if (sx >= nScreenWidth || sy >= nScreenHeight) continue;

		int attr = vram[offs * 2 + 1];
		int color = ((attr & 0x3e) >> 1) | 0x80;
		int flipx = (attr & 0x40);
		int flipy = (attr & 0x80);

		if (flipy) {
			if (flipx) {
				Render8x8Tile_Mask_FlipXY_Clip(pTransDraw, code, sx, sy, color, 4, 15, 0, PGMTileROM);
			} else {
				Render8x8Tile_Mask_FlipY_Clip(pTransDraw, code, sx, sy, color, 4, 15, 0, PGMTileROM);
			}
		} else {
			if (flipx) {
				Render8x8Tile_Mask_FlipX_Clip(pTransDraw, code, sx, sy, color, 4, 15, 0, PGMTileROM);
			} else {
				Render8x8Tile_Mask_Clip(pTransDraw, code, sx, sy, color, 4, 15, 0, PGMTileROM);
			}
		}
	}
}

static void draw_background()
{
	unsigned short *vram = (unsigned short*)RamBg;
	unsigned short *dst   = pTransDraw;

	unsigned short *rowscroll = (unsigned short*)RamRs;
	int yscroll = (signed short)RamVReg[0x2000 / 2];
	int xscroll = (signed short)RamVReg[0x3000 / 2];

	for (int y = 0; y < 224; y++, dst += nScreenWidth)
	{
		int scrollx = (xscroll + rowscroll[y]) & 0x7ff;
		int scrolly = (yscroll + y) & 0x7ff;

		for (int x = 0; x < 480; x+=32)
		{
			int sx = x - (scrollx & 0x1f);

			int offs = ((scrolly & 0x7e0) << 2) | (((scrollx + x) & 0x7e0) >> 4);

			int attr  = vram[offs + 1];
			int code  = vram[offs];
			if (code > 0x7ff) code += 0x1000;
			if (code == 0) continue;
			int color = ((attr & 0x3e) << 4) | 0x400;
			int flipx = ((attr & 0x40) >> 6) * 0x1f;
			int flipy = ((attr & 0x80) >> 7) * 0x1f;

			unsigned char *src = PGMTileROMExp + (code * 1024) + (((scrolly ^ flipy) & 0x1f) << 5);

			if (sx >= 0 && sx <= 416) {
				for (int xx = 0; xx < 32; xx++, sx++) {
					int pxl = src[xx^flipx];
	
					if (pxl != 0x1f) {
						dst[sx] = pxl | color;
					}
				}
			} else {
				for (int xx = 0; xx < 32; xx++, sx++) {
					if (sx < 0) continue;
					if (sx >= nScreenWidth) break;
	
					int pxl = src[xx^flipx];
	
					if (pxl != 0x1f) {
						dst[sx] = pxl | color;
					}
				}
			}
		}
	}
}

int pgmDraw()
{
	int spr_offs = 0;

	{
		for (int i = 0; i < 0x1200 / 2; i++) {
			RamCurPal[i] = CalcCol(RamPal[i]);
		}

		// black / magenta
		RamCurPal[0x1200] = (nSpriteEnable & 1) ? 0 : BurnHighCol(0xff, 0, 0xff, 0);
	}

	// Fill in background color
	{
		for (int i = 0; i < nScreenWidth * nScreenHeight; i++) {
			pTransDraw[i] = 0x1200;
		}
	}			

	// calculate sprite priorities
	// Draw everything from 0 to the last background priority behind the background
	// layer and then everything from there to the end of sprite ram above it.
	{
		for (int i = (0xa00/2)-5; i >= 0; i-=5) {
			if (RamSprBuf[i+2] & 0x80) {
				spr_offs = i+5;
				break;
			}
		}
	}

	if (nSpriteEnable & 1) pgm_drawsprites(RamSprBuf,            RamSprBuf + spr_offs);
	if (nBurnLayer & 1) draw_background();
	if (nSpriteEnable & 2) pgm_drawsprites(RamSprBuf + spr_offs, RamSprBuf + 0xa00/2);
	if (nBurnLayer & 2) draw_text();

	BurnTransferCopy(RamCurPal);

	return 0;
}

void pgmInitDraw()
{
	GenericTilesInit();
	pTempDraw = (unsigned short*)malloc(0x400 * 0x200 * 2);
}

void pgmExitDraw()
{
	free (pTempDraw);
	GenericTilesExit();
}
