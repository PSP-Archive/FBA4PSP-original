#include "tiles_generic.h"
#include "hd6309_intf.h"
#include "m6800_intf.h"
#include "m6809_intf.h"
#include "burn_ym2151.h"
#include "msm5205.h"
#include "msm6295.h"

static unsigned char DrvInputPort0[8] = {0, 0, 0, 0, 0, 0, 0, 0};
static unsigned char DrvInputPort1[8] = {0, 0, 0, 0, 0, 0, 0, 0};
static unsigned char DrvInputPort2[8] = {0, 0, 0, 0, 0, 0, 0, 0};
static unsigned char DrvDip[2]        = {0, 0};
static unsigned char DrvInput[3]      = {0x00, 0x00, 0x00};
static unsigned char DrvReset         = 0;

static unsigned char *Mem                 = NULL;
static unsigned char *MemEnd              = NULL;
static unsigned char *RamStart            = NULL;
static unsigned char *RamEnd              = NULL;
static unsigned char *DrvHD6309Rom        = NULL;
static unsigned char *DrvSubCPURom        = NULL;
static unsigned char *DrvSoundCPURom      = NULL;
static unsigned char *DrvHD6309Ram        = NULL;
static unsigned char *DrvSubCPURam        = NULL;
static unsigned char *DrvSoundCPURam      = NULL;
static unsigned char *DrvFgVideoRam       = NULL;
static unsigned char *DrvBgVideoRam       = NULL;
static unsigned char *DrvSpriteRam        = NULL;
static unsigned char *DrvPaletteRam1      = NULL;
static unsigned char *DrvPaletteRam2      = NULL;
static unsigned char *DrvChars            = NULL;
static unsigned char *DrvTiles            = NULL;
static unsigned char *DrvSprites          = NULL;
static unsigned char *DrvTempRom          = NULL;
static unsigned int  *DrvPalette          = NULL;

static unsigned char DrvRomBank;
static unsigned char DrvVBlank;
static unsigned char DrvSubCPUBusy;
static unsigned char DrvSoundLatch;
static unsigned char DrvADPCMIdle[2];
static UINT32 DrvADPCMPos[2];
static UINT32 DrvADPCMEnd[2];

static unsigned short DrvScrollXHi;
static unsigned short DrvScrollYHi;
static unsigned char  DrvScrollXLo;
static unsigned char  DrvScrollYLo;

static int nCyclesDone[3], nCyclesTotal[3];
static int nCyclesSegment;

#define DD_CPU_TYPE_NONE		0
#define DD_CPU_TYPE_HD63701		1
#define DD_CPU_TYPE_HD6309		2
#define DD_CPU_TYPE_M6803		3
#define DD_CPU_TYPE_Z80			4
#define DD_CPU_TYPE_M6809		5
static int DrvSubCPUType;
static int DrvSoundCPUType;

#define DD_VID_TYPE_DD2			1
static int DrvVidHardwareType;

static struct BurnInputInfo DrvInputList[] =
{
	{"Coin 1"            , BIT_DIGITAL  , DrvInputPort1 + 6, "p1 coin"   },
	{"Start 1"           , BIT_DIGITAL  , DrvInputPort0 + 6, "p1 start"  },
	{"Coin 2"            , BIT_DIGITAL  , DrvInputPort1 + 7, "p2 coin"   },
	{"Start 2"           , BIT_DIGITAL  , DrvInputPort0 + 7, "p2 start"  },

	{"P1 Up"             , BIT_DIGITAL  , DrvInputPort0 + 2, "p1 up"     },
	{"P1 Down"           , BIT_DIGITAL  , DrvInputPort0 + 3, "p1 down"   },
	{"P1 Left"           , BIT_DIGITAL  , DrvInputPort0 + 1, "p1 left"   },
	{"P1 Right"          , BIT_DIGITAL  , DrvInputPort0 + 0, "p1 right"  },
	{"P1 Fire 1"         , BIT_DIGITAL  , DrvInputPort0 + 4, "p1 fire 1" },
	{"P1 Fire 2"         , BIT_DIGITAL  , DrvInputPort0 + 5, "p1 fire 2" },
	{"P1 Fire 3"         , BIT_DIGITAL  , DrvInputPort2 + 1, "p1 fire 3" },
	
	{"P2 Up"             , BIT_DIGITAL  , DrvInputPort1 + 2, "p2 up"     },
	{"P2 Down"           , BIT_DIGITAL  , DrvInputPort1 + 3, "p2 down"   },
	{"P2 Left"           , BIT_DIGITAL  , DrvInputPort1 + 1, "p2 left"   },
	{"P2 Right"          , BIT_DIGITAL  , DrvInputPort1 + 0, "p2 right"  },
	{"P2 Fire 1"         , BIT_DIGITAL  , DrvInputPort1 + 4, "p2 fire 1" },
	{"P2 Fire 2"         , BIT_DIGITAL  , DrvInputPort1 + 5, "p2 fire 2" },
	{"P2 Fire 3"         , BIT_DIGITAL  , DrvInputPort2 + 2, "p2 fire 3" },

	{"Reset"             , BIT_DIGITAL  , &DrvReset        , "reset"     },
	{"Service"           , BIT_DIGITAL  , DrvInputPort2 + 0, "service"   },
	{"Dip 1"             , BIT_DIPSWITCH, DrvDip + 0       , "dip"       },
	{"Dip 2"             , BIT_DIPSWITCH, DrvDip + 1       , "dip"       },
};

STDINPUTINFO(Drv);

static inline void DrvMakeInputs()
{
	// Reset Inputs
	DrvInput[0] = DrvInput[1] = 0xff;
	DrvInput[2] = 0xe7;

	// Compile Digital Inputs
	for (int i = 0; i < 8; i++) {
		DrvInput[0] -= (DrvInputPort0[i] & 1) << i;
		DrvInput[1] -= (DrvInputPort1[i] & 1) << i;
		DrvInput[2] -= (DrvInputPort2[i] & 1) << i;
	}
}

static struct BurnDIPInfo DrvDIPList[]=
{
	// Default Values
	{0x14, 0xff, 0xff, 0xff, NULL                     },
	{0x15, 0xff, 0xff, 0xff, NULL                     },
	
	// Dip 1
	{0   , 0xfe, 0   , 8   , "Coin A"                 },
	{0x14, 0x01, 0x07, 0x00, "4 Coins 1 Play"         },
	{0x14, 0x01, 0x07, 0x01, "3 Coins 1 Play"         },
	{0x14, 0x01, 0x07, 0x02, "2 Coins 1 Play"         },
	{0x14, 0x01, 0x07, 0x07, "1 Coin  1 Play"         },
	{0x14, 0x01, 0x07, 0x06, "1 Coin  2 Plays"        },
	{0x14, 0x01, 0x07, 0x05, "1 Coin  3 Plays"        },
	{0x14, 0x01, 0x07, 0x04, "1 Coin  4 Plays"        },
	{0x14, 0x01, 0x07, 0x03, "1 Coin  5 Plays"        },
	
	{0   , 0xfe, 0   , 8   , "Coin B"                 },
	{0x14, 0x01, 0x38, 0x00, "4 Coins 1 Play"         },
	{0x14, 0x01, 0x38, 0x08, "3 Coins 1 Play"         },
	{0x14, 0x01, 0x38, 0x10, "2 Coins 1 Play"         },
	{0x14, 0x01, 0x38, 0x38, "1 Coin  1 Play"         },
	{0x14, 0x01, 0x38, 0x30, "1 Coin  2 Plays"        },
	{0x14, 0x01, 0x38, 0x28, "1 Coin  3 Plays"        },
	{0x14, 0x01, 0x38, 0x20, "1 Coin  4 Plays"        },
	{0x14, 0x01, 0x38, 0x18, "1 Coin  5 Plays"        },
	
	{0   , 0xfe, 0   , 2   , "Cabinet"                },
	{0x14, 0x01, 0x40, 0x40, "Upright"                },
	{0x14, 0x01, 0x40, 0x00, "Cocktail"               },
	
	{0   , 0xfe, 0   , 2   , "Flip Screen"            },
	{0x14, 0x01, 0x80, 0x80, "Off"                    },
	{0x14, 0x01, 0x80, 0x00, "On"                     },
	
	// Dip 2
	{0   , 0xfe, 0   , 4   , "Difficulty"             },
	{0x15, 0x01, 0x03, 0x01, "Easy"                   },
	{0x15, 0x01, 0x03, 0x03, "Medium"                 },
	{0x15, 0x01, 0x03, 0x02, "Hard"                   },
	{0x15, 0x01, 0x03, 0x00, "Hardest"                },
	
	{0   , 0xfe, 0   , 2   , "Demo Sounds"            },
	{0x15, 0x01, 0x04, 0x00, "Off"                    },
	{0x15, 0x01, 0x04, 0x04, "On"                     },
	
	{0   , 0xfe, 0   , 4   , "Bonus Life"             },
	{0x15, 0x01, 0x30, 0x10, "20k"                    },
	{0x15, 0x01, 0x30, 0x00, "40k"                    },
	{0x15, 0x01, 0x30, 0x30, "30k and every 60k"      },
	{0x15, 0x01, 0x30, 0x20, "40k and every 80k"      },	
	
	{0   , 0xfe, 0   , 4   , "Lives"                  },
	{0x15, 0x01, 0xc0, 0xc0, "2"                      },
	{0x15, 0x01, 0xc0, 0x80, "3"                      },
	{0x15, 0x01, 0xc0, 0x40, "4"                      },
	{0x15, 0x01, 0xc0, 0x00, "Infinite"               },
};

STDDIPINFO(Drv);

static struct BurnDIPInfo Drv2DIPList[]=
{
	// Default Values
	{0x14, 0xff, 0xff, 0xff, NULL                     },
	{0x15, 0xff, 0xff, 0xff, NULL                     },
	
	// Dip 1
	{0   , 0xfe, 0   , 8   , "Coin A"                 },
	{0x14, 0x01, 0x07, 0x00, "4 Coins 1 Play"         },
	{0x14, 0x01, 0x07, 0x01, "3 Coins 1 Play"         },
	{0x14, 0x01, 0x07, 0x02, "2 Coins 1 Play"         },
	{0x14, 0x01, 0x07, 0x07, "1 Coin  1 Play"         },
	{0x14, 0x01, 0x07, 0x06, "1 Coin  2 Plays"        },
	{0x14, 0x01, 0x07, 0x05, "1 Coin  3 Plays"        },
	{0x14, 0x01, 0x07, 0x04, "1 Coin  4 Plays"        },
	{0x14, 0x01, 0x07, 0x03, "1 Coin  5 Plays"        },
	
	{0   , 0xfe, 0   , 8   , "Coin B"                 },
	{0x14, 0x01, 0x38, 0x00, "4 Coins 1 Play"         },
	{0x14, 0x01, 0x38, 0x08, "3 Coins 1 Play"         },
	{0x14, 0x01, 0x38, 0x10, "2 Coins 1 Play"         },
	{0x14, 0x01, 0x38, 0x38, "1 Coin  1 Play"         },
	{0x14, 0x01, 0x38, 0x30, "1 Coin  2 Plays"        },
	{0x14, 0x01, 0x38, 0x28, "1 Coin  3 Plays"        },
	{0x14, 0x01, 0x38, 0x20, "1 Coin  4 Plays"        },
	{0x14, 0x01, 0x38, 0x18, "1 Coin  5 Plays"        },
	
	{0   , 0xfe, 0   , 2   , "Cabinet"                },
	{0x14, 0x01, 0x40, 0x40, "Upright"                },
	{0x14, 0x01, 0x40, 0x00, "Cocktail"               },
	
	{0   , 0xfe, 0   , 2   , "Flip Screen"            },
	{0x14, 0x01, 0x80, 0x80, "Off"                    },
	{0x14, 0x01, 0x80, 0x00, "On"                     },
	
	// Dip 2
	{0   , 0xfe, 0   , 2   , "Hurricane Kick"         },
	{0x15, 0x01, 0x08, 0x00, "Easy"                   },
	{0x15, 0x01, 0x08, 0x08, "Normal"                 },
	
