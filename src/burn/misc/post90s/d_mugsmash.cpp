// FB Alpha Mug Smashers driver module
// Based on MAME driver by David Haywood

#include "tiles_generic.h"
#include "burn_ym2151.h"
#include "msm6295.h"

static unsigned char *AllMem;
static unsigned char *MemEnd;
static unsigned char *AllRam;
static unsigned char *RamEnd;
static unsigned char *Drv68KROM;
static unsigned char *DrvZ80ROM;
static unsigned char *DrvGfxROM0;
static unsigned char *DrvGfxROM1;
static unsigned char *DrvSndROM;
static unsigned char *Drv68KRAM;
static unsigned char *DrvZ80RAM;
static unsigned char *DrvPalRAM;
static unsigned char *DrvSprRAM;
static unsigned char *DrvVidRAM0;
static unsigned char *DrvVidRAM1;
static unsigned char *DrvVidRegs;
static unsigned char *DrvSndRegs;

static unsigned int  *DrvPalette;
static unsigned int  *Palette;

static unsigned char  DrvRecalc;

static int           *DrvScrollX;
static int           *DrvScrollY;

static unsigned char  DrvReset;
static unsigned char  DrvJoy1[16];
static unsigned char  DrvJoy2[16];
static unsigned char  DrvDips[4];
static unsigned short DrvInps[4];

static unsigned char *soundlatch;

static struct BurnInputInfo MugsmashInputList[] = {
	{"P1 Coin",		BIT_DIGITAL,	DrvJoy1 + 8,	"p1 coin"},
	{"P1 Start",		BIT_DIGITAL,	DrvJoy1 + 7,	"p1 start"},
	{"P1 Up",		BIT_DIGITAL,	DrvJoy1 + 2,	"p1 up"},
	{"P1 Down",		BIT_DIGITAL,	DrvJoy1 + 3,	"p1 down"},
	{"P1 Left",		BIT_DIGITAL,	DrvJoy1 + 1,	"p1 left"},
	{"P1 Right",		BIT_DIGITAL,	DrvJoy1 + 0,	"p1 right"},
	{"P1 Button 1",		BIT_DIGITAL,	DrvJoy1 + 4,	"p1 fire 1"},
	{"P1 Button 2",		BIT_DIGITAL,	DrvJoy1 + 5,	"p1 fire 2"},
	{"P1 Button 3",		BIT_DIGITAL,	DrvJoy1 + 6,	"p1 fire 3"},

	{"P2 Coin",		BIT_DIGITAL,	DrvJoy1 + 9,	"p2 coin"},
	{"P2 Start",		BIT_DIGITAL,	DrvJoy2 + 7,	"p2 start"},
	{"P2 Up",		BIT_DIGITAL,	DrvJoy2 + 2,	"p2 up"},
	{"P2 Down",		BIT_DIGITAL,	DrvJoy2 + 3,	"p2 down"},
	{"P2 Left",		BIT_DIGITAL,	DrvJoy2 + 1,	"p2 left"},
	{"P2 Right",		BIT_DIGITAL,	DrvJoy2 + 0,	"p2 right"},
	{"P2 Button 1",		BIT_DIGITAL,	DrvJoy2 + 4,	"p2 fire 1"},
	{"P2 Button 2",		BIT_DIGITAL,	DrvJoy2 + 5,	"p2 fire 2"},
	{"P2 Button 3",		BIT_DIGITAL,	DrvJoy2 + 6,	"p2 fire 3"},

	{"Reset",		BIT_DIGITAL,	&DrvReset,	"reset"},
	{"Dip A",		BIT_DIPSWITCH,	DrvDips + 0,	"dip"},
	{"Dip B",		BIT_DIPSWITCH,	DrvDips + 1,	"dip"},
	{"Dip C",		BIT_DIPSWITCH,	DrvDips + 2,	"dip"},
	{"Dip D",		BIT_DIPSWITCH,	DrvDips + 3,	"dip"},
};

STDINPUTINFO(Mugsmash);

static struct BurnDIPInfo MugsmashDIPList[]=
{
	// Default Values
	{0x13, 0xff, 0xff, 0xff, NULL			},
	{0x14, 0xff, 0xff, 0xe1, NULL			},
	{0x15, 0xff, 0xff, 0xda, NULL			},
	{0x16, 0xff, 0xff, 0xff, NULL			},