	{0   , 0xfe, 0   , 4   , "Timer"                  },
	{0x15, 0x01, 0x30, 0x00, "60"                     },
	{0x15, 0x01, 0x30, 0x10, "65"                     },
	{0x15, 0x01, 0x30, 0x30, "70"                     },
	{0x15, 0x01, 0x30, 0x20, "80"                     },
	
	{0   , 0xfe, 0   , 4   , "Lives"                  },
	{0x15, 0x01, 0xc0, 0xc0, "1"                      },
	{0x15, 0x01, 0xc0, 0x80, "2"                      },
	{0x15, 0x01, 0xc0, 0x40, "3"                      },
	{0x15, 0x01, 0xc0, 0x00, "4"                      },
};

STDDIPINFO(Drv2);

static struct BurnRomInfo DrvRomDesc[] = {
	{ "21j-1-5.26",    0x08000, 0x42045dfd, BRF_ESS | BRF_PRG }, //  0	HD6309 Program Code
	{ "21j-2-3.25",    0x08000, 0x5779705e, BRF_ESS | BRF_PRG }, //  1
	{ "21j-3.24",      0x08000, 0x3bdea613, BRF_ESS | BRF_PRG }, //  2
	{ "21j-4-1.23",    0x08000, 0x728f87b9, BRF_ESS | BRF_PRG }, //  3
	
	{ "63701.bin",     0x04000, 0xf5232d03, BRF_ESS | BRF_PRG }, //  4	HD63701 Program Code
	
	{ "21j-0-1",       0x08000, 0x9efa95bb, BRF_ESS | BRF_PRG }, //  5	M6809 Program Code
	
	{ "21j-5",         0x08000, 0x7a8b8db4, BRF_GRA },	     //  6	Characters
	
	{ "21j-a",         0x10000, 0x574face3, BRF_GRA },	     //  7	Sprites
	{ "21j-b",         0x10000, 0x40507a76, BRF_GRA },	     //  8
	{ "21j-c",         0x10000, 0xbb0bc76f, BRF_GRA },	     //  9
	{ "21j-d",         0x10000, 0xcb4f231b, BRF_GRA },	     //  10
	{ "21j-e",         0x10000, 0xa0a0c261, BRF_GRA },	     //  11
	{ "21j-f",         0x10000, 0x6ba152f6, BRF_GRA },	     //  12
	{ "21j-g",         0x10000, 0x3220a0b6, BRF_GRA },	     //  13
	{ "21j-h",         0x10000, 0x65c7517d, BRF_GRA },	     //  14
	
	{ "21j-8",         0x10000, 0x7c435887, BRF_GRA },	     //  15	Tiles
	{ "21j-9",         0x10000, 0xc6640aed, BRF_GRA },	     //  16
	{ "21j-i",         0x10000, 0x5effb0a0, BRF_GRA },	     //  17
	{ "21j-j",         0x10000, 0x5fb42e7c, BRF_GRA },	     //  18
	
	{ "21j-6",         0x10000, 0x34755de3, BRF_GRA },	     //  19	Samples
	{ "21j-7",         0x10000, 0x904de6f8, BRF_GRA },	     //  20
	
	{ "21j-k-0",       0x00100, 0xfdb130a9, BRF_GRA },	     //  21	PROMs
	{ "21j-l-0",       0x00200, 0x46339529, BRF_GRA },	     //  22
};

STD_ROM_PICK(Drv);
STD_ROM_FN(Drv);

static struct BurnRomInfo DrvwRomDesc[] = {
	{ "21j-1.26",      0x08000, 0xae714964, BRF_ESS | BRF_PRG }, //  0	HD6309 Program Code
	{ "21j-2-3.25",    0x08000, 0x5779705e, BRF_ESS | BRF_PRG }, //  1
	{ "21a-3.24",      0x08000, 0xdbf24897, BRF_ESS | BRF_PRG }, //  2
	{ "21j-4.23",      0x08000, 0x6c9f46fa, BRF_ESS | BRF_PRG }, //  3
	
	{ "63701.bin",     0x04000, 0xf5232d03, BRF_ESS | BRF_PRG }, //  4	HD63701 Program Code
	
	{ "21j-0-1",       0x08000, 0x9efa95bb, BRF_ESS | BRF_PRG }, //  5	M6809 Program Code
	
	{ "21j-5",         0x08000, 0x7a8b8db4, BRF_GRA },	     //  6	Characters
	
	{ "21j-a",         0x10000, 0x574face3, BRF_GRA },	     //  7	Sprites
	{ "21j-b",         0x10000, 0x40507a76, BRF_GRA },	     //  8
	{ "21j-c",         0x10000, 0xbb0bc76f, BRF_GRA },	     //  9
	{ "21j-d",         0x10000, 0xcb4f231b, BRF_GRA },	     //  10
	{ "21j-e",         0x10000, 0xa0a0c261, BRF_GRA },	     //  11
	{ "21j-f",         0x10000, 0x6ba152f6, BRF_GRA },	     //  12
	{ "21j-g",         0x10000, 0x3220a0b6, BRF_GRA },	     //  13
	{ "21j-h",         0x10000, 0x65c7517d, BRF_GRA },	     //  14
	
	{ "21j-8",         0x10000, 0x7c435887, BRF_GRA },	     //  15	Tiles
	{ "21j-9",         0x10000, 0xc6640aed, BRF_GRA },	     //  16
	{ "21j-i",         0x10000, 0x5effb0a0, BRF_GRA },	     //  17
	{ "21j-j",         0x10000, 0x5fb42e7c, BRF_GRA },	     //  18
	
	{ "21j-6",         0x10000, 0x34755de3, BRF_GRA },	     //  19	Samples
	{ "21j-7",         0x10000, 0x904de6f8, BRF_GRA },	     //  20
	
	{ "21j-k-0",       0x00100, 0xfdb130a9, BRF_GRA },	     //  21	PROMs
	{ "21j-l-0",       0x00200, 0x46339529, BRF_GRA },	     //  22
};

STD_ROM_PICK(Drvw);
STD_ROM_FN(Drvw);

static struct BurnRomInfo Drvw1RomDesc[] = {
	{ "e1-1.26",       0x08000, 0x4b951643, BRF_ESS | BRF_PRG }, //  0	HD6309 Program Code
	{ "21a-2-4.25",    0x08000, 0x5cd67657, BRF_ESS | BRF_PRG }, //  1
	{ "21a-3.24",      0x08000, 0xdbf24897, BRF_ESS | BRF_PRG }, //  2
	{ "e4-1.23",       0x08000, 0xb1e26935, BRF_ESS | BRF_PRG }, //  3
	
	{ "63701.bin",     0x04000, 0xf5232d03, BRF_ESS | BRF_PRG }, //  4	HD63701 Program Code
	
	{ "21j-0-1",       0x08000, 0x9efa95bb, BRF_ESS | BRF_PRG }, //  5	M6809 Program Code
	
	{ "21j-5",         0x08000, 0x7a8b8db4, BRF_GRA },	     //  6	Characters
	
	{ "21j-a",         0x10000, 0x574face3, BRF_GRA },	     //  7	Sprites
	{ "21j-b",         0x10000, 0x40507a76, BRF_GRA },	     //  8
	{ "21j-c",         0x10000, 0xbb0bc76f, BRF_GRA },	     //  9
	{ "21j-d",         0x10000, 0xcb4f231b, BRF_GRA },	     //  10
	{ "21j-e",         0x10000, 0xa0a0c261, BRF_GRA },	     //  11
	{ "21j-f",         0x10000, 0x6ba152f6, BRF_GRA },	     //  12
	{ "21j-g",         0x10000, 0x3220a0b6, BRF_GRA },	     //  13
	{ "21j-h",         0x10000, 0x65c7517d, BRF_GRA },	     //  14
	
	{ "21j-8",         0x10000, 0x7c435887, BRF_GRA },	     //  15	Tiles
	{ "21j-9",         0x10000, 0xc6640aed, BRF_GRA },	     //  16
	{ "21j-i",         0x10000, 0x5effb0a0, BRF_GRA },	     //  17
	{ "21j-j",         0x10000, 0x5fb42e7c, BRF_GRA },	     //  18
	
	{ "21j-6",         0x10000, 0x34755de3, BRF_GRA },	     //  19	Samples
	{ "21j-7",         0x10000, 0x904de6f8, BRF_GRA },	     //  20
	
	{ "21j-k-0",       0x00100, 0xfdb130a9, BRF_GRA },	     //  21	PROMs
	{ "21j-l-0",       0x00200, 0x46339529, BRF_GRA },	     //  22
};

STD_ROM_PICK(Drvw1);
STD_ROM_FN(Drvw1);

static struct BurnRomInfo DrvuRomDesc[] = {
	{ "21a-1-5.26",    0x08000, 0xe24a6e11, BRF_ESS | BRF_PRG }, //  0	HD6309 Program Code
	{ "21j-2-3.25",    0x08000, 0x5779705e, BRF_ESS | BRF_PRG }, //  1
	{ "21a-3.24",      0x08000, 0xdbf24897, BRF_ESS | BRF_PRG }, //  2
	{ "21a-4.23",      0x08000, 0x6ea16072, BRF_ESS | BRF_PRG }, //  3
	
	{ "63701.bin",     0x04000, 0xf5232d03, BRF_ESS | BRF_PRG }, //  4	HD63701 Program Code
	
	{ "21j-0-1",       0x08000, 0x9efa95bb, BRF_ESS | BRF_PRG }, //  5	M6809 Program Code
	
	{ "21j-5",         0x08000, 0x7a8b8db4, BRF_GRA },	     //  6	Characters
	
	{ "21j-a",         0x10000, 0x574face3, BRF_GRA },	     //  7	Sprites
	{ "21j-b",         0x10000, 0x40507a76, BRF_GRA },	     //  8
	{ "21j-c",         0x10000, 0xbb0bc76f, BRF_GRA },	     //  9
	{ "21j-d",         0x10000, 0xcb4f231b, BRF_GRA },	     //  10
	{ "21j-e",         0x10000, 0xa0a0c261, BRF_GRA },	     //  11
	{ "21j-f",         0x10000, 0x6ba152f6, BRF_GRA },	     //  12
	{ "21j-g",         0x10000, 0x3220a0b6, BRF_GRA },	     //  13
	{ "21j-h",         0x10000, 0x65c7517d, BRF_GRA },	     //  14
	
	{ "21j-8",         0x10000, 0x7c435887, BRF_GRA },	     //  15	Tiles
	{ "21j-9",         0x10000, 0xc6640aed, BRF_GRA },	     //  16
	{ "21j-i",         0x10000, 0x5effb0a0, BRF_GRA },	     //  17
	{ "21j-j",         0x10000, 0x5fb42e7c, BRF_GRA },	     //  18
	
	{ "21j-6",         0x10000, 0x34755de3, BRF_GRA },	     //  19	Samples
	{ "21j-7",         0x10000, 0x904de6f8, BRF_GRA },	     //  20
	
	{ "21j-k-0",       0x00100, 0xfdb130a9, BRF_GRA },	     //  21	PROMs
	{ "21j-l-0",       0x00200, 0x46339529, BRF_GRA },	     //  22
};

STD_ROM_PICK(Drvu);
STD_ROM_FN(Drvu);

static struct BurnRomInfo DrvuaRomDesc[] = {
	{ "21a-1",         0x08000, 0x1d625008, BRF_ESS | BRF_PRG }, //  0	HD6309 Program Code
	{ "21a-2_4",       0x08000, 0x5cd67657, BRF_ESS | BRF_PRG }, //  1
	{ "21a-3",         0x08000, 0xdbf24897, BRF_ESS | BRF_PRG }, //  2
	{ "21a-4_2",       0x08000, 0x9b019598, BRF_ESS | BRF_PRG }, //  3
	
	{ "63701.bin",     0x04000, 0xf5232d03, BRF_ESS | BRF_PRG }, //  4	HD63701 Program Code
	
	{ "21j-0-1",       0x08000, 0x9efa95bb, BRF_ESS | BRF_PRG }, //  5	M6809 Program Code
	
	{ "21j-5",         0x08000, 0x7a8b8db4, BRF_GRA },	     //  6	Characters
	
	{ "21j-a",         0x10000, 0x574face3, BRF_GRA },	     //  7	Sprites
	{ "21j-b",         0x10000, 0x40507a76, BRF_GRA },	     //  8
	{ "21j-c",         0x10000, 0xbb0bc76f, BRF_GRA },	     //  9
	{ "21j-d",         0x10000, 0xcb4f231b, BRF_GRA },	     //  10
	{ "21j-e",         0x10000, 0xa0a0c261, BRF_GRA },	     //  11
	{ "21j-f",         0x10000, 0x6ba152f6, BRF_GRA },	     //  12
	{ "21j-g",         0x10000, 0x3220a0b6, BRF_GRA },	     //  13
	{ "21j-h",         0x10000, 0x65c7517d, BRF_GRA },	     //  14
	
	{ "21j-8",         0x10000, 0x7c435887, BRF_GRA },	     //  15	Tiles
	{ "21j-9",         0x10000, 0xc6640aed, BRF_GRA },	     //  16
	{ "21j-i",         0x10000, 0x5effb0a0, BRF_GRA },	     //  17
	{ "21j-j",         0x10000, 0x5fb42e7c, BRF_GRA },	     //  18
	
	{ "21j-6",         0x10000, 0x34755de3, BRF_GRA },	     //  19	Samples
	{ "21j-7",         0x10000, 0x904de6f8, BRF_GRA },	     //  20
	
	{ "21j-k-0",       0x00100, 0xfdb130a9, BRF_GRA },	     //  21	PROMs
	{ "21j-l-0",       0x00200, 0x46339529, BRF_GRA },	     //  22
};

STD_ROM_PICK(Drvua);
STD_ROM_FN(Drvua);

static struct BurnRomInfo Drvb2RomDesc[] = {
	{ "4.bin",         0x08000, 0x668dfa19, BRF_ESS | BRF_PRG }, //  0	HD6309 Program Code
	{ "5.bin",         0x08000, 0x5779705e, BRF_ESS | BRF_PRG }, //  1
	{ "6.bin",         0x08000, 0x3bdea613, BRF_ESS | BRF_PRG }, //  2
	{ "7.bin",         0x08000, 0x728f87b9, BRF_ESS | BRF_PRG }, //  3
	
	{ "63701.bin",     0x04000, 0xf5232d03, BRF_ESS | BRF_PRG }, //  4	HD63701 Program Code
	
	{ "3.bin",         0x08000, 0x9efa95bb, BRF_ESS | BRF_PRG }, //  5	M6809 Program Code
	
	{ "8.bin",         0x08000, 0x7a8b8db4, BRF_GRA },	     //  6	Characters
	
	{ "11.bin",        0x10000, 0x574face3, BRF_GRA },	     //  7	Sprites
	{ "12.bin",        0x10000, 0x40507a76, BRF_GRA },	     //  8
	{ "13.bin",        0x10000, 0xc8b91e17, BRF_GRA },	     //  9
	{ "14.bin",        0x10000, 0xcb4f231b, BRF_GRA },	     //  10
	{ "15.bin",        0x10000, 0xa0a0c261, BRF_GRA },	     //  11
	{ "16.bin",        0x10000, 0x6ba152f6, BRF_GRA },	     //  12
	{ "17.bin",        0x10000, 0x3220a0b6, BRF_GRA },	     //  13
	{ "18.bin",        0x10000, 0x65c7517d, BRF_GRA },	     //  14
	
	{ "9.bin",         0x10000, 0x7c435887, BRF_GRA },	     //  15	Tiles
	{ "10.bin",        0x10000, 0xc6640aed, BRF_GRA },	     //  16
	{ "19.bin",        0x10000, 0x22d65df2, BRF_GRA },	     //  17
	{ "20.bin",        0x10000, 0x5fb42e7c, BRF_GRA },	     //  18
	
	{ "1.bin",         0x10000, 0x34755de3, BRF_GRA },	     //  19	Samples
	{ "2.bin",         0x10000, 0x904de6f8, BRF_GRA },	     //  20
	
	{ "21j-k-0",       0x00100, 0xfdb130a9, BRF_GRA },	     //  21	PROMs
	{ "21j-l-0",       0x00200, 0x46339529, BRF_GRA },	     //  22
};

STD_ROM_PICK(Drvb2);
STD_ROM_FN(Drvb2);

static struct BurnRomInfo DrvbRomDesc[] = {
	{ "21j-1.26",      0x08000, 0xae714964, BRF_ESS | BRF_PRG }, //  0	HD6309 Program Code
	{ "21j-2-3.25",    0x08000, 0x5779705e, BRF_ESS | BRF_PRG }, //  1
	{ "21a-3.24",      0x08000, 0xdbf24897, BRF_ESS | BRF_PRG }, //  2
	{ "21j-4.23",      0x08000, 0x6c9f46fa, BRF_ESS | BRF_PRG }, //  3
	
	{ "ic38",          0x04000, 0x6a6a0325, BRF_ESS | BRF_PRG }, //  4	HD6309 Program Code
	
	{ "21j-0-1",       0x08000, 0x9efa95bb, BRF_ESS | BRF_PRG }, //  5	M6809 Program Code
	
	{ "21j-5",         0x08000, 0x7a8b8db4, BRF_GRA },	     //  6	Characters
	
	{ "21j-a",         0x10000, 0x574face3, BRF_GRA },	     //  7	Sprites
	{ "21j-b",         0x10000, 0x40507a76, BRF_GRA },	     //  8
	{ "21j-c",         0x10000, 0xbb0bc76f, BRF_GRA },	     //  9
	{ "21j-d",         0x10000, 0xcb4f231b, BRF_GRA },	     //  10
	{ "21j-e",         0x10000, 0xa0a0c261, BRF_GRA },	     //  11
	{ "21j-f",         0x10000, 0x6ba152f6, BRF_GRA },	     //  12
	{ "21j-g",         0x10000, 0x3220a0b6, BRF_GRA },	     //  13
	{ "21j-h",         0x10000, 0x65c7517d, BRF_GRA },	     //  14
	
	{ "21j-8",         0x10000, 0x7c435887, BRF_GRA },	     //  15	Tiles
	{ "21j-9",         0x10000, 0xc6640aed, BRF_GRA },	     //  16
	{ "21j-i",         0x10000, 0x5effb0a0, BRF_GRA },	     //  17
	{ "21j-j",         0x10000, 0x5fb42e7c, BRF_GRA },	     //  18
	
	{ "21j-6",         0x10000, 0x34755de3, BRF_GRA },	     //  19	Samples
	{ "21j-7",         0x10000, 0x904de6f8, BRF_GRA },	     //  20
	
	{ "21j-k-0",       0x00100, 0xfdb130a9, BRF_GRA },	     //  21	PROMs
	{ "21j-l-0",       0x00200, 0x46339529, BRF_GRA },	     //  22
};

STD_ROM_PICK(Drvb);
STD_ROM_FN(Drvb);

static struct BurnRomInfo DrvbaRomDesc[] = {
	{ "5.bin",         0x08000, 0xae714964, BRF_ESS | BRF_PRG }, //  0	HD6309 Program Code
	{ "4.bin",         0x10000, 0x48045762, BRF_ESS | BRF_PRG }, //  1
	{ "3.bin",         0x08000, 0xdbf24897, BRF_ESS | BRF_PRG }, //  2
	
	{ "2_32.bin",      0x04000, 0x67875473, BRF_ESS | BRF_PRG }, //  3	M6803 Program Code
	
	{ "6.bin",         0x08000, 0x9efa95bb, BRF_ESS | BRF_PRG }, //  4	M6809 Program Code
	
	{ "1.bin",         0x08000, 0x7a8b8db4, BRF_GRA },	     //  5	Characters
	
	{ "21j-a",         0x10000, 0x574face3, BRF_GRA },	     //  6	Sprites
	{ "21j-b",         0x10000, 0x40507a76, BRF_GRA },	     //  7
	{ "21j-c",         0x10000, 0xbb0bc76f, BRF_GRA },	     //  8
	{ "21j-d",         0x10000, 0xcb4f231b, BRF_GRA },	     //  9
	{ "21j-e",         0x10000, 0xa0a0c261, BRF_GRA },	     //  10
	{ "21j-f",         0x10000, 0x6ba152f6, BRF_GRA },	     //  11
	{ "21j-g",         0x10000, 0x3220a0b6, BRF_GRA },	     //  12
	{ "21j-h",         0x10000, 0x65c7517d, BRF_GRA },	     //  13
	
	{ "21j-8",         0x10000, 0x7c435887, BRF_GRA },	     //  14	Tiles
	{ "21j-9",         0x10000, 0xc6640aed, BRF_GRA },	     //  15
	{ "21j-i",         0x10000, 0x5effb0a0, BRF_GRA },	     //  16
	{ "21j-j",         0x10000, 0x5fb42e7c, BRF_GRA },	     //  17
	
	{ "8.bin",         0x10000, 0x34755de3, BRF_GRA },	     //  18	Samples
	{ "7.bin",         0x10000, 0xf9311f72, BRF_GRA },	     //  19
	
	{ "21j-k-0",       0x00100, 0xfdb130a9, BRF_GRA },	     //  20	PROMs
	{ "21j-l-0",       0x00200, 0x46339529, BRF_GRA },	     //  21
};

STD_ROM_PICK(Drvba);
STD_ROM_FN(Drvba);

static struct BurnRomInfo Drv2RomDesc[] = {
	{ "26a9-04.bin",   0x08000, 0xf2cfc649, BRF_ESS | BRF_PRG }, //  0	HD6309 Program Code
	{ "26aa-03.bin",   0x08000, 0x44dd5d4b, BRF_ESS | BRF_PRG }, //  1
	{ "26ab-0.bin",    0x08000, 0x49ddddcd, BRF_ESS | BRF_PRG }, //  2
	{ "26ac-0e.63",    0x08000, 0x57acad2c, BRF_ESS | BRF_PRG }, //  3
	
	{ "26ae-0.bin",    0x10000, 0xea437867, BRF_ESS | BRF_PRG }, //  4	Z80 #1 Program Code
	
	{ "26ad-0.bin",    0x08000, 0x75e36cd6, BRF_ESS | BRF_PRG }, //  5	Z80 #2 Program Code
	
	{ "26a8-0e.19",    0x10000, 0x4e80cd36, BRF_GRA },	     //  6	Characters
	
	{ "26j0-0.bin",    0x20000, 0xdb309c84, BRF_GRA },	     //  7	Sprites
	{ "26j1-0.bin",    0x20000, 0xc3081e0c, BRF_GRA },	     //  8
	{ "26af-0.bin",    0x20000, 0x3a615aad, BRF_GRA },	     //  9
	{ "26j2-0.bin",    0x20000, 0x589564ae, BRF_GRA },	     //  10
	{ "26j3-0.bin",    0x20000, 0xdaf040d6, BRF_GRA },	     //  11
	{ "26a10-0.bin",   0x20000, 0x6d16d889, BRF_GRA },	     //  12
	
	{ "26j4-0.bin",    0x20000, 0xa8c93e76, BRF_GRA },	     //  13	Tiles
	{ "26j5-0.bin",    0x20000, 0xee555237, BRF_GRA },	     //  14
	
	{ "26j6-0.bin",    0x20000, 0xa84b2a29, BRF_GRA },	     //  15	Samples
	{ "26j7-0.bin",    0x20000, 0xbc6a48d5, BRF_GRA },	     //  16
	
	{ "prom.16",       0x00200, 0x46339529, BRF_GRA },	     //  17	PROMs
};

STD_ROM_PICK(Drv2);
STD_ROM_FN(Drv2);

static struct BurnRomInfo Drv2uRomDesc[] = {
	{ "26a9-04.bin",   0x08000, 0xf2cfc649, BRF_ESS | BRF_PRG }, //  0	HD6309 Program Code
	{ "26aa-03.bin",   0x08000, 0x44dd5d4b, BRF_ESS | BRF_PRG }, //  1
	{ "26ab-0.bin",    0x08000, 0x49ddddcd, BRF_ESS | BRF_PRG }, //  2
	{ "26ac-02.bin",   0x08000, 0x097eaf26, BRF_ESS | BRF_PRG }, //  3
	