	{0   , 0xfe, 0   ,    2, "Color Test"		},
	{0x13, 0x01, 0x10, 0x10, "Off"			},
	{0x13, 0x01, 0x10, 0x00, "On"			},

	{0   , 0xfe, 0   ,    2, "Draw SF."		},
	{0x13, 0x01, 0x20, 0x20, "Off"			},
	{0x13, 0x01, 0x20, 0x00, "On"			},

	{0   , 0xfe, 0   ,    2, "Service Mode"		},
	{0x14, 0x01, 0x01, 0x01, "Off"			},
	{0x14, 0x01, 0x01, 0x00, "On"			},

	{0   , 0xfe, 0   ,    8, "Coinage"		},
	{0x14, 0x01, 0x0e, 0x0c, "4 Coins 1 Credits "	},
	{0x14, 0x01, 0x0e, 0x0a, "3 Coins 1 Credits "	},
	{0x14, 0x01, 0x0e, 0x08, "2 Coins 1 Credits "	},
	{0x14, 0x01, 0x0e, 0x00, "1 Coin 1 Credits "	},
	{0x14, 0x01, 0x0e, 0x02, "1 Coin 2 Credits "	},
	{0x14, 0x01, 0x0e, 0x04, "1 Coin 3 Credits "	},
	{0x14, 0x01, 0x0e, 0x06, "1 Coin 4 Credits "	},
	{0x14, 0x01, 0x0e, 0x0e, "Free Play"		},

	{0   , 0xfe, 0   ,    2, "Allow_Continue"	},
	{0x14, 0x01, 0x10, 0x10, "No"			},
	{0x14, 0x01, 0x10, 0x00, "Yes"			},

	{0   , 0xfe, 0   ,    2, "Sound Test"		},
	{0x14, 0x01, 0x20, 0x20, "Off"			},
	{0x14, 0x01, 0x20, 0x00, "On"			},

	{0   , 0xfe, 0   ,    2, "Demo_Sounds"		},
	{0x15, 0x01, 0x01, 0x01, "Off"			},
	{0x15, 0x01, 0x01, 0x00, "On"			},

	{0   , 0xfe, 0   ,    4, "Lives"		},
	{0x15, 0x01, 0x06, 0x00, "1"			},
	{0x15, 0x01, 0x06, 0x02, "2"			},
	{0x15, 0x01, 0x06, 0x04, "3"			},
	{0x15, 0x01, 0x06, 0x06, "4"			},

	{0   , 0xfe, 0   ,    4, "Difficulty"		},
	{0x15, 0x01, 0x30, 0x00, "Very_Easy"		},
	{0x15, 0x01, 0x30, 0x10, "Easy"			},
	{0x15, 0x01, 0x30, 0x20, "Hard"			},
	{0x15, 0x01, 0x30, 0x30, "Very_Hard"		},

	{0   , 0xfe, 0   ,    2, "Draw Objects"		},
	{0x16, 0x01, 0x01, 0x01, "Off"			},
	{0x16, 0x01, 0x01, 0x00, "On"			},

	{0   , 0xfe, 0   ,    2, "Freeze"		},
	{0x16, 0x01, 0x02, 0x02, "Off"			},
	{0x16, 0x01, 0x02, 0x00, "On"			},
};

STDDIPINFO(Mugsmash);

unsigned char __fastcall mugsmash_read_byte(unsigned int address)
{
	switch (address)
	{
		case 0x180000:
		case 0x180001:
		case 0x180002:
		case 0x180003:
		case 0x180004:
		case 0x180005:
		case 0x180006:
		case 0x180007:
			return DrvInps[(address >> 1) & 3] >> ((~address & 1) << 3);
	}

	return 0;
}

unsigned short __fastcall mugsmash_read_word(unsigned int address)
{
	switch (address)
	{
		case 0x180000:
		case 0x180002:
		case 0x180004:
		case 0x180006:
			return DrvInps[(address >> 1) & 3];
	}

	return 0;
}

static void palette_write(int offset)
{
	unsigned short data = *((unsigned short*)(DrvPalRAM + offset));

	unsigned char r,g,b;

	r = (data >> 10);
	r = (r << 3) | (r >> 2);

	g = (data >>  5);
	g = (g << 3) | (r >> 2);

	b = (data >>  0);
	b = (b << 3) | (b >> 2);

	Palette[offset >> 1] = (r << 16) | (g << 8) | (b);
	DrvPalette[offset >> 1] = BurnHighCol(r, g, b, 0);
}