	{ "26ae-0.bin",    0x10000, 0xea437867, BRF_ESS | BRF_PRG }, //  4	Z80 #1 Program Code
	
	{ "26ad-0.bin",    0x08000, 0x75e36cd6, BRF_ESS | BRF_PRG }, //  5	Z80 #2 Program Code
	
	{ "26a8-0.bin",    0x10000, 0x3ad1049c, BRF_GRA },	     //  6	Characters
	
	{ "26j0-0.bin",    0x20000, 0xdb309c84, BRF_GRA },	     //  7	Sprites
	{ "26j1-0.bin",    0x20000, 0xc3081e0c, BRF_GRA },	     //  8
	{ "26af-0.bin",    0x20000, 0x3a615aad, BRF_GRA },	     //  9
	{ "26j2-0.bin",    0x20000, 0x589564ae, BRF_GRA },	     //  10
	{ "26j3-0.bin",    0x20000, 0xdaf040d6, BRF_GRA },	     //  11
	{ "26a10-0.bin",   0x20000, 0x6d16d889, BRF_GRA },	     //  12
	
	{ "26j4-0.bin",    0x20000, 0xa8c93e76, BRF_GRA },	     //  13	Tiles
	{ "26j5-0.bin",    0x20000, 0xee555237, BRF_GRA },	     //  14
	
	{ "26j6-0.bin",    0x20000, 0xa84b2a29, BRF_GRA },	     //  15	Samples
	{ "26j7-0.bin",    0x20000, 0xbc6a48d5, BRF_GRA },	     //  16
	
	{ "prom.16",       0x00200, 0x46339529, BRF_GRA },	     //  17	PROMs
};

STD_ROM_PICK(Drv2u);
STD_ROM_FN(Drv2u);

static int MemIndex()
{
	unsigned char *Next; Next = Mem;

	DrvHD6309Rom           = Next; Next += 0x30000;
	DrvSubCPURom           = Next; Next += 0x04000;
	DrvSoundCPURom         = Next; Next += 0x08000;
	MSM5205ROM             = Next; Next += 0x20000;

	RamStart               = Next;

	DrvHD6309Ram           = Next; Next += 0x01000;
	DrvSubCPURam           = Next; Next += 0x00fd0;
	DrvSoundCPURam         = Next; Next += 0x01000;
	DrvFgVideoRam          = Next; Next += 0x00800;
	DrvSpriteRam           = Next; Next += 0x01000;
	DrvBgVideoRam          = Next; Next += 0x00800;
	DrvPaletteRam1         = Next; Next += 0x00200;
	DrvPaletteRam2         = Next; Next += 0x00200;

	RamEnd                 = Next;

	DrvChars               = Next; Next += 0x0400 * 8 * 8;
	DrvTiles               = Next; Next += 0x0800 * 16 * 16;
	DrvSprites             = Next; Next += 0x1000 * 16 * 16;
	DrvPalette             = (unsigned int*)Next; Next += 0x00180 * sizeof(unsigned int);

	MemEnd                 = Next;

	return 0;
}

static int Drv2MemIndex()
{
	unsigned char *Next; Next = Mem;

	DrvHD6309Rom           = Next; Next += 0x30000;
	DrvSubCPURom           = Next; Next += 0x10000;
	DrvSoundCPURom         = Next; Next += 0x08000;
	MSM6295ROM             = Next; Next += 0x40000;

	RamStart               = Next;

	DrvHD6309Ram           = Next; Next += 0x01800;
	DrvSoundCPURam         = Next; Next += 0x00800;
	DrvFgVideoRam          = Next; Next += 0x00800;
	DrvSpriteRam           = Next; Next += 0x01000;
	DrvBgVideoRam          = Next; Next += 0x00800;
	DrvPaletteRam1         = Next; Next += 0x00200;
	DrvPaletteRam2         = Next; Next += 0x00200;

	RamEnd                 = Next;

	DrvChars               = Next; Next += 0x0800 * 8 * 8;
	DrvTiles               = Next; Next += 0x0800 * 16 * 16;
	DrvSprites             = Next; Next += 0x1800 * 16 * 16;
	DrvPalette             = (unsigned int*)Next; Next += 0x00180 * sizeof(unsigned int);

	MemEnd                 = Next;

	return 0;
}

static int DrvDoReset()
{
	HD6309Open(0);
	HD6309Reset();
	HD6309Close();
	
	if (DrvSubCPUType == DD_CPU_TYPE_HD63701) {
		HD63701Reset();
	}
	
	if (DrvSubCPUType == DD_CPU_TYPE_HD6309) {
		HD6309Open(1);
		HD6309Reset();
		HD6309Close();
	}
	
	if (DrvSubCPUType == DD_CPU_TYPE_M6803) {
		M6803Reset();
	}
	
	if (DrvSubCPUType == DD_CPU_TYPE_Z80) {
		ZetOpen(0);
		ZetReset();
		ZetClose();
	}
	
	if (DrvSoundCPUType == DD_CPU_TYPE_M6809) {
		M6809Open(0);
		M6809Reset();
		M6809Close();
		
		MSM5205Reset(0);
		MSM5205Reset(1);
	}
	
	if (DrvSoundCPUType == DD_CPU_TYPE_Z80) {
		ZetOpen(1);
		ZetReset();
		ZetClose();
		
		MSM6295Reset(0);
	}
	
	BurnYM2151Reset();	
	
	DrvRomBank = 0;
	DrvVBlank = 0;
	DrvSubCPUBusy = 1;
	DrvSoundLatch = 0;
	DrvScrollXHi = 0;
	DrvScrollYHi = 0;
	DrvScrollXLo = 0;
	DrvScrollYLo = 0;
	
	DrvADPCMIdle[0] = 0;
	DrvADPCMIdle[1] = 1;
	DrvADPCMPos[0] = 0;
	DrvADPCMPos[1] = 0;
	DrvADPCMEnd[0] = 0;
	DrvADPCMEnd[1] = 0;
	
	return 0;
}

unsigned char DrvDdragonHD6309ReadByte(unsigned short Address)
{
	if (Address >= 0x2000 && Address <= 0x2fff) {
		if (Address == 0x2049 && HD6309GetPC() == 0x6261 && DrvSpriteRam[0x0049] == 0x1f) return 0x01;
		return DrvSpriteRam[Address - 0x2000];
	}
	
	switch (Address) {
		case 0x3800: {
			return DrvInput[0];
		}
		
		case 0x3801: {
			return DrvInput[1];
		}
		
		case 0x3802: {
			return DrvInput[2] | ((DrvVBlank) ? 0x08 : 0) | (DrvSubCPUBusy ? 0x10 : 0);
		}
		
		case 0x3803: {
			return DrvDip[0];
		}
		
		case 0x3804: {
			return DrvDip[1];
		}
		
		case 0x380b: {
			// ???
			return 0;
		}
	}
	
	bprintf(PRINT_NORMAL, _T("HD6309 Read Byte -> %04X\n"), Address);
	
	return 0;
}

void DrvDdragonHD6309WriteByte(unsigned short Address, unsigned char Data)
{
	switch (Address) {
		case 0x3808: {
			DrvRomBank = (Data & 0xe0) >> 5;
			HD6309MapMemory(DrvHD6309Rom + 0x8000 + (DrvRomBank * 0x4000), 0x4000, 0x7fff, M6809_ROM);
			
			DrvScrollXHi = (Data & 0x01) << 8;
			DrvScrollYHi = (Data & 0x02) << 7;
			
			if (Data & 0x10) {
				DrvSubCPUBusy = 0;
			} else {
				if (DrvSubCPUBusy == 0) {
					if (DrvSubCPUType == DD_CPU_TYPE_HD63701) {
						HD63701SetIRQ(HD63701_INPUT_LINE_NMI, HD63701_IRQSTATUS_ACK);
					}
					
					if (DrvSubCPUType == DD_CPU_TYPE_HD6309) {
						HD6309Close();
						HD6309Open(1);
						HD6309SetIRQ(HD6309_INPUT_LINE_NMI, HD6309_IRQSTATUS_ACK);
						HD6309Close();
						HD6309Open(0);
					}
					
					if (DrvSubCPUType == DD_CPU_TYPE_M6803) {
						M6803SetIRQ(M6803_INPUT_LINE_NMI, M6803_IRQSTATUS_ACK);
					}
					
					if (DrvSubCPUType == DD_CPU_TYPE_Z80) {
						ZetOpen(0);
						ZetNmi();
						ZetClose();
					}
				}
			}
			return;			
		}
		
		case 0x3809: {
			DrvScrollXLo = Data;
			return;
		}
		
		case 0x380a: {
			DrvScrollYLo = Data;
			return;
		}
		
		case 0x380b: {
			HD6309SetIRQ(HD6309_INPUT_LINE_NMI, HD6309_IRQSTATUS_NONE);
			return;
		}
		
		case 0x380c: {
			HD6309SetIRQ(HD6309_FIRQ_LINE, HD6309_IRQSTATUS_NONE);
			return;
		}

		case 0x380d: {
			HD6309SetIRQ(HD6309_IRQ_LINE, HD6309_IRQSTATUS_NONE);
			return;
		}
		
		case 0x380e: {
			DrvSoundLatch = Data;
			if (DrvSoundCPUType == DD_CPU_TYPE_M6809) {
				M6809Open(0);
				M6809SetIRQ(M6809_IRQ_LINE, M6809_IRQSTATUS_ACK);
				M6809Close();
			}
			
			if (DrvSoundCPUType == DD_CPU_TYPE_Z80) {
				ZetOpen(1);
				ZetNmi();
				ZetClose();
			}
			return;
		}
		
		case 0x380f: {
			// ???
			return;
		}
	}	
	
	bprintf(PRINT_NORMAL, _T("HD6309 Write Byte -> %04X, %02X\n"), Address, Data);
}

unsigned char DrvDdragonHD63701ReadByte(unsigned short Address)
{
	if (Address >= 0x0020 && Address <= 0x0fff) {
		return DrvSubCPURam[Address - 0x0020];
	}
	
	if (Address >= 0x8000 && Address <= 0x8fff) {
		if (Address == 0x8049 && HD63701GetPC() == 0x6261 && DrvSpriteRam[0x0049] == 0x1f) return 0x01;
		return DrvSpriteRam[Address - 0x8000];
	}
	
	bprintf(PRINT_NORMAL, _T("M6800 Read Byte -> %04X\n"), Address);
	
	return 0;
}

void DrvDdragonHD63701WriteByte(unsigned short Address, unsigned char Data)
{
	if (Address <= 0x001f) {
		if (Address == 0x17) {
			if (Data & 3) {
				HD6309Open(0);
				HD6309SetIRQ(HD6309_IRQ_LINE, HD6309_IRQSTATUS_ACK);
				HD6309Close();
				
				HD63701SetIRQ(HD63701_INPUT_LINE_NMI, HD63701_IRQSTATUS_NONE);
			}
		}
		return;
	}
	
	if (Address >= 0x0020 && Address <= 0x0fff) {
		DrvSubCPURam[Address - 0x0020] = Data;
		return;
	}
	
	if (Address >= 0x8000 && Address <= 0x8fff) {
		if (Address == 0x8000) DrvSubCPUBusy = 1;
		DrvSpriteRam[Address - 0x8000] = Data;
		return;
	}
	
	bprintf(PRINT_NORMAL, _T("M6800 Write Byte -> %04X, %02X\n"), Address, Data);
}

unsigned char DrvDdragonbSubHD6309ReadByte(unsigned short Address)
{
	if (Address >= 0x0020 && Address <= 0x0fff) {
		return DrvSubCPURam[Address - 0x0020];
	}
	
	if (Address >= 0x8000 && Address <= 0x8fff) {
		return DrvSpriteRam[Address - 0x8000];
	}
	
	bprintf(PRINT_NORMAL, _T("Sub HD6309 Read Byte -> %04X\n"), Address);
	
	return 0;
}

void DrvDdragonbSubHD6309WriteByte(unsigned short Address, unsigned char Data)
{
	if (Address <= 0x001f) {
		if (Address == 0x17) {
			if (Data & 3) {
				HD6309Close();
				
				HD6309Open(0);
				HD6309SetIRQ(HD6309_IRQ_LINE, HD6309_IRQSTATUS_ACK);
				HD6309Close();
				
				HD6309Open(1);
				HD6309SetIRQ(HD6309_INPUT_LINE_NMI, HD6309_IRQSTATUS_NONE);
			}
		}
		return;
	}
	
	if (Address >= 0x0020 && Address <= 0x0fff) {
		DrvSubCPURam[Address - 0x0020] = Data;
		return;
	}
	
	if (Address >= 0x8000 && Address <= 0x8fff) {
		if (Address == 0x8000) DrvSubCPUBusy = 1;
		DrvSpriteRam[Address - 0x8000] = Data;
		return;
	}
	
	bprintf(PRINT_NORMAL, _T("Sub HD6309 Write Byte -> %04X, %02X\n"), Address, Data);
}

unsigned char DrvDdragonbaM6803ReadByte(unsigned short Address)
{
	if (Address >= 0x0020 && Address <= 0x0fff) {
		return DrvSubCPURam[Address - 0x0020];
	}
	
	if (Address >= 0x8000 && Address <= 0x8fff) {
		return DrvSpriteRam[Address - 0x8000];
	}
	
	bprintf(PRINT_NORMAL, _T("M6803 Read Byte -> %04X\n"), Address);
	
	return 0;
}

void DrvDdragonbaM6803WriteByte(unsigned short Address, unsigned char Data)
{
	if (Address >= 0x0020 && Address <= 0x0fff) {
		DrvSubCPURam[Address - 0x0020] = Data;
		return;
	}
	
	if (Address >= 0x8000 && Address <= 0x8fff) {
		if (Address == 0x8000) DrvSubCPUBusy = 1;
		DrvSpriteRam[Address - 0x8000] = Data;
		return;
	}
	
	if (Address <= 0x001f) {
		m6803_internal_registers_w(Address, Data);
		return;
	}
	
	bprintf(PRINT_NORMAL, _T("M6803 Write Byte -> %04X, %02X\n"), Address, Data);
}

void DrvDdragonbaM6803WritePort(unsigned short, unsigned char)
{
	M6803SetIRQ(M6803_INPUT_LINE_NMI, M6803_IRQSTATUS_NONE);
	
	HD6309Open(0);
	HD6309SetIRQ(HD6309_IRQ_LINE, HD6309_IRQSTATUS_ACK);
	HD6309Close();
}

void __fastcall Ddragon2SubZ80Write(unsigned short Address, unsigned char Data)
{
	if (Address >= 0xc000 && Address <= 0xc3ff) {
		if (Address == 0xc000) DrvSubCPUBusy = 1;
		DrvSpriteRam[Address - 0xc000] = Data;
		return;
	}
	
	switch (Address) {
		case 0xd000: {
			// Lower NMI
			return;
		}
		
		case 0xe000: {
			HD6309Open(0);
			HD6309SetIRQ(HD6309_IRQ_LINE, HD6309_IRQSTATUS_ACK);
			HD6309Close();
			return;
		}
		
		default: {
			bprintf(PRINT_NORMAL, _T("Sub Z80 Write => %04X, %02X\n"), Address, Data);
		}
	}
}

unsigned char DrvDdragonM6809ReadByte(unsigned short Address)
{
	switch (Address) {
		case 0x1000: {
			M6809SetIRQ(M6809_IRQ_LINE, M6809_IRQSTATUS_NONE);
			return DrvSoundLatch;
		}
		
		case 0x1800: {
			return DrvADPCMIdle[0] + (DrvADPCMIdle[1] << 1);
		}
		
		case 0x2801: {
			return BurnYM2151ReadStatus();
		}
	}
	
	bprintf(PRINT_NORMAL, _T("M6809 Read Byte -> %04X\n"), Address);
	
	return 0;
}


void DrvDdragonM6809WriteByte(unsigned short Address, unsigned char Data)
{
	switch (Address) {
		case 0x2800: {
			BurnYM2151SelectRegister(Data);
			return;
		}
		
		case 0x2801: {
			BurnYM2151WriteRegister(Data);
			return;
		}
	
		case 0x3800: {
			DrvADPCMIdle[0] = 0;
			MSM5205Play(DrvADPCMPos[0], DrvADPCMEnd[0] - DrvADPCMPos[0], 0);
			return;
		}
		
		case 0x3801: {
			DrvADPCMIdle[1] = 0;
			MSM5205Play(0x10000 + DrvADPCMPos[1], DrvADPCMEnd[1] - DrvADPCMPos[1], 1);
			return;
		}
		
		case 0x3802: {
			DrvADPCMEnd[0] = (Data & 0x7f) * 0x200;			
			return;
		}
		
		case 0x3803: {
			DrvADPCMEnd[1] = (Data & 0x7f) * 0x200;			
			return;
		}
		
		case 0x3804: {
			DrvADPCMPos[0] = (Data & 0x7f) * 0x200;			
			return;
		}
		
		case 0x3805: {
			DrvADPCMPos[1] = (Data & 0x7f) * 0x200;
			return;
		}
		
		case 0x3806: {
			DrvADPCMIdle[0] = 1;
			MSM5205Reset(0);			
			return;
		}
		
		case 0x3807: {
			DrvADPCMIdle[1] = 1;
			MSM5205Reset(1);			
			return;
		}
	}
	
	bprintf(PRINT_NORMAL, _T("M6809 Write Byte -> %04X, %02X\n"), Address, Data);
}

unsigned char __fastcall Ddragon2SoundZ80Read(unsigned short Address)
{
	switch (Address) {
		case 0x8801: {
			return BurnYM2151ReadStatus();
		}
		
		case 0x9800: {
			return MSM6295ReadStatus(0);
		}
		
		case 0xa000: {
			return DrvSoundLatch;
		}
		
		default: {
			bprintf(PRINT_NORMAL, _T("Sound Z80 Read => %04X\n"), Address);
		}
	}

	return 0;
}

void __fastcall Ddragon2SoundZ80Write(unsigned short Address, unsigned char Data)
{
	switch (Address) {
		case 0x8800: {
			BurnYM2151SelectRegister(Data);
			return;
		}
		
		case 0x8801: {
			BurnYM2151WriteRegister(Data);
			return;
		}
		
		case 0x9800: {
			MSM6295Command(0, Data);
			return;
		}
		
		default: {
			bprintf(PRINT_NORMAL, _T("Sound Z80 Write => %04X, %02X\n"), Address, Data);
		}
	}
}

static int CharPlaneOffsets[4]      = { 0, 2, 4, 6 };
static int CharXOffsets[8]          = { 1, 0, 65, 64, 129, 128, 193, 192 };
static int CharYOffsets[8]          = { 0, 8, 16, 24, 32, 40, 48, 56 };
static int TilePlaneOffsets[4]      = { 0x100000, 0x100004, 0, 4 };
static int TileXOffsets[16]         = { 3, 2, 1, 0, 131, 130, 129, 128, 259, 258, 257, 256, 387, 386, 385, 384 };
static int TileYOffsets[16]         = { 0, 8, 16, 24, 32, 40, 48, 56, 64, 72, 80, 88, 96, 104, 112, 120 };
static int SpritePlaneOffsets[4]    = { 0x200000, 0x200004, 0, 4 };
static int Dd2SpritePlaneOffsets[4] = { 0x300000, 0x300004, 0, 4 };

static void DrvYM2151IrqHandler(int Irq)
{
	if (Irq) {
		M6809SetIRQ(M6809_FIRQ_LINE, M6809_IRQSTATUS_ACK);
	} else {
		M6809SetIRQ(M6809_FIRQ_LINE, M6809_IRQSTATUS_NONE);
	}
}

static void Ddragon2YM2151IrqHandler(int Irq)
{
	if (Irq) {
		ZetSetIRQLine(0, ZET_IRQSTATUS_ACK);
	} else {
		ZetSetIRQLine(0, ZET_IRQSTATUS_NONE);
	}
}

static int DrvMemInit()
{
	int nLen;
	
	// Allocate and Blank all required memory
	Mem = NULL;
	MemIndex();
	nLen = MemEnd - (unsigned char *)0;
	if ((Mem = (unsigned char *)malloc(nLen)) == NULL) return 1;
	memset(Mem, 0, nLen);
	MemIndex();
	
	return 0;
}

static int Drv2MemInit()
{
	int nLen;
	
	// Allocate and Blank all required memory
	Mem = NULL;
	Drv2MemIndex();
	nLen = MemEnd - (unsigned char *)0;
	if ((Mem = (unsigned char *)malloc(nLen)) == NULL) return 1;
	memset(Mem, 0, nLen);
	Drv2MemIndex();
	
	return 0;
}

static int DrvLoadRoms()
{
	int nRet = 0;

	DrvTempRom = (unsigned char *)malloc(0x80000);

	// Load HD6309 Program Roms
	nRet = BurnLoadRom(DrvHD6309Rom + 0x00000, 0, 1); if (nRet != 0) return 1;
	nRet = BurnLoadRom(DrvHD6309Rom + 0x08000, 1, 1); if (nRet != 0) return 1;
	nRet = BurnLoadRom(DrvHD6309Rom + 0x10000, 2, 1); if (nRet != 0) return 1;
	nRet = BurnLoadRom(DrvHD6309Rom + 0x18000, 3, 1); if (nRet != 0) return 1;
	
	// Load HD63701 Program Roms
	nRet = BurnLoadRom(DrvSubCPURom + 0x00000, 4, 1); if (nRet != 0) return 1;
	
	// Load M6809 Program Roms
	nRet = BurnLoadRom(DrvSoundCPURom + 0x00000, 5, 1); if (nRet != 0) return 1;
	
	// Load and decode the chars
	nRet = BurnLoadRom(DrvTempRom, 6, 1); if (nRet != 0) return 1;
	GfxDecode(0x400, 4, 8, 8, CharPlaneOffsets, CharXOffsets, CharYOffsets, 0x100, DrvTempRom, DrvChars);
	
	// Load and decode the sprites
	memset(DrvTempRom, 0, 0x80000);
	nRet = BurnLoadRom(DrvTempRom + 0x00000,  7, 1); if (nRet != 0) return 1;
	nRet = BurnLoadRom(DrvTempRom + 0x10000,  8, 1); if (nRet != 0) return 1;
	nRet = BurnLoadRom(DrvTempRom + 0x20000,  9, 1); if (nRet != 0) return 1;
	nRet = BurnLoadRom(DrvTempRom + 0x30000, 10, 1); if (nRet != 0) return 1;
	nRet = BurnLoadRom(DrvTempRom + 0x40000, 11, 1); if (nRet != 0) return 1;
	nRet = BurnLoadRom(DrvTempRom + 0x50000, 12, 1); if (nRet != 0) return 1;
	nRet = BurnLoadRom(DrvTempRom + 0x60000, 13, 1); if (nRet != 0) return 1;	
	nRet = BurnLoadRom(DrvTempRom + 0x70000, 14, 1); if (nRet != 0) return 1;
	GfxDecode(0x1000, 4, 16, 16, SpritePlaneOffsets, TileXOffsets, TileYOffsets, 0x200, DrvTempRom, DrvSprites);
	
	// Load and decode the tiles
	memset(DrvTempRom, 0, 0x80000);
	nRet = BurnLoadRom(DrvTempRom + 0x00000, 15, 1); if (nRet != 0) return 1;
	nRet = BurnLoadRom(DrvTempRom + 0x10000, 16, 1); if (nRet != 0) return 1;
	nRet = BurnLoadRom(DrvTempRom + 0x20000, 17, 1); if (nRet != 0) return 1;
	nRet = BurnLoadRom(DrvTempRom + 0x30000, 18, 1); if (nRet != 0) return 1;
	GfxDecode(0x800, 4, 16, 16, TilePlaneOffsets, TileXOffsets, TileYOffsets, 0x200, DrvTempRom, DrvTiles);
	
	// Load samples
	nRet = BurnLoadRom(MSM5205ROM + 0x00000, 19, 1); if (nRet != 0) return 1;
	nRet = BurnLoadRom(MSM5205ROM + 0x10000, 20, 1); if (nRet != 0) return 1;

	free(DrvTempRom);
	
	return 0;
}