void __fastcall mugsmash_write_byte(unsigned int address, unsigned char data)
{
	if (address >= 0x100000 && address <= 0x1005ff) {
		DrvPalRAM[address & 0x7ff] = data;

		palette_write(address & 0x7ff);

		return;
	}

	switch (address)
	{
		case 0x0c0000:
		case 0x0c0001:
		case 0x0c0002:
		case 0x0c0003:
		case 0x0c0004:
		case 0x0c0005:
		case 0x0c0006:
		case 0x0c0007:
			DrvVidRegs[address & 7] = data;
		return;

		case 0x140002:
		case 0x140003:
			ZetNmi();
		case 0x140000:
		case 0x140001:
		case 0x140004:
		case 0x140005:
		case 0x140006:
		case 0x140007:
			DrvSndRegs[address & 7] = data;
		return;
	}
}

void __fastcall mugsmash_write_word(unsigned int address, unsigned short data)
{
	if (address >= 0x100000 && address <= 0x1005ff) {
		*((unsigned short*)(DrvPalRAM + (address & 0x7fe))) = data;

		palette_write(address & 0x7fe);

		return;
	}

	switch (address)
	{
		case 0x0c0000:
			*((unsigned short*)(DrvVidRegs + (address & 7))) = data;
			DrvScrollX[1] = *((unsigned short*)(DrvVidRegs + 4)) + 7;
		return;

		case 0x0c0002:
			*((unsigned short*)(DrvVidRegs + (address & 7))) = data;
			DrvScrollY[1] = *((unsigned short*)(DrvVidRegs + 6)) + 12;
		return;

		case 0x0c0004:
			*((unsigned short*)(DrvVidRegs + (address & 7))) = data;
			DrvScrollX[0] = *((unsigned short*)(DrvVidRegs + 0)) + 3;
		return;

		case 0x0c0006:
			*((unsigned short*)(DrvVidRegs + (address & 7))) = data;
			DrvScrollY[0] = *((unsigned short*)(DrvVidRegs + 2)) + 12;
		return;

		case 0x140002:
			ZetNmi();
		case 0x140000:
		case 0x140004:
		case 0x140006:
			*((unsigned short*)(DrvSndRegs + (address & 7))) = data;
		return;
	}
}

void __fastcall mugsmash_sound_write(unsigned short address, unsigned char data)
{
	switch (address)
	{
		case 0x8800:
			BurnYM2151SelectRegister(data);
		return;

		case 0x8801:
			BurnYM2151WriteRegister(data);
		return;

		case 0x9800:
			MSM6295Command(0, data);
		return;
	}
}

unsigned char __fastcall mugsmash_sound_read(unsigned short address)
{
	switch (address)
	{
		case 0x8801:
		return BurnYM2151ReadStatus();

		case 0x9800:
		return MSM6295ReadStatus(0);

		case 0xa000:
			return *soundlatch;
	}

	return 0;
}

static int DrvGfxDecode()
{
	int Plane0[4]  = { 0, 1, 2, 3 };
	int XOffs0[16] = { 16, 20, 24, 28, 0, 4, 8, 12, 48, 52, 56, 60, 32, 36, 40, 44 };
	int YOffs0[16] = { 0, 64, 128, 192, 256, 320, 384, 448, 512, 576, 640, 704, 768, 832, 896, 960 };

	int Plane1[4]  = { 0xc00000, 0x800000, 0x400000, 0x000000 };
	int XOffs1[16] = { 0, 1, 2, 3, 4, 5, 6, 7, 128, 129, 130, 131, 132, 133, 134, 135 };
	int YOffs1[16] = { 0, 8, 16, 24, 32, 40, 48, 56, 64, 72, 80, 88, 96, 104, 112, 120 };

	unsigned char *tmp = (unsigned char*)malloc(0x300000);
	if (tmp == NULL) {
		return 1;
	}

	memcpy (tmp, DrvGfxROM0, 0x300000);

	GfxDecode(0x6000, 4, 16, 16, Plane0, XOffs0, YOffs0, 0x400, tmp, DrvGfxROM0);

	memcpy (tmp, DrvGfxROM1, 0x200000);

	GfxDecode(0x4000, 4, 16, 16, Plane1, XOffs1, YOffs1, 0x100, tmp, DrvGfxROM1);

	free (tmp);

	return 0;
}