static int DrvbaLoadRoms()
{
	int nRet = 0;
	
	DrvTempRom = (unsigned char *)malloc(0x80000);

	// Load HD6309 Program Roms
	nRet = BurnLoadRom(DrvHD6309Rom + 0x00000, 0, 1); if (nRet != 0) return 1;
	nRet = BurnLoadRom(DrvHD6309Rom + 0x08000, 1, 1); if (nRet != 0) return 1;
	memcpy(DrvHD6309Rom + 0x18000, DrvHD6309Rom + 0x10000, 0x8000);
	nRet = BurnLoadRom(DrvHD6309Rom + 0x10000, 2, 1); if (nRet != 0) return 1;
	
	// Load M6803 Program Roms
	nRet = BurnLoadRom(DrvSubCPURom + 0x00000, 3, 1); if (nRet != 0) return 1;
	
	// Load M6809 Program Roms
	nRet = BurnLoadRom(DrvSoundCPURom + 0x00000, 4, 1); if (nRet != 0) return 1;
	
	// Load and decode the chars
	nRet = BurnLoadRom(DrvTempRom, 5, 1); if (nRet != 0) return 1;
	GfxDecode(0x400, 4, 8, 8, CharPlaneOffsets, CharXOffsets, CharYOffsets, 0x100, DrvTempRom, DrvChars);
	
	// Load and decode the sprites
	memset(DrvTempRom, 0, 0x80000);
	nRet = BurnLoadRom(DrvTempRom + 0x00000,  6, 1); if (nRet != 0) return 1;
	nRet = BurnLoadRom(DrvTempRom + 0x10000,  7, 1); if (nRet != 0) return 1;
	nRet = BurnLoadRom(DrvTempRom + 0x20000,  8, 1); if (nRet != 0) return 1;
	nRet = BurnLoadRom(DrvTempRom + 0x30000,  9, 1); if (nRet != 0) return 1;
	nRet = BurnLoadRom(DrvTempRom + 0x40000, 10, 1); if (nRet != 0) return 1;
	nRet = BurnLoadRom(DrvTempRom + 0x50000, 11, 1); if (nRet != 0) return 1;
	nRet = BurnLoadRom(DrvTempRom + 0x60000, 12, 1); if (nRet != 0) return 1;	
	nRet = BurnLoadRom(DrvTempRom + 0x70000, 13, 1); if (nRet != 0) return 1;
	GfxDecode(0x1000, 4, 16, 16, SpritePlaneOffsets, TileXOffsets, TileYOffsets, 0x200, DrvTempRom, DrvSprites);
	
	// Load and decode the tiles
	memset(DrvTempRom, 0, 0x80000);
	nRet = BurnLoadRom(DrvTempRom + 0x00000, 14, 1); if (nRet != 0) return 1;
	nRet = BurnLoadRom(DrvTempRom + 0x10000, 15, 1); if (nRet != 0) return 1;
	nRet = BurnLoadRom(DrvTempRom + 0x20000, 16, 1); if (nRet != 0) return 1;
	nRet = BurnLoadRom(DrvTempRom + 0x30000, 17, 1); if (nRet != 0) return 1;
	GfxDecode(0x800, 4, 16, 16, TilePlaneOffsets, TileXOffsets, TileYOffsets, 0x200, DrvTempRom, DrvTiles);
	
	// Load samples
	nRet = BurnLoadRom(MSM5205ROM + 0x00000, 18, 1); if (nRet != 0) return 1;
	nRet = BurnLoadRom(MSM5205ROM + 0x10000, 19, 1); if (nRet != 0) return 1;

	free(DrvTempRom);
	
	return 0;
}

static int Drv2LoadRoms()
{
	int nRet = 0;

	DrvTempRom = (unsigned char *)malloc(0xc0000);

	// Load HD6309 Program Roms
	nRet = BurnLoadRom(DrvHD6309Rom + 0x00000, 0, 1); if (nRet != 0) return 1;
	nRet = BurnLoadRom(DrvHD6309Rom + 0x08000, 1, 1); if (nRet != 0) return 1;
	nRet = BurnLoadRom(DrvHD6309Rom + 0x10000, 2, 1); if (nRet != 0) return 1;
	nRet = BurnLoadRom(DrvHD6309Rom + 0x18000, 3, 1); if (nRet != 0) return 1;
	
	// Load HD63701 Program Roms
	nRet = BurnLoadRom(DrvSubCPURom + 0x00000, 4, 1); if (nRet != 0) return 1;
	
	// Load M6809 Program Roms
	nRet = BurnLoadRom(DrvSoundCPURom + 0x00000, 5, 1); if (nRet != 0) return 1;
	
	// Load and decode the chars
	nRet = BurnLoadRom(DrvTempRom, 6, 1); if (nRet != 0) return 1;
	GfxDecode(0x800, 4, 8, 8, CharPlaneOffsets, CharXOffsets, CharYOffsets, 0x100, DrvTempRom, DrvChars);
	
	// Load and decode the sprites
	memset(DrvTempRom, 0, 0xc0000);
	nRet = BurnLoadRom(DrvTempRom + 0x00000,  7, 1); if (nRet != 0) return 1;
	nRet = BurnLoadRom(DrvTempRom + 0x20000,  8, 1); if (nRet != 0) return 1;
	nRet = BurnLoadRom(DrvTempRom + 0x40000,  9, 1); if (nRet != 0) return 1;
	nRet = BurnLoadRom(DrvTempRom + 0x60000, 10, 1); if (nRet != 0) return 1;
	nRet = BurnLoadRom(DrvTempRom + 0x80000, 11, 1); if (nRet != 0) return 1;
	nRet = BurnLoadRom(DrvTempRom + 0xa0000, 12, 1); if (nRet != 0) return 1;
	GfxDecode(0x1800, 4, 16, 16, Dd2SpritePlaneOffsets, TileXOffsets, TileYOffsets, 0x200, DrvTempRom, DrvSprites);
	
	// Load and decode the tiles
	memset(DrvTempRom, 0, 0xc0000);
	nRet = BurnLoadRom(DrvTempRom + 0x00000, 13, 1); if (nRet != 0) return 1;
	nRet = BurnLoadRom(DrvTempRom + 0x20000, 14, 1); if (nRet != 0) return 1;
	GfxDecode(0x800, 4, 16, 16, TilePlaneOffsets, TileXOffsets, TileYOffsets, 0x200, DrvTempRom, DrvTiles);
	
	// Load samples
	nRet = BurnLoadRom(MSM6295ROM + 0x00000, 15, 1); if (nRet != 0) return 1;
	nRet = BurnLoadRom(MSM6295ROM + 0x20000, 16, 1); if (nRet != 0) return 1;

	free(DrvTempRom);
	
	return 0;
}

static int DrvMachineInit()
{
	// Setup the HD6309 emulation
	if (DrvSubCPUType == DD_CPU_TYPE_HD6309) {
		HD6309Init(2);
	} else {
		HD6309Init(1);
	}
	HD6309Open(0);
	HD6309MapMemory(DrvHD6309Ram         , 0x0000, 0x0fff, M6809_RAM);
	HD6309MapMemory(DrvPaletteRam1       , 0x1000, 0x11ff, M6809_RAM);
	HD6309MapMemory(DrvPaletteRam2       , 0x1200, 0x13ff, M6809_RAM);
	HD6309MapMemory(DrvFgVideoRam        , 0x1800, 0x1fff, M6809_RAM);
	HD6309MapMemory(DrvSpriteRam         , 0x2000, 0x2fff, M6809_WRITE);
	HD6309MapMemory(DrvBgVideoRam        , 0x3000, 0x37ff, M6809_RAM);
	HD6309MapMemory(DrvHD6309Rom + 0x8000, 0x4000, 0x7fff, M6809_ROM);
	HD6309MapMemory(DrvHD6309Rom         , 0x8000, 0xffff, M6809_ROM);
	HD6309SetReadByteHandler(DrvDdragonHD6309ReadByte);
	HD6309SetWriteByteHandler(DrvDdragonHD6309WriteByte);
	HD6309Close();
	
	if (DrvSubCPUType == DD_CPU_TYPE_HD63701) {
		HD63701Init(1);
		HD63701MapMemory(DrvSubCPURom        , 0xc000, 0xffff, HD63701_ROM);
		HD63701SetReadByteHandler(DrvDdragonHD63701ReadByte);
		HD63701SetWriteByteHandler(DrvDdragonHD63701WriteByte);
	}
	
	if (DrvSubCPUType == DD_CPU_TYPE_HD6309) {
		HD6309Open(1);
		HD6309MapMemory(DrvSubCPURom        , 0xc000, 0xffff, HD6309_ROM);
		HD6309SetReadByteHandler(DrvDdragonbSubHD6309ReadByte);
		HD6309SetWriteByteHandler(DrvDdragonbSubHD6309WriteByte);
		HD6309Close();
	}
	
	if (DrvSubCPUType == DD_CPU_TYPE_M6803) {
		M6803Init(1);
		M6803MapMemory(DrvSubCPURom        , 0xc000, 0xffff, M6803_ROM);
		M6803SetReadByteHandler(DrvDdragonbaM6803ReadByte);
		M6803SetWriteByteHandler(DrvDdragonbaM6803WriteByte);
		M6803SetWritePortHandler(DrvDdragonbaM6803WritePort);
	}
	
	if (DrvSoundCPUType == DD_CPU_TYPE_M6809) {
		M6809Init(1);
		M6809Open(0);
		M6809MapMemory(DrvSoundCPURam      , 0x0000, 0x0fff, M6809_RAM);
		M6809MapMemory(DrvSoundCPURom      , 0x8000, 0xffff, M6809_ROM);
		M6809SetReadByteHandler(DrvDdragonM6809ReadByte);
		M6809SetWriteByteHandler(DrvDdragonM6809WriteByte);
		M6809Close();
	
		BurnYM2151Init(3579545, 25.0);
		BurnYM2151SetIrqHandler(&DrvYM2151IrqHandler);
	
		MSM5205Init(0, 8000, 100, 1);
		MSM5205Init(1, 8000, 100, 1);
	}
	
	BurnSetRefreshRate(57.444853);
	
	nCyclesTotal[0] = (int)((double)12000000 / 57.44853);
	nCyclesTotal[1] = (int)((double)6000000 / 57.44853);
	nCyclesTotal[2] = (int)((double)6000000 / 57.44853);
	
	GenericTilesInit();

	// Reset the driver
	DrvDoReset();

	return 0;
}

static int Drv2MachineInit()
{
	// Setup the HD6309 emulation
	HD6309Init(1);
	HD6309Open(0);
	HD6309MapMemory(DrvHD6309Ram         , 0x0000, 0x17ff, M6809_RAM);
	HD6309MapMemory(DrvFgVideoRam        , 0x1800, 0x1fff, M6809_RAM);
	HD6309MapMemory(DrvSpriteRam         , 0x2000, 0x2fff, M6809_WRITE);
	HD6309MapMemory(DrvBgVideoRam        , 0x3000, 0x37ff, M6809_RAM);
	HD6309MapMemory(DrvPaletteRam1       , 0x3c00, 0x3dff, M6809_RAM);
	HD6309MapMemory(DrvPaletteRam2       , 0x3e00, 0x3fff, M6809_RAM);
	HD6309MapMemory(DrvHD6309Rom + 0x8000, 0x4000, 0x7fff, M6809_ROM);
	HD6309MapMemory(DrvHD6309Rom         , 0x8000, 0xffff, M6809_ROM);
	HD6309SetReadByteHandler(DrvDdragonHD6309ReadByte);
	HD6309SetWriteByteHandler(DrvDdragonHD6309WriteByte);
	HD6309Close();
	
	ZetInit(2);
	ZetOpen(0);
	ZetSetWriteHandler(Ddragon2SubZ80Write);
	ZetMapArea(0x0000, 0xbfff, 0, DrvSubCPURom);
	ZetMapArea(0x0000, 0xbfff, 2, DrvSubCPURom);
	ZetMapArea(0xc000, 0xc3ff, 0, DrvSpriteRam);
	ZetMapArea(0xc000, 0xc3ff, 2, DrvSpriteRam);
	ZetMemEnd();
	ZetClose();
	
	ZetOpen(1);
	ZetSetReadHandler(Ddragon2SoundZ80Read);
	ZetSetWriteHandler(Ddragon2SoundZ80Write);
	ZetMapArea(0x0000, 0x7fff, 0, DrvSoundCPURom);
	ZetMapArea(0x0000, 0x7fff, 2, DrvSoundCPURom);
	ZetMapArea(0x8000, 0x87ff, 0, DrvSoundCPURam);
	ZetMapArea(0x8000, 0x87ff, 1, DrvSoundCPURam);
	ZetMapArea(0x8000, 0x87ff, 2, DrvSoundCPURam);
	ZetMemEnd();
	ZetClose();
	
	BurnYM2151Init(3579545, 25.0);
	BurnYM2151SetIrqHandler(&Ddragon2YM2151IrqHandler);
	
	MSM6295Init(0, 1056000 / 132, 10.0, 1);
	
	BurnSetRefreshRate(57.444853);
	
	nCyclesTotal[0] = (int)((double)12000000 / 57.44853);
	nCyclesTotal[1] = (int)((double)4000000 / 57.44853);
	nCyclesTotal[2] = (int)((double)3579545 / 57.44853);

	GenericTilesInit();

	// Reset the driver
	DrvDoReset();

	return 0;
}

static int DrvInit()
{
	DrvSubCPUType = DD_CPU_TYPE_HD63701;
	DrvSoundCPUType = DD_CPU_TYPE_M6809;
	
	if (DrvMemInit()) return 1;
	if (DrvLoadRoms()) return 1;
	if (DrvMachineInit()) return 1;
	
	return 0;
}

static int DrvbInit()
{
	DrvSubCPUType = DD_CPU_TYPE_HD6309;
	DrvSoundCPUType = DD_CPU_TYPE_M6809;
	
	if (DrvMemInit()) return 1;
	if (DrvLoadRoms()) return 1;
	if (DrvMachineInit()) return 1;

	return 0;
}

static int DrvbaInit()
{
	DrvSubCPUType = DD_CPU_TYPE_M6803;
	DrvSoundCPUType = DD_CPU_TYPE_M6809;
	
	if (DrvMemInit()) return 1;
	if (DrvbaLoadRoms()) return 1;
	if (DrvMachineInit()) return 1;

	return 0;
}

static int Drv2Init()
{
	DrvSubCPUType = DD_CPU_TYPE_Z80;
	DrvSoundCPUType = DD_CPU_TYPE_Z80;
	DrvVidHardwareType = DD_VID_TYPE_DD2;
	
	if (Drv2MemInit()) return 1;
	if (Drv2LoadRoms()) return 1;
	if (Drv2MachineInit()) return 1;

	return 0;
}

static int DrvExit()
{
	HD6309Exit();
	M6800Exit();
	M6809Exit();
	ZetExit();
	
	BurnYM2151Exit();
	MSM5205Exit(0);
	MSM5205Exit(1);
	MSM6295Exit(0);
	
	GenericTilesExit();
	
	free(Mem);
	Mem = NULL;
	
	DrvRomBank = 0;
	DrvVBlank = 0;
	DrvSubCPUBusy = 0;
	DrvSoundLatch = 0;
	DrvScrollXHi = 0;
	DrvScrollYHi = 0;
	DrvScrollXLo = 0;
	DrvScrollYLo = 0;
	
	DrvADPCMIdle[0] = 0;
	DrvADPCMIdle[1] = 0;
	DrvADPCMPos[0] = 0;
	DrvADPCMPos[1] = 0;
	DrvADPCMEnd[0] = 0;
	DrvADPCMEnd[1] = 0;
	
	DrvSubCPUType = DD_CPU_TYPE_NONE;
	DrvSoundCPUType = DD_CPU_TYPE_NONE;
	DrvVidHardwareType = 0;

	return 0;
}

static inline unsigned char pal4bit(unsigned char bits)
{
	bits &= 0x0f;
	return (bits << 4) | bits;
}

inline static unsigned int CalcCol(unsigned short nColour)
{
	int r, g, b;

	r = pal4bit(nColour >> 0);
	g = pal4bit(nColour >> 4);
	b = pal4bit(nColour >> 8);

	return BurnHighCol(r, g, b, 0);
}

static void DrvCalcPalette()
{
	for (int i = 0; i < 0x180; i++) {
		int Val = DrvPaletteRam1[i] + (DrvPaletteRam2[i] << 8);
		
		DrvPalette[i] = CalcCol(Val);
	}
}

static void DrvRenderBgLayer()
{
	int mx, my, Code, Attr, Colour, x, y, TileIndex, xScroll, yScroll, Flip, xFlip, yFlip;
	
	xScroll = DrvScrollXHi + DrvScrollXLo;
	xScroll &= 0x1ff;
		
	yScroll = DrvScrollYHi + DrvScrollYLo;
	yScroll &= 0x1ff;

	for (mx = 0; mx < 32; mx++) {
		for (my = 0; my < 32; my++) {
			TileIndex = (my & 0x0f) + ((mx & 0x0f) << 4) + ((my & 0x10) << 4) + ((mx & 0x10) << 5);
			
			Attr = DrvBgVideoRam[(2 * TileIndex) + 0];
			Code = DrvBgVideoRam[(2 * TileIndex) + 1] + ((Attr & 0x07) << 8);
			Colour = (Attr >> 3) & 0x07;
			Flip = (Attr & 0xc0) >> 6;
			xFlip = (Flip >> 0) & 0x01;
			yFlip = (Flip >> 1) & 0x01;
			
			y = 16 * mx;
			x = 16 * my;

			x -= xScroll;
			if (x < -16) x += 512;
			
			y -= yScroll;
			if (y < -16) y += 512;
			
			y -= 8;

			if (x > 16 && x < 240 && y > 16 && y < 224) {
				if (xFlip) {
					if (yFlip) {
						Render16x16Tile_FlipXY(pTransDraw, Code, x, y, Colour, 4, 256, DrvTiles);
					} else {
						Render16x16Tile_FlipX(pTransDraw, Code, x, y, Colour, 4, 256, DrvTiles);
					}
				} else {
					if (yFlip) {
						Render16x16Tile_FlipY(pTransDraw, Code, x, y, Colour, 4, 256, DrvTiles);
					} else {
						Render16x16Tile(pTransDraw, Code, x, y, Colour, 4, 256, DrvTiles);
					}
				}
			} else {
				if (xFlip) {
					if (yFlip) {
						Render16x16Tile_FlipXY_Clip(pTransDraw, Code, x, y, Colour, 4, 256, DrvTiles);
					} else {
						Render16x16Tile_FlipX_Clip(pTransDraw, Code, x, y, Colour, 4, 256, DrvTiles);
					}
				} else {
					if (yFlip) {
						Render16x16Tile_FlipY_Clip(pTransDraw, Code, x, y, Colour, 4, 256, DrvTiles);
					} else {
						Render16x16Tile_Clip(pTransDraw, Code, x, y, Colour, 4, 256, DrvTiles);
					}
				}
			}
		}
	}
}

#define DRAW_SPRITE(Order, sx, sy)													\
	if (sx > 16 && sx < 240 && sy > 16 && sy < 224) {										\
		if (xFlip) {														\
			if (yFlip) {													\
				Render16x16Tile_Mask_FlipXY(pTransDraw, Code + Order, sx, sy, Colour, 4, 0, 128, DrvSprites);		\
			} else {													\
				Render16x16Tile_Mask_FlipX(pTransDraw, Code + Order, sx, sy, Colour, 4, 0, 128, DrvSprites);		\
			}														\
		} else {														\
			if (yFlip) {													\
				Render16x16Tile_Mask_FlipY(pTransDraw, Code + Order, sx, sy, Colour, 4, 0, 128, DrvSprites);		\
			} else {													\
				Render16x16Tile_Mask(pTransDraw, Code + Order, sx, sy, Colour, 4, 0, 128, DrvSprites);			\
			}														\
		}															\
	} else {															\
		if (xFlip) {														\
			if (yFlip) {													\
				Render16x16Tile_Mask_FlipXY_Clip(pTransDraw, Code + Order, sx, sy, Colour, 4, 0, 128, DrvSprites);	\
			} else {													\
				Render16x16Tile_Mask_FlipX_Clip(pTransDraw, Code + Order, sx, sy, Colour, 4, 0, 128, DrvSprites);	\
			}														\
		} else {														\
			if (yFlip) {													\
				Render16x16Tile_Mask_FlipY_Clip(pTransDraw, Code + Order, sx, sy, Colour, 4, 0, 128, DrvSprites);	\
			} else {													\
				Render16x16Tile_Mask_Clip(pTransDraw, Code + Order, sx, sy, Colour, 4, 0, 128, DrvSprites);		\
			}														\
		}															\
	}

static void DrvRenderSpriteLayer()
{
	unsigned char *Src = &DrvSpriteRam[0x800];
	
	for (int i = 0; i < (64 * 5); i += 5) {
		int Attr = Src[i + 1];
		
		if (Attr & 0x80) {
			int sx = 240 - Src[i + 4] + ((Attr & 2) << 7);
			int sy = 232 - Src[i + 0] + ((Attr & 1) << 8);
			int Size = (Attr & 0x30) >> 4;
			int xFlip = Attr & 0x08;
			int yFlip = Attr & 0x04;
			int dx = -16;
			int dy = -16;
			
			int Colour;
			int Code;
			
			if (DrvVidHardwareType == DD_VID_TYPE_DD2) {
				Colour = (Src[i + 2] >> 5);
				Code = Src[i + 3] + ((Src[i + 2] & 0x1f) << 8);
			} else {
				Colour = (Src[i + 2] >> 4) & 0x07;
				Code = Src[i + 3] + ((Src[i + 2] & 0x0f) << 8);
			}
			
			Code &= ~Size;
			
			switch (Size) {
				case 0: {
					DRAW_SPRITE(0, sx, sy)
					break;
				}
				
				case 1: {
					DRAW_SPRITE(0, sx, sy + dy)
					DRAW_SPRITE(1, sx, sy)
					break;
				}
				
				case 2: {
					DRAW_SPRITE(0, sx + dx, sy)
					DRAW_SPRITE(2, sx, sy)
					break;
				}
				
				case 3: {
					DRAW_SPRITE(0, sx + dx, sy + dy)
					DRAW_SPRITE(1, sx + dx, sy)
					DRAW_SPRITE(2, sx, sy + dy)
					DRAW_SPRITE(3, sx, sy)
					break;
				}
			}
		}
	}
}

#undef DRAW_SPRITE

static void DrvRenderCharLayer()
{
	int mx, my, Code, Attr, Colour, x, y, TileIndex = 0;

	for (my = 0; my < 32; my++) {
		for (mx = 0; mx < 32; mx++) {
			Attr = DrvFgVideoRam[(2 * TileIndex) + 0];
			Code = DrvFgVideoRam[(2 * TileIndex) + 1] + ((Attr & 0x07) << 8);
			if (DrvVidHardwareType != DD_VID_TYPE_DD2) Code &= 0x3ff;
			
			Colour = Attr >> 5;
			
			x = 8 * mx;
			y = 8 * my;
			
			y -= 8;

			if (x > 0 && x < 248 && y > 0 && y < 232) {
				Render8x8Tile_Mask(pTransDraw, Code, x, y, Colour, 4, 0, 0, DrvChars);
			} else {
				Render8x8Tile_Mask_Clip(pTransDraw, Code, x, y, Colour, 4, 0, 0, DrvChars);
			}

			TileIndex++;
		}
	}
}

static void DrvDraw()
{
	BurnTransferClear();
	DrvCalcPalette();
	DrvRenderBgLayer();
	DrvRenderSpriteLayer();
	DrvRenderCharLayer();
	BurnTransferCopy(DrvPalette);
}