static int MemIndex()
{
	unsigned char *Next; Next = AllMem;

	Drv68KROM	= Next; Next += 0x080000;
	DrvZ80ROM	= Next; Next += 0x010000;

	MSM6295ROM	= Next;
	DrvSndROM	= Next; Next += 0x040000;

	DrvGfxROM0	= Next; Next += 0x600000;
	DrvGfxROM1	= Next; Next += 0x400000;

	DrvPalette	= (unsigned int*)Next; Next += 0x0300 * sizeof(int);

	AllRam		= Next;

	Drv68KRAM	= Next; Next += 0x010000;
	DrvPalRAM	= Next; Next += 0x000800;
	DrvSprRAM	= Next; Next += 0x004000;
	DrvVidRAM0	= Next; Next += 0x001000;
	DrvVidRAM1	= Next; Next += 0x001000;

	DrvVidRegs	= Next; Next += 0x000008;
	DrvSndRegs	= Next; Next += 0x000008;

	DrvScrollX	= (int*)Next; Next += 0x000002 * sizeof(int);
	DrvScrollY	= (int*)Next; Next += 0x000002 * sizeof(int);

	DrvZ80RAM	= Next; Next += 0x000800;

	Palette		= (unsigned int*)Next; Next += 0x0300 * sizeof(int);

	RamEnd		= Next;

	MemEnd		= Next;

	return 0;
}

static int DrvDoReset()
{
	DrvReset = 0;

	memset (AllRam, 0, RamEnd - AllRam);

	SekOpen(0);
	SekReset();
	SekClose();

	ZetOpen(0);
	ZetReset();
	ZetClose();

	MSM6295Reset(0);
	BurnYM2151Reset();

	return 0;
}

void MugsmashYM2151IrqHandler(int Irq)
{
	if (Irq) {
		ZetSetIRQLine(0xff, ZET_IRQSTATUS_ACK);
	} else {
		ZetSetIRQLine(0,    ZET_IRQSTATUS_NONE);
	}
}

static int DrvInit()
{
	AllMem = NULL;
	MemIndex();
	int nLen = MemEnd - (unsigned char *)0;
	if ((AllMem = (unsigned char *)malloc(nLen)) == NULL) return 1;
	memset(AllMem, 0, nLen);
	MemIndex();

	{
		if (BurnLoadRom(Drv68KROM + 1,		0, 2)) return 1;
		if (BurnLoadRom(Drv68KROM + 0,		1, 2)) return 1;

		if (BurnLoadRom(DrvZ80ROM,		2, 1)) return 1;

		for (int i = 0; i < 6; i+=2) {
			if (BurnLoadRom(DrvGfxROM0 + i * 0x80000 + 0, i +  3, 2)) return 1;
			if (BurnLoadRom(DrvGfxROM0 + i * 0x80000 + 1, i +  4, 2)) return 1;
		}

		for (int i = 0; i < 4; i++)
			if (BurnLoadRom(DrvGfxROM1 + i * 0x80000, i +  9, 1)) return 1;

		for (int i = 0; i < 2; i++)
			if (BurnLoadRom(DrvSndROM  + i * 0x20000, i + 13, 1)) return 1;

		DrvGfxDecode();
	}

	SekInit(0, 0x68000);
	SekOpen(0);
	SekMapMemory(Drv68KROM,		0x000000, 0x07ffff, SM_ROM);
	SekMapMemory(DrvVidRAM0,	0x080000, 0x080fff, SM_RAM);
	SekMapMemory(DrvVidRAM1,	0x082000, 0x082fff, SM_RAM);
	SekMapMemory(DrvPalRAM,		0x100000, 0x1005ff, SM_ROM);
	SekMapMemory(Drv68KRAM,		0x1c0000, 0x1cffff, SM_RAM);
	SekMapMemory(DrvSprRAM,		0x200000, 0x203fff, SM_RAM);
	SekSetWriteByteHandler(0,	mugsmash_write_byte);
	SekSetWriteWordHandler(0,	mugsmash_write_word);
	SekSetReadByteHandler(0,	mugsmash_read_byte);
	SekSetReadWordHandler(0,	mugsmash_read_word);
	SekClose();

	ZetInit(1);
	ZetOpen(0);
	ZetMapArea(0x0000, 0x7fff, 0, DrvZ80ROM);
	ZetMapArea(0x0000, 0x7fff, 2, DrvZ80ROM);
	ZetMapArea(0x8000, 0x87ff, 0, DrvZ80RAM);
	ZetMapArea(0x8000, 0x87ff, 1, DrvZ80RAM);
	ZetMapArea(0x8000, 0x87ff, 2, DrvZ80RAM);
	ZetSetWriteHandler(mugsmash_sound_write);
	ZetSetReadHandler(mugsmash_sound_read);
	ZetMemEnd();
	ZetClose();

	GenericTilesInit();

	soundlatch = DrvSndRegs + 2;

	BurnYM2151Init(3579545, 100.0);
	BurnYM2151SetIrqHandler(&MugsmashYM2151IrqHandler);

	MSM6295Init(0, 1122000 / 132, 50.0, 1);

	DrvDoReset();

	return 0;
}