static int DrvFrame()
{
	int nInterleave = 272;
	int nSoundBufferPos = 0;

	if (DrvReset) DrvDoReset();

	DrvMakeInputs();

	nCyclesDone[0] = nCyclesDone[1] = nCyclesDone[2] = 0;
	
	DrvVBlank = 0;
	
	for (int i = 0; i < nInterleave; i++) {
		int nCurrentCPU, nNext;

		nCurrentCPU = 0;
		HD6309Open(0);
		nNext = (i + 1) * nCyclesTotal[nCurrentCPU] / nInterleave;
		nCyclesSegment = nNext - nCyclesDone[nCurrentCPU];
		nCyclesDone[nCurrentCPU] += HD6309Run(nCyclesSegment);
		HD6309Close();
		
		if (DrvSubCPUType == DD_CPU_TYPE_HD63701) {
			nCurrentCPU = 1;
			nNext = (i + 1) * nCyclesTotal[nCurrentCPU] / nInterleave;
			nCyclesSegment = nNext - nCyclesDone[nCurrentCPU];
			nCyclesSegment = HD63701Run(nCyclesSegment);
			nCyclesDone[nCurrentCPU] += nCyclesSegment;
		}
		
		if (DrvSubCPUType == DD_CPU_TYPE_HD6309) {
			nCurrentCPU = 1;
			HD6309Open(1);
			nNext = (i + 1) * nCyclesTotal[nCurrentCPU] / nInterleave;
			nCyclesSegment = nNext - nCyclesDone[nCurrentCPU];
			nCyclesSegment = HD6309Run(nCyclesSegment);
			nCyclesDone[nCurrentCPU] += nCyclesSegment;
			HD6309Close();
		}
		
		if (DrvSubCPUType == DD_CPU_TYPE_M6803) {
			nCurrentCPU = 1;
			nNext = (i + 1) * nCyclesTotal[nCurrentCPU] / nInterleave;
			nCyclesSegment = nNext - nCyclesDone[nCurrentCPU];
			nCyclesSegment = M6803Run(nCyclesSegment);
			nCyclesDone[nCurrentCPU] += nCyclesSegment;
		}
		
		if (DrvSubCPUType == DD_CPU_TYPE_Z80) {
			nCurrentCPU = 1;
			ZetOpen(0);
			nNext = (i + 1) * nCyclesTotal[nCurrentCPU] / nInterleave;
			nCyclesSegment = nNext - nCyclesDone[nCurrentCPU];
			nCyclesSegment = ZetRun(nCyclesSegment);
			nCyclesDone[nCurrentCPU] += nCyclesSegment;
			ZetClose();
		}
		
		if (DrvSoundCPUType == DD_CPU_TYPE_M6809) {
			nCurrentCPU = 2;
			M6809Open(0);
			nNext = (i + 1) * nCyclesTotal[nCurrentCPU] / nInterleave;
			nCyclesSegment = nNext - nCyclesDone[nCurrentCPU];
			nCyclesDone[nCurrentCPU] += M6809Run(nCyclesSegment);
			M6809Close();
		}
		
		if (DrvSoundCPUType == DD_CPU_TYPE_Z80) {
			nCurrentCPU = 2;
			ZetOpen(1);
			nNext = (i + 1) * nCyclesTotal[nCurrentCPU] / nInterleave;
			nCyclesSegment = nNext - nCyclesDone[nCurrentCPU];
			nCyclesSegment = ZetRun(nCyclesSegment);
			nCyclesDone[nCurrentCPU] += nCyclesSegment;
			ZetClose();
		}
		
		if (i == 240) DrvVBlank = 1;
		if (i == 240) {
			HD6309Open(0);
			HD6309SetIRQ(HD6309_INPUT_LINE_NMI, HD6309_IRQSTATUS_ACK);
			HD6309Close();
		}
		
		if (i == 16 || i == 32 || i == 48 || i == 64 || i == 80 || i == 96 || i == 112 || i == 128 || i == 144 || i == 160 || i == 176 || i == 192 || i == 208 || i == 224 || i == 240 || i == 264) {
			HD6309Open(0);
			HD6309SetIRQ(HD6309_FIRQ_LINE, HD6309_IRQSTATUS_ACK);
			HD6309Close();
		}
		
		if (pBurnSoundOut) {
			int nSegmentLength = nBurnSoundLen / nInterleave;
			short* pSoundBuf = pBurnSoundOut + (nSoundBufferPos << 1);
			
			if (DrvSoundCPUType == DD_CPU_TYPE_M6809) {
				M6809Open(0);
				BurnYM2151Render(pSoundBuf, nSegmentLength);
				M6809Close();
				MSM5205Render(0, pSoundBuf, nSegmentLength);
				MSM5205Render(1, pSoundBuf, nSegmentLength);
			}
			
			if (DrvSoundCPUType == DD_CPU_TYPE_Z80) {
				ZetOpen(1);
				BurnYM2151Render(pSoundBuf, nSegmentLength);
				ZetClose();
				MSM6295Render(0, pSoundBuf, nSegmentLength);
			}
			
			nSoundBufferPos += nSegmentLength;
		}
	}
	
	// Make sure the buffer is entirely filled.
	if (pBurnSoundOut) {
		int nSegmentLength = nBurnSoundLen - nSoundBufferPos;
		short* pSoundBuf = pBurnSoundOut + (nSoundBufferPos << 1);

		if (nSegmentLength) {
			if (DrvSoundCPUType == DD_CPU_TYPE_M6809) {
				M6809Open(0);
				BurnYM2151Render(pSoundBuf, nSegmentLength);
				M6809Close();
				MSM5205Render(0, pSoundBuf, nSegmentLength);
				MSM5205Render(1, pSoundBuf, nSegmentLength);
			}
			
			if (DrvSoundCPUType == DD_CPU_TYPE_Z80) {
				ZetOpen(1);
				BurnYM2151Render(pSoundBuf, nSegmentLength);
				ZetClose();
				MSM6295Render(0, pSoundBuf, nSegmentLength);
			}
		}
	}

	if (pBurnDraw) DrvDraw();

	return 0;
}

static int DrvScan(int nAction, int *pnMin)
{
	struct BurnArea ba;
	
	if (pnMin != NULL) {			// Return minimum compatible version
		*pnMin = 0x029672;
	}

	if (nAction & ACB_MEMORY_RAM) {
		memset(&ba, 0, sizeof(ba));
		ba.Data	  = RamStart;
		ba.nLen	  = RamEnd-RamStart;
		ba.szName = "All Ram";
		BurnAcb(&ba);
	}

	return 0;
}

struct BurnDriver BurnDrvDdragon = {
	"ddragon", NULL, NULL, "1987",
	"Double Dragon (Japan)\0", NULL, "Technos", "Miscellaneous",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING, 2, HARDWARE_MISC_PRE90S, //GBF_SCRFIGHT, 0,
	NULL, DrvRomInfo, DrvRomName, DrvInputInfo, DrvDIPInfo,
	DrvInit, DrvExit, DrvFrame, NULL, DrvScan,
	0, NULL, NULL, NULL, NULL, 256, 240, 4, 3
};

struct BurnDriver BurnDrvDdragonw = {
	"ddragonw", "ddragon", NULL, "1987",
	"Double Dragon (World set 1)\0", NULL, "[Technos] (Taito license)", "Miscellaneous",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_CLONE, 2, HARDWARE_MISC_PRE90S, //GBF_SCRFIGHT, 0,
	NULL, DrvwRomInfo, DrvwRomName, DrvInputInfo, DrvDIPInfo,
	DrvInit, DrvExit, DrvFrame, NULL, DrvScan,
	0, NULL, NULL, NULL, NULL, 256, 240, 4, 3
};

struct BurnDriver BurnDrvDdragnw1 = {
	"ddragnw1", "ddragon", NULL, "1987",
	"Double Dragon (World set 2)\0", NULL, "[Technos] (Taito license)", "Miscellaneous",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_CLONE, 2, HARDWARE_MISC_PRE90S, //GBF_SCRFIGHT, 0,
	NULL, Drvw1RomInfo, Drvw1RomName, DrvInputInfo, DrvDIPInfo,
	DrvInit, DrvExit, DrvFrame, NULL, DrvScan,
	0, NULL, NULL, NULL, NULL, 256, 240, 4, 3
};

struct BurnDriver BurnDrvDdragonu = {
	"ddragonu", "ddragon", NULL, "1987",
	"Double Dragon (US set 1)\0", NULL, "[Technos] (Taito America license)", "Miscellaneous",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_CLONE, 2, HARDWARE_MISC_PRE90S, //GBF_SCRFIGHT, 0,
	NULL, DrvuRomInfo, DrvuRomName, DrvInputInfo, DrvDIPInfo,
	DrvInit, DrvExit, DrvFrame, NULL, DrvScan,
	0, NULL, NULL, NULL, NULL, 256, 240, 4, 3
};

struct BurnDriver BurnDrvDdragoua = {
	"ddragoua", "ddragon", NULL, "1987",
	"Double Dragon (US set 2)\0", NULL, "[Technos] (Taito America license)", "Miscellaneous",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_CLONE, 2, HARDWARE_MISC_PRE90S, //GBF_SCRFIGHT, 0,
	NULL, DrvuaRomInfo, DrvuaRomName, DrvInputInfo, DrvDIPInfo,
	DrvInit, DrvExit, DrvFrame, NULL, DrvScan,
	0, NULL, NULL, NULL, NULL, 256, 240, 4, 3
};

struct BurnDriver BurnDrvDdragob2 = {
	"ddragob2", "ddragon", NULL, "1987",
	"Double Dragon (bootleg)\0", NULL, "bootleg", "Miscellaneous",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_CLONE | BDF_BOOTLEG, 2, HARDWARE_MISC_PRE90S, //GBF_SCRFIGHT, 0,
	NULL, Drvb2RomInfo, Drvb2RomName, DrvInputInfo, DrvDIPInfo,
	DrvInit, DrvExit, DrvFrame, NULL, DrvScan,
	0, NULL, NULL, NULL, NULL, 256, 240, 4, 3
};

struct BurnDriver BurnDrvDdragonb = {
	"ddragonb", "ddragon", NULL, "1987",
	"Double Dragon (bootleg with HD6309)\0", NULL, "bootleg", "Miscellaneous",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_CLONE | BDF_BOOTLEG, 2, HARDWARE_MISC_PRE90S, //GBF_SCRFIGHT, 0,
	NULL, DrvbRomInfo, DrvbRomName, DrvInputInfo, DrvDIPInfo,
	DrvbInit, DrvExit, DrvFrame, NULL, DrvScan,
	0, NULL, NULL, NULL, NULL, 256, 240, 4, 3
};

struct BurnDriver BurnDrvDdragnba = {
	"ddragnba", "ddragon", NULL, "1987",
	"Double Dragon (bootleg with M6803)\0", NULL, "bootleg", "Miscellaneous",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_CLONE | BDF_BOOTLEG, 2, HARDWARE_MISC_PRE90S, //GBF_SCRFIGHT, 0,
	NULL, DrvbaRomInfo, DrvbaRomName, DrvInputInfo, DrvDIPInfo,
	DrvbaInit, DrvExit, DrvFrame, NULL, DrvScan,
	0, NULL, NULL, NULL, NULL, 256, 240, 4, 3
};

struct BurnDriver BurnDrvDdragon2 = {
	"ddragon2", NULL, NULL, "1988",
	"Double Dragon II - The Revenge (World)\0", NULL, "Technos", "Miscellaneous",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING, 2, HARDWARE_MISC_PRE90S, //GBF_SCRFIGHT, 0,
	NULL, Drv2RomInfo, Drv2RomName, DrvInputInfo, Drv2DIPInfo,
	Drv2Init, DrvExit, DrvFrame, NULL, DrvScan,
	0, NULL, NULL, NULL, NULL, 256, 240, 4, 3
};

struct BurnDriver BurnDrvDdragn2u = {
	"ddragn2u", "ddragon2", NULL, "1988",
	"Double Dragon II - The Revenge (US)\0", NULL, "Technos", "Miscellaneous",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_CLONE, 2, HARDWARE_MISC_PRE90S, //GBF_SCRFIGHT, 0,
	NULL, Drv2uRomInfo, Drv2uRomName, DrvInputInfo, Drv2DIPInfo,
	Drv2Init, DrvExit, DrvFrame, NULL, DrvScan,
	0, NULL, NULL, NULL, NULL, 256, 240, 4, 3
};