static int DrvExit()
{
	GenericTilesExit();

	SekExit();
	ZetExit();

	MSM6295Exit(0);
	BurnYM2151Exit();

	free (AllMem);
	AllMem = NULL;

	return 0;
}

static void draw_sprites()
{
	const unsigned short *source = (const unsigned short*)DrvSprRAM;
	const unsigned short *finish = source + 0x2000;

	while (source < finish)
	{
		int xpos  = source[0] & 0x00ff;
		int ypos  = source[4] & 0x00ff;
		int num   = (source[3] & 0x00ff) | ((source[2] & 0x00ff) << 8);
		int attr  = source[1];
		int flipx = (attr & 0x0080)>>7;
		int color = attr & 0x000f;

		xpos += (attr & 0x0020) << 3;
		ypos += (attr & 0x0010) << 4;

		xpos -= 28;
		ypos -= 24;

		if (flipx) {
			Render16x16Tile_Mask_FlipX_Clip(pTransDraw, num, xpos, ypos, color, 4, 0, 0, DrvGfxROM0);
		} else {
			Render16x16Tile_Mask_Clip(pTransDraw, num, xpos, ypos, color, 4, 0, 0, DrvGfxROM0);
		}

		source += 8;
	}
}

static void draw_layer(unsigned char *source, int colofst, int scroll)
{
	unsigned short *vram = (unsigned short*)source;

	for (int offs = 0; offs < 0x400; offs++)
	{
		int sx = (offs & 0x1f) << 4;
		int sy = (offs >> 5) << 4;

		int code  = vram[(offs << 1) | 1];
		int color = vram[(offs << 1)];
		int flipx = color & 0x40;
		int flipy = color & 0x80;
		color &= 0x000f;

		sx -= DrvScrollX[scroll] & 0x1ff;
		sy -= DrvScrollY[scroll] & 0x1ff;
		if (sx < -15) sx += 0x200;
		if (sy < -15) sy += 0x200;

		if (flipy) {
			if (flipx) {
				Render16x16Tile_Mask_FlipXY_Clip(pTransDraw, code, sx, sy, color, 4, 0, colofst, DrvGfxROM1);
			} else {
				Render16x16Tile_Mask_FlipY_Clip(pTransDraw, code, sx, sy, color, 4, 0, colofst, DrvGfxROM1);
			}
		} else {
			if (flipx) {
				Render16x16Tile_Mask_FlipX_Clip(pTransDraw, code, sx, sy, color, 4, 0, colofst, DrvGfxROM1);
			} else {
				Render16x16Tile_Mask_Clip(pTransDraw, code, sx, sy, color, 4, 0, colofst, DrvGfxROM1);
			}
		}
	}
}

static int DrvDraw()
{
	if (DrvRecalc) {
		for (int i = 0; i < 0x300; i++) {
			int rgb = Palette[i];
			DrvPalette[i] = BurnHighCol(rgb >> 16, rgb >> 8, rgb, 0);
		}
	}

	memset (pTransDraw, 0, nScreenWidth * nScreenHeight * 2);

	draw_layer(DrvVidRAM1, 0x200, 1);
	draw_layer(DrvVidRAM0, 0x100, 0);

	draw_sprites();

	BurnTransferCopy(DrvPalette);

	return 0;
}

static int DrvFrame()
{
	if (DrvReset) {
		DrvDoReset();
	}

	{
		for (int i = 0; i < 4; i++) {
			DrvInps[i] = (DrvDips[i] << 8) | 0xff;
		}

		for (int i = 0; i < 16; i++) {
			DrvInps[0] ^= (DrvJoy1[i] & 1) << i;
			DrvInps[1] ^= (DrvJoy2[i] & 1) << i;
		}	
	}

	int nInterleave = 10;
	int nCyclesTotal[2] = { 12000000 / 60, 4000000 / 60 };
	int nCyclesDone[2] = { 0, 0 };

	SekOpen(0);
	ZetOpen(0);

	for (int i = 0; i < nInterleave; i++)
	{
		int nSegment;

		nSegment = (nCyclesTotal[0] - nCyclesDone[0]) / (nInterleave - i);
		nCyclesDone[0] += SekRun(nSegment);
		if (i == (nInterleave - 1)) SekSetIRQLine(6, SEK_IRQSTATUS_AUTO);

		nSegment = (nCyclesTotal[1] - nCyclesDone[1]) / (nInterleave - i);
		nCyclesDone[1] += ZetRun(nSegment);

		nSegment = nBurnSoundLen / nInterleave;

		BurnYM2151Render(pBurnSoundOut + nSegment * i, nSegment);

		if (pBurnSoundOut) {
			MSM6295Render(0, pBurnSoundOut + nSegment * i, nSegment);
		}
	}

	ZetClose();
	SekClose();

	if (pBurnDraw) {
		DrvDraw();
	}

	return 0;
}

static int DrvScan(int nAction, int *pnMin)
{
	struct BurnArea ba;
	
	if (pnMin != NULL) {
		*pnMin = 0x029697;
	}

	if (nAction & ACB_MEMORY_RAM) {
		memset(&ba, 0, sizeof(ba));
		ba.Data	  = AllRam;
		ba.nLen	  = RamEnd-AllRam;
		ba.szName = "All Ram";
		BurnAcb(&ba);
	}

	if (nAction & ACB_DRIVER_DATA) {
		SekScan(nAction);
		ZetScan(nAction);

		BurnYM2151Scan(nAction);
		MSM6295Scan(0, nAction);
	}

	return 0;
}


// Mug Smashers

static struct BurnRomInfo mugsmashRomDesc[] = {
	{ "mugs_04.bin",	0x40000, 0x2498fd27, 1 | BRF_PRG | BRF_ESS }, //  0 68000 Code
	{ "mugs_05.bin",	0x40000, 0x95efb40b, 1 | BRF_PRG | BRF_ESS }, //  1

	{ "mugs_03.bin",	0x10000, 0x0101df2d, 2 | BRF_PRG | BRF_ESS }, //  2 Z80 Code

	{ "mugs_11.bin",	0x80000, 0x1c9f5acf, 3 | BRF_GRA },           //  3 Sprites
	{ "mugs_10.bin",	0x80000, 0x6b3c22d9, 3 | BRF_GRA },           //  4
	{ "mugs_09.bin",	0x80000, 0x4e9490f3, 3 | BRF_GRA },           //  5
	{ "mugs_08.bin",	0x80000, 0x716328d5, 3 | BRF_GRA },           //  6
	{ "mugs_07.bin",	0x80000, 0x9e3167fd, 3 | BRF_GRA },           //  7
	{ "mugs_06.bin",	0x80000, 0x8df75d29, 3 | BRF_GRA },           //  8

	{ "mugs_12.bin",	0x80000, 0xc0a6ed98, 4 | BRF_GRA },           //  9 Bg Tiles
	{ "mugs_13.bin",	0x80000, 0xe2be8595, 4 | BRF_GRA },           // 10
	{ "mugs_14.bin",	0x80000, 0x24e81068, 4 | BRF_GRA },           // 11
	{ "mugs_15.bin",	0x80000, 0x82e8187c, 4 | BRF_GRA },           // 12

	{ "mugs_02.bin",	0x20000, 0xf92a7f4a, 5 | BRF_GRA },           // 13 Samples
	{ "mugs_01.bin",	0x20000, 0x1a3a0b39, 5 | BRF_GRA },           // 14
};

STD_ROM_PICK(mugsmash);
STD_ROM_FN(mugsmash);

struct BurnDriver BurnDrvMugsmash = {
	"mugsmash", NULL, NULL, "1990?",
	"Mug Smashers\0", NULL, "Electronic Devices Italy / 3D Games England", "Miscellaneous",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING, 2, HARDWARE_MISC_POST90S, //GBF_SCRFIGHT, 0,
	NULL, mugsmashRomInfo, mugsmashRomName, MugsmashInputInfo, MugsmashDIPInfo,
	DrvInit, DrvExit, DrvFrame, DrvDraw, DrvScan, 0, NULL, NULL, NULL, &DrvRecalc,
	320, 240, 4, 3
};
