#include "tiles_generic.h"
#include "z80.h"

static unsigned char DrvInputPort0[8] = {0, 0, 0, 0, 0, 0, 0, 0};
static unsigned char DrvInputPort1[8] = {0, 0, 0, 0, 0, 0, 0, 0};
static unsigned char DrvDip[3]        = {0, 0, 0};
static unsigned char DrvInput[2]      = {0x00, 0x00};
static unsigned char DrvReset         = 0;

static unsigned char *Mem                 = NULL;
static unsigned char *MemEnd              = NULL;
static unsigned char *RamStart            = NULL;
static unsigned char *RamEnd              = NULL;
static unsigned char *DrvZ80Rom1          = NULL;
static unsigned char *DrvZ80Rom2          = NULL;
static unsigned char *DrvZ80Rom3          = NULL;
static unsigned char *Drv54xxRom          = NULL;
static unsigned char *Drv51xxRom          = NULL;
static unsigned char *DrvVideoRam         = NULL;
static unsigned char *DrvSharedRam1       = NULL;
static unsigned char *DrvSharedRam2       = NULL;
static unsigned char *DrvSharedRam3       = NULL;
static unsigned char *DrvPromPalette      = NULL;
static unsigned char *DrvPromCharLookup   = NULL;
static unsigned char *DrvPromSpriteLookup = NULL;
static unsigned char *DrvChars            = NULL;
static unsigned char *DrvSprites          = NULL;
static unsigned char *DrvTempRom          = NULL;
static unsigned int  *DrvPalette          = NULL;

static unsigned char DrvCPU1FireIRQ;
static unsigned char DrvCPU2FireIRQ;
static unsigned char DrvCPU3FireIRQ;
static unsigned char DrvCPU2Halt;
static unsigned char DrvCPU3Halt;

static unsigned char IOChipCustomCommand;
static unsigned char IOChipCPU1FireIRQ;
static unsigned char IOChipMode;
static unsigned char IOChipCredits;
static unsigned char IOChipCoinPerCredit;
static unsigned char IOChipCreditPerCoin;
static unsigned char IOChipCustom[16];

static int nCyclesDone[4], nCyclesTotal[4];
static int nCyclesSegment;

static Z80_Regs Z80_0;
static Z80_Regs Z80_1;
static Z80_Regs Z80_2;
static Z80_Regs Z80_3;

struct CPU_Config {
	Z80ReadIoHandler Z80In;
	Z80WriteIoHandler Z80Out;
	Z80ReadProgHandler Z80Read;
	Z80WriteProgHandler Z80Write;
	Z80ReadOpHandler Z80ReadOp;
	Z80ReadOpArgHandler Z80ReadOpArg;
};

static int nActiveCPU;
static CPU_Config Z80_0_Config;
static CPU_Config Z80_1_Config;
static CPU_Config Z80_2_Config;
static CPU_Config Z80_3_Config;

static struct BurnInputInfo DrvInputList[] =
{
	{"Coin 1"            , BIT_DIGITAL  , DrvInputPort0 + 4, "p1 coin"   },
	{"Start 1"           , BIT_DIGITAL  , DrvInputPort0 + 2, "p1 start"  },
	{"Coin 2"            , BIT_DIGITAL  , DrvInputPort0 + 5, "p2 coin"   },
	{"Start 2"           , BIT_DIGITAL  , DrvInputPort0 + 3, "p2 start"  },

	{"Left"              , BIT_DIGITAL  , DrvInputPort1 + 3, "p1 left"   },
	{"Right"             , BIT_DIGITAL  , DrvInputPort1 + 1, "p1 right"  },
	{"Fire 1"            , BIT_DIGITAL  , DrvInputPort0 + 0, "p1 fire 1" },
	
	{"Left (Cocktail)"   , BIT_DIGITAL  , DrvInputPort1 + 7, "p2 left"   },
	{"Right (Cocktail)"  , BIT_DIGITAL  , DrvInputPort1 + 5, "p2 right"  },
	{"Fire 1 (Cocktail)" , BIT_DIGITAL  , DrvInputPort0 + 1, "p2 fire 1" },

	{"Reset"             , BIT_DIGITAL  , &DrvReset        , "reset"     },
	{"Service"           , BIT_DIGITAL  , DrvInputPort0 + 6, "service"   },
	{"Dip 1"             , BIT_DIPSWITCH, DrvDip + 0       , "dip"       },
	{"Dip 2"             , BIT_DIPSWITCH, DrvDip + 1       , "dip"       },
	{"Dip 3"             , BIT_DIPSWITCH, DrvDip + 2       , "dip"       },
};

STDINPUTINFO(Drv);

static inline void DrvMakeInputs()
{
	// Reset Inputs
	DrvInput[0] = 0x7f;
	DrvInput[1] = 0xff;

	// Compile Digital Inputs
	for (int i = 0; i < 8; i++) {
		DrvInput[0] -= (DrvInputPort0[i] & 1) << i;
		DrvInput[1] -= (DrvInputPort1[i] & 1) << i;
	}
}

static struct BurnDIPInfo DrvDIPList[]=
{
	// Default Values
	{0x0c, 0xff, 0xff, 0x80, NULL                     },
	{0x0d, 0xff, 0xff, 0xf7, NULL                     },
	{0x0e, 0xff, 0xff, 0x97, NULL                     },
	
	// Dip 1
	{0   , 0xfe, 0   , 2   , "Service Mode"           },
	{0x0c, 0x01, 0x80, 0x80, "Off"                    },
	{0x0c, 0x01, 0x08, 0x00, "On"                     },

	// Dip 2
	{0   , 0xfe, 0   , 4   , "Difficulty"             },
	{0x0d, 0x01, 0x03, 0x03, "Easy"                   },
	{0x0d, 0x01, 0x03, 0x00, "Medium"                 },
	{0x0d, 0x01, 0x03, 0x01, "Hard"                   },
	{0x0d, 0x01, 0x03, 0x02, "Hardest"                },
	
	{0   , 0xfe, 0   , 2   , "Demo Sounds"            },
	{0x0d, 0x01, 0x08, 0x08, "Off"                    },
	{0x0d, 0x01, 0x08, 0x00, "On"                     },
	
	{0   , 0xfe, 0   , 2   , "Freeze"                 },
	{0x0d, 0x01, 0x10, 0x10, "Off"                    },
	{0x0d, 0x01, 0x10, 0x00, "On"                     },
	
	{0   , 0xfe, 0   , 2   , "Rack Test"              },
	{0x0d, 0x01, 0x20, 0x20, "Off"                    },
	{0x0d, 0x01, 0x20, 0x00, "On"                     },
	
	{0   , 0xfe, 0   , 2   , "Cabinet"                },
	{0x0d, 0x01, 0x80, 0x80, "Upright"                },
	{0x0d, 0x01, 0x80, 0x00, "Cocktail"               },
	
	// Dip 3	
	{0   , 0xfe, 0   , 8   , "Coinage"                },
	{0x0e, 0x01, 0x07, 0x04, "4 Coins 1 Play"         },
	{0x0e, 0x01, 0x07, 0x02, "3 Coins 1 Play"         },
	{0x0e, 0x01, 0x07, 0x06, "2 Coins 1 Play"         },
	{0x0e, 0x01, 0x07, 0x07, "1 Coin  1 Play"         },
	{0x0e, 0x01, 0x07, 0x01, "2 Coins 3 Plays"        },
	{0x0e, 0x01, 0x07, 0x03, "1 Coin  2 Plays"        },
	{0x0e, 0x01, 0x07, 0x05, "1 Coin  3 Plays"        },
	{0x0e, 0x01, 0x07, 0x00, "Freeplay"               },	
	
	{0   , 0xfe, 0   , 8   , "Bonus Life"             },
	{0x0e, 0x01, 0x38, 0x20, "20k  60k  60k"          },
	{0x0e, 0x01, 0x38, 0x18, "20k  60k"               },
	{0x0e, 0x01, 0x38, 0x10, "20k  70k  70k"          },
	{0x0e, 0x01, 0x38, 0x30, "20k  80k  80k"          },
	{0x0e, 0x01, 0x38, 0x38, "30k  80k"               },
	{0x0e, 0x01, 0x38, 0x08, "30k 100k 100k"          },
	{0x0e, 0x01, 0x38, 0x28, "30k 120k 120k"          },
	{0x0e, 0x01, 0x38, 0x00, "None"                   },	
	
	{0   , 0xfe, 0   , 4   , "Lives"                  },
	{0x0e, 0x01, 0xc0, 0x00, "2"                      },
	{0x0e, 0x01, 0xc0, 0x80, "3"                      },
	{0x0e, 0x01, 0xc0, 0x40, "4"                      },
	{0x0e, 0x01, 0xc0, 0xc0, "5"                      },
};

STDDIPINFO(Drv);

static struct BurnRomInfo DrvRomDesc[] = {
	{ "gg1-1b.3p",     0x01000, 0xab036c9f, BRF_ESS | BRF_PRG }, //  0	Z80 #1 Program Code
	{ "gg1-2b.3m",     0x01000, 0xd9232240, BRF_ESS | BRF_PRG }, //	 1
	{ "gg1-3.2m",      0x01000, 0x753ce503, BRF_ESS | BRF_PRG }, //	 2
	{ "gg1-4b.2l",     0x01000, 0x499fcc76, BRF_ESS | BRF_PRG }, //	 3
	
	{ "gg1-5b.3f",     0x01000, 0xbb5caae3, BRF_ESS | BRF_PRG }, //  4	Z80 #2 Program Code
	
	{ "gg1-7b.2c",     0x01000, 0xd016686b, BRF_ESS | BRF_PRG }, //  5	Z80 #3 Program Code
	
	{ "54xx.bin",      0x00400, 0xee7357e0, BRF_ESS | BRF_PRG }, //  6	54XX Program Code
	
	{ "51xx.bin",      0x00400, 0xc2f57ef8, BRF_ESS | BRF_PRG }, //  7	51XX Program Code
	
	{ "gg1-9.4l",      0x01000, 0x58b2f47c, BRF_GRA },	     //  8	Characters
	
	{ "gg1-11.4d",     0x01000, 0xad447c80, BRF_GRA },	     //  9	Sprites
	{ "gg1-10.4f",     0x01000, 0xdd6f1afc, BRF_GRA },	     //  10
	
	{ "prom-5.5n",     0x00020, 0x54603c6b, BRF_GRA },	     //  11	PROMs
	{ "prom-4.2n",     0x00100, 0x59b6edab, BRF_GRA },	     //  12
	{ "prom-3.1c",     0x00100, 0x4a04bb6b, BRF_GRA },	     //  13
	{ "prom-1.1d",     0x00100, 0x7a2815b4, BRF_GRA },	     //  14
	{ "prom-2.5c",     0x00100, 0x77245b66, BRF_GRA },	     //  15
};

STD_ROM_PICK(Drv);
STD_ROM_FN(Drv);

static struct BurnRomInfo GallagRomDesc[] = {
	{ "gg1-1",         0x01000, 0xa3a0f743, BRF_ESS | BRF_PRG }, //  0	Z80 #1 Program Code
	{ "gallag.2",      0x01000, 0x5eda60a7, BRF_ESS | BRF_PRG }, //	 1
	{ "gg1-3.2m",      0x01000, 0x753ce503, BRF_ESS | BRF_PRG }, //	 2
	{ "gg1-4",         0x01000, 0x83874442, BRF_ESS | BRF_PRG }, //	 3
	
	{ "gg1-5",         0x01000, 0x3102fccd, BRF_ESS | BRF_PRG }, //  4	Z80 #2 Program Code
	
	{ "gg1-7",         0x01000, 0x8995088d, BRF_ESS | BRF_PRG }, //  5	Z80 #3 Program Code
	
	{ "gallag.6",      0x01000, 0x001b70bc, BRF_ESS | BRF_PRG }, //  6	Z80 #4 Program Code
	
	{ "gallag.8",      0x01000, 0x169a98a4, BRF_GRA },	     //  7	Characters
	
	{ "gg1-11.4d",     0x01000, 0xad447c80, BRF_GRA },	     //  8	Sprites
	{ "gg1-10.4f",     0x01000, 0xdd6f1afc, BRF_GRA },	     //  9
	
	{ "prom-5.5n",     0x00020, 0x54603c6b, BRF_GRA },	     //  10	PROMs
	{ "prom-4.2n",     0x00100, 0x59b6edab, BRF_GRA },	     //  11
	{ "prom-3.1c",     0x00100, 0x4a04bb6b, BRF_GRA },	     //  12
	{ "prom-1.1d",     0x00100, 0x7a2815b4, BRF_GRA },	     //  13
	{ "prom-2.5c",     0x00100, 0x77245b66, BRF_GRA },	     //  14
};

STD_ROM_PICK(Gallag);
STD_ROM_FN(Gallag);

static int MemIndex()
{
	unsigned char *Next; Next = Mem;

	DrvZ80Rom1             = Next; Next += 0x04000;
	DrvZ80Rom2             = Next; Next += 0x04000;
	DrvZ80Rom3             = Next; Next += 0x04000;
	Drv54xxRom             = Next; Next += 0x00400;
	Drv51xxRom             = Next; Next += 0x00400;
	DrvPromPalette         = Next; Next += 0x00020;
	DrvPromCharLookup      = Next; Next += 0x00100;
	DrvPromSpriteLookup    = Next; Next += 0x00100;
	
	RamStart               = Next;

	DrvVideoRam            = Next; Next += 0x00800;
	DrvSharedRam1          = Next; Next += 0x00400;
	DrvSharedRam1          = Next; Next += 0x04000;
	DrvSharedRam2          = Next; Next += 0x00400;
	DrvSharedRam3          = Next; Next += 0x00400;

	RamEnd                 = Next;

	DrvChars               = Next; Next += 0x100 * 8 * 8;
	DrvSprites             = Next; Next += 0x080 * 16 * 16;
	DrvPalette             = (unsigned int*)Next; Next += 576 * sizeof(unsigned int);

	MemEnd                 = Next;

	return 0;
}

static void OpenCPU(int nCPU)
{
	nActiveCPU = nCPU;
	
	switch (nCPU) {
		case 0: {
			Z80SetContext(&Z80_0);
			
			Z80SetIOReadHandler(Z80_0_Config.Z80In);
			Z80SetIOWriteHandler(Z80_0_Config.Z80Out);
			Z80SetProgramReadHandler(Z80_0_Config.Z80Read);
			Z80SetProgramWriteHandler(Z80_0_Config.Z80Write);
			Z80SetCPUOpArgReadHandler(Z80_0_Config.Z80ReadOpArg);
			Z80SetCPUOpReadHandler(Z80_0_Config.Z80ReadOp);
			return;
		}
		
		case 1: {
			Z80SetContext(&Z80_1);
			
			Z80SetIOReadHandler(Z80_1_Config.Z80In);
			Z80SetIOWriteHandler(Z80_1_Config.Z80Out);
			Z80SetProgramReadHandler(Z80_1_Config.Z80Read);
			Z80SetProgramWriteHandler(Z80_1_Config.Z80Write);
			Z80SetCPUOpArgReadHandler(Z80_1_Config.Z80ReadOpArg);
			Z80SetCPUOpReadHandler(Z80_1_Config.Z80ReadOp);
			return;
		}
		
		case 2: {
			Z80SetContext(&Z80_2);
			
			Z80SetIOReadHandler(Z80_2_Config.Z80In);
			Z80SetIOWriteHandler(Z80_2_Config.Z80Out);
			Z80SetProgramReadHandler(Z80_2_Config.Z80Read);
			Z80SetProgramWriteHandler(Z80_2_Config.Z80Write);
			Z80SetCPUOpArgReadHandler(Z80_2_Config.Z80ReadOpArg);
			Z80SetCPUOpReadHandler(Z80_2_Config.Z80ReadOp);
			return;
		}
		
		case 3: {
			Z80SetContext(&Z80_3);
			
			Z80SetIOReadHandler(Z80_3_Config.Z80In);
			Z80SetIOWriteHandler(Z80_3_Config.Z80Out);
			Z80SetProgramReadHandler(Z80_3_Config.Z80Read);
			Z80SetProgramWriteHandler(Z80_3_Config.Z80Write);
			Z80SetCPUOpArgReadHandler(Z80_3_Config.Z80ReadOpArg);
			Z80SetCPUOpReadHandler(Z80_3_Config.Z80ReadOp);
			return;
		}
	}
}

static void CloseCPU()
{
	switch (nActiveCPU) {
		case 0: {
			Z80GetContext(&Z80_0);
			return;
		}
		
		case 1: {
			Z80GetContext(&Z80_1);
			return;
		}
		
		case 2: {
			Z80GetContext(&Z80_2);
			return;
		}
		
		case 3: {
			Z80GetContext(&Z80_3);
			return;
		}
	}
	
	nActiveCPU = -1;
}

static int DrvDoReset()
{
	for (int i = 0; i < 3; i++) {
		OpenCPU(i);
		Z80Reset();
		CloseCPU();
	}
	
	DrvCPU1FireIRQ = 0;
	DrvCPU2FireIRQ = 0;
	DrvCPU3FireIRQ = 0;
	DrvCPU2Halt = 0;
	DrvCPU3Halt = 0;
	
	IOChipCustomCommand = 0;
	IOChipCPU1FireIRQ = 0;
	IOChipMode = 0;
	IOChipCredits = 0;
	IOChipCoinPerCredit = 0;
	IOChipCreditPerCoin = 0;
	for (int i = 0; i < 16; i++) {
		IOChipCustom[i] = 0;
	}

	return 0;
}

unsigned char __fastcall GalagaZ80PortRead(unsigned int a)
{
	a &= 0xff;
	
	switch (a) {
		default: {
			bprintf(PRINT_NORMAL, _T("Z80 #%i Port Read 02%x\n"), nActiveCPU, a);
		}
	}	

	return 0;
}

void __fastcall GalagaZ80PortWrite(unsigned int a, unsigned char d)
{
	a &= 0xff;
	
	switch (a) {
		default: {
			bprintf(PRINT_NORMAL, _T("Z80 #%i Port Write %02x, %02%x\n"), nActiveCPU, a, d);
		}
	}
}

unsigned char __fastcall GalagaZ80ProgRead(unsigned int a)
{
	if (a <= 0x3fff) {
		switch (nActiveCPU) {
			case 0: return DrvZ80Rom1[a];
			case 1: return DrvZ80Rom2[a];
			case 2: return DrvZ80Rom3[a];
		}
	}
	
	if (a >= 0x8000 && a <= 0x87ff) return DrvVideoRam[a - 0x8000];
	if (a >= 0x8800 && a <= 0x8bff) return DrvSharedRam1[a - 0x8800];
	if (a >= 0x9000 && a <= 0x93ff) return DrvSharedRam2[a - 0x9000];
	if (a >= 0x9800 && a <= 0x9bff) return DrvSharedRam3[a - 0x9800];
	
	switch (a) {
		case 0x6800:
		case 0x6801:
		case 0x6802:
		case 0x6803:
		case 0x6804:
		case 0x6805:
		case 0x6806:
		case 0x6807: {
			int Offset = a - 0x6800;
			int Bit0 = (DrvDip[2] >> Offset) & 0x01;
			int Bit1 = (DrvDip[1] >> Offset) & 0x01;

			return Bit0 | (Bit1 << 1);
		}
		
		case 0x7000:
		case 0x7001:
		case 0x7002:
		case 0x7003:
		case 0x7004:
		case 0x7005:
		case 0x7006:
		case 0x7007:
		case 0x7008:
		case 0x7009:
		case 0x700a:
		case 0x700b:
		case 0x700c:
		case 0x700d:
		case 0x700e:
		case 0x700f: {
			int Offset = a - 0x7000;
			
			switch (IOChipCustomCommand) {
				case 0x71:
				case 0xb1: {
					if (Offset == 0) {
						if (IOChipMode) {
							return (DrvInput[0] | DrvDip[0]);
						} else {
							unsigned char In;
							static unsigned char CoinInserted;
							
							In = DrvInput[0];
							if (IOChipCoinPerCredit > 0) {
								if ((In & 0x70) != 0x70 && IOChipCredits < 99) {
									CoinInserted++;
									if (CoinInserted >= IOChipCoinPerCredit) {
										IOChipCredits += IOChipCreditPerCoin;
										CoinInserted = 0;
									}
								}
							} else {
								IOChipCredits = 2;
							}
							
							if ((In & 0x04) == 0) {
								if (IOChipCredits >= 1) IOChipCredits--;
							}
							
							if ((In & 0x08) == 0) {
								if (IOChipCredits >= 2) IOChipCredits -= 2;
							}
							
							return (IOChipCredits / 10) * 16 + IOChipCredits % 10;
						}
					}
					
					if (Offset == 1) return DrvInput[1];
					if (Offset == 2) return 0xff;
				}
			}
			
			return 0x00;
		}
		
		case 0x7100: {
			return IOChipCustomCommand;
		}
		
		default: {
			bprintf(PRINT_NORMAL, _T("Z80 #%i Read %04x\n"), nActiveCPU, a);
		}
	}
	
	return 0;
}

void __fastcall GalagaZ80ProgWrite(unsigned int a, unsigned char d)
{
	if (a >= 0x8000 && a <= 0x87ff) { DrvVideoRam[a - 0x8000] = d; return; }
	if (a >= 0x8800 && a <= 0x8bff) { DrvSharedRam1[a - 0x8800] = d; return; }
	if (a >= 0x9000 && a <= 0x93ff) { DrvSharedRam2[a - 0x9000] = d; return; }
	if (a >= 0x9800 && a <= 0x9bff) { DrvSharedRam3[a - 0x9800] = d; return; }
	
	if (a >= 0x6800 && a <= 0x681f) return;
	
	switch (a) {
		case 0x6820: {
			DrvCPU1FireIRQ = d & 0x01;
			if (!DrvCPU1FireIRQ) {
			
			}
			return;
		}
		
		case 0x6821: {
			DrvCPU2FireIRQ = d & 0x01;
			if (!DrvCPU2FireIRQ) {
			
			}
			return;
		}
		
		case 0x6822: {
			DrvCPU3FireIRQ = !(d & 0x01);
			return;
		}
		
		case 0x6823: {
			if (!(d & 0x01)) {
				int nActive = nActiveCPU;
				CloseCPU();
				OpenCPU(1);
				Z80Reset();
				CloseCPU();
				OpenCPU(2);
				Z80Reset();
				CloseCPU();
				OpenCPU(nActive);
				DrvCPU2Halt = 1;
				DrvCPU3Halt = 1;
				return;
			} else {
				DrvCPU2Halt = 0;
				DrvCPU3Halt = 0;
			}
		}
		
		case 0x6830: {
			// watchdog write
			return;
		}
		
		case 0x7000:
		case 0x7001:
		case 0x7002:
		case 0x7003:
		case 0x7004:
		case 0x7005:
		case 0x7006:
		case 0x7007:
		case 0x7008:
		case 0x7009:
		case 0x700a:
		case 0x700b:
		case 0x700c:
		case 0x700d:
		case 0x700e:
		case 0x700f: {
			int Offset = a - 0x7000;
			IOChipCustom[Offset] = d;
			
			switch (IOChipCustomCommand) {

			}
			
			return;
		}
	
		case 0x7100: {
			IOChipCustomCommand = d;
			IOChipCPU1FireIRQ = 1;
			
			switch (IOChipCustomCommand) {
				case 0x10: {
					IOChipCPU1FireIRQ = 0;
					return;
				}
				
				case 0xa1: {
					IOChipMode = 1;
					return;
				}
				
				case 0xe1: {
					IOChipCredits = 0;
					IOChipMode = 0;
					return;
				}
			}
			
			return;
		}
		
		default: {
			bprintf(PRINT_NORMAL, _T("Z80 #%i Write %04x, %02x\n"), nActiveCPU, a, d);
		}
	}
}

unsigned char __fastcall GalagaZ80OpRead(unsigned int a)
{
	if (a <= 0x3fff) {
		switch (nActiveCPU) {
			case 0: return DrvZ80Rom1[a];
			case 1: return DrvZ80Rom2[a];
			case 2: return DrvZ80Rom3[a];
		}
	}
	
	switch (a) {
		default: {
			bprintf(PRINT_NORMAL, _T("Z80 #%i Op Read %04x\n"), nActiveCPU, a);
		}
	}
	
	return 0;
}

unsigned char __fastcall GalagaZ80OpArgRead(unsigned int a)
{
	if (a <= 0x3fff) {
		switch (nActiveCPU) {
			case 0: return DrvZ80Rom1[a];
			case 1: return DrvZ80Rom2[a];
			case 2: return DrvZ80Rom3[a];
		}
	}
	
	switch (a) {
		default: {
			bprintf(PRINT_NORMAL, _T("Z80 #%i Op Arg Read %04x\n"), nActiveCPU, a);
		}
	}
	
	return 0;
}

unsigned char __fastcall GalagaZ804PortRead(unsigned int a)
{
	a &= 0xff;
	
	switch (a) {
		default: {
			bprintf(PRINT_NORMAL, _T("Z80 #4 Port Read 02%x\n"), a);
		}
	}	

	return 0;
}

void __fastcall GalagaZ804PortWrite(unsigned int a, unsigned char d)
{
	a &= 0xff;
	
	switch (a) {
		default: {
			bprintf(PRINT_NORMAL, _T("Z80 #4 Port Write %02x, %02%x\n"), a, d);
		}
	}
}

unsigned char __fastcall GalagaZ804ProgRead(unsigned int a)
{
	switch (a) {
		default: {
			bprintf(PRINT_NORMAL, _T("Z80 #4 Read %04x\n"), a);
		}
	}
	
	return 0;
}

void __fastcall GalagaZ804ProgWrite(unsigned int a, unsigned char d)
{
	switch (a) {
		default: {
			bprintf(PRINT_NORMAL, _T("Z80 #4 Write %04x, %02x\n"), a, d);
		}
	}
}

unsigned char __fastcall GalagaZ804OpRead(unsigned int a)
{
	switch (a) {
		default: {
			bprintf(PRINT_NORMAL, _T("Z80 #4 Op Read %04x\n"), a);
		}
	}
	
	return 0;
}

unsigned char __fastcall GalagaZ804OpArgRead(unsigned int a)
{
	switch (a) {
		default: {
			bprintf(PRINT_NORMAL, _T("Z80 #4 Op Arg Read %04x\n"), a);
		}
	}
	
	return 0;
}

static int CharPlaneOffsets[2]   = { 0, 4 };
static int CharXOffsets[8]       = { 64, 65, 66, 67, 0, 1, 2, 3 };
static int CharYOffsets[8]       = { 0, 8, 16, 24, 32, 40, 48, 56 };
static int SpritePlaneOffsets[2] = { 0, 4 };
static int SpriteXOffsets[16]    = { 0, 1, 2, 3, 64, 65, 66, 67, 128, 129, 130, 131, 192, 193, 194, 195 };
static int SpriteYOffsets[16]    = { 0, 8, 16, 24, 32, 40, 48, 56, 256, 264, 272, 280, 288, 296, 304, 312 };

static int DrvInit()
{
	int nRet = 0, nLen;

	// Allocate and Blank all required memory
	Mem = NULL;
	MemIndex();
	nLen = MemEnd - (unsigned char *)0;
	if ((Mem = (unsigned char *)malloc(nLen)) == NULL) return 1;
	memset(Mem, 0, nLen);
	MemIndex();

	DrvTempRom = (unsigned char *)malloc(0x02000);

	// Load Z80 #1 Program Roms
	nRet = BurnLoadRom(DrvZ80Rom1 + 0x00000,  0, 1); if (nRet != 0) return 1;
	nRet = BurnLoadRom(DrvZ80Rom1 + 0x01000,  1, 1); if (nRet != 0) return 1;
	nRet = BurnLoadRom(DrvZ80Rom1 + 0x02000,  2, 1); if (nRet != 0) return 1;
	nRet = BurnLoadRom(DrvZ80Rom1 + 0x03000,  3, 1); if (nRet != 0) return 1;
	
	// Load Z80 #2 Program Roms
	nRet = BurnLoadRom(DrvZ80Rom2 + 0x00000,  4, 1); if (nRet != 0) return 1;
	
	// Load Z80 #3 Program Roms
	nRet = BurnLoadRom(DrvZ80Rom3 + 0x00000,  5, 1); if (nRet != 0) return 1;
	
	// Load and decode the chars
	nRet = BurnLoadRom(DrvTempRom,            8, 1); if (nRet != 0) return 1;
	GfxDecode(0x100, 2, 8, 8, CharPlaneOffsets, CharXOffsets, CharYOffsets, 0x80, DrvTempRom, DrvChars);
	
	// Load and decode the sprites
	memset(DrvTempRom, 0, 0x02000);
	nRet = BurnLoadRom(DrvTempRom + 0x00000,  9, 1); if (nRet != 0) return 1;
	nRet = BurnLoadRom(DrvTempRom + 0x01000, 10, 1); if (nRet != 0) return 1;
	GfxDecode(0x80, 2, 16, 16, SpritePlaneOffsets, SpriteXOffsets, SpriteYOffsets, 0x200, DrvTempRom, DrvSprites);

	// Load the PROMs
	nRet = BurnLoadRom(DrvPromPalette,       11, 1); if (nRet != 0) return 1;
	nRet = BurnLoadRom(DrvPromCharLookup,    12, 1); if (nRet != 0) return 1;
	nRet = BurnLoadRom(DrvPromSpriteLookup,  13, 1); if (nRet != 0) return 1;
	
	free(DrvTempRom);
	
	// Setup the Z80 emulation
	Z80Init();
	
	Z80_0_Config.Z80In = GalagaZ80PortRead;
	Z80_0_Config.Z80Out = GalagaZ80PortWrite;
	Z80_0_Config.Z80Read = GalagaZ80ProgRead;
	Z80_0_Config.Z80Write = GalagaZ80ProgWrite;
	Z80_0_Config.Z80ReadOpArg = GalagaZ80OpArgRead;
	Z80_0_Config.Z80ReadOp = GalagaZ80OpRead;

	GenericTilesInit();

	// Reset the driver
	DrvDoReset();

	return 0;
}

static int GallagInit()
{
	int nRet = 0, nLen;

	// Allocate and Blank all required memory
	Mem = NULL;
	MemIndex();
	nLen = MemEnd - (unsigned char *)0;
	if ((Mem = (unsigned char *)malloc(nLen)) == NULL) return 1;
	memset(Mem, 0, nLen);
	MemIndex();

	DrvTempRom = (unsigned char *)malloc(0x02000);

	// Load Z80 #1 Program Roms
	nRet = BurnLoadRom(DrvZ80Rom1 + 0x00000,  0, 1); if (nRet != 0) return 1;
	nRet = BurnLoadRom(DrvZ80Rom1 + 0x01000,  1, 1); if (nRet != 0) return 1;
	nRet = BurnLoadRom(DrvZ80Rom1 + 0x02000,  2, 1); if (nRet != 0) return 1;
	nRet = BurnLoadRom(DrvZ80Rom1 + 0x03000,  3, 1); if (nRet != 0) return 1;
	
	// Load Z80 #2 Program Roms
	nRet = BurnLoadRom(DrvZ80Rom2 + 0x00000,  4, 1); if (nRet != 0) return 1;
	
	// Load Z80 #3 Program Roms
	nRet = BurnLoadRom(DrvZ80Rom3 + 0x00000,  5, 1); if (nRet != 0) return 1;
	
	// Load and decode the chars
	nRet = BurnLoadRom(DrvTempRom,            7, 1); if (nRet != 0) return 1;
	GfxDecode(0x100, 2, 8, 8, CharPlaneOffsets, CharXOffsets, CharYOffsets, 0x80, DrvTempRom, DrvChars);
	
	// Load and decode the sprites
	memset(DrvTempRom, 0, 0x02000);
	nRet = BurnLoadRom(DrvTempRom + 0x00000,  8, 1); if (nRet != 0) return 1;
	nRet = BurnLoadRom(DrvTempRom + 0x01000,  9, 1); if (nRet != 0) return 1;
	GfxDecode(0x80, 2, 16, 16, SpritePlaneOffsets, SpriteXOffsets, SpriteYOffsets, 0x200, DrvTempRom, DrvSprites);

	// Load the PROMs
	nRet = BurnLoadRom(DrvPromPalette,       10, 1); if (nRet != 0) return 1;
	nRet = BurnLoadRom(DrvPromCharLookup,    11, 1); if (nRet != 0) return 1;
	nRet = BurnLoadRom(DrvPromSpriteLookup,  12, 1); if (nRet != 0) return 1;
	
	free(DrvTempRom);
	
	// Setup the Z80 emulation
	Z80Init();
	
	Z80_0_Config.Z80In = GalagaZ80PortRead;
	Z80_0_Config.Z80Out = GalagaZ80PortWrite;
	Z80_0_Config.Z80Read = GalagaZ80ProgRead;
	Z80_0_Config.Z80Write = GalagaZ80ProgWrite;
	Z80_0_Config.Z80ReadOpArg = GalagaZ80OpArgRead;
	Z80_0_Config.Z80ReadOp = GalagaZ80OpRead;
	
	Z80_1_Config.Z80In = GalagaZ80PortRead;
	Z80_1_Config.Z80Out = GalagaZ80PortWrite;
	Z80_1_Config.Z80Read = GalagaZ80ProgRead;
	Z80_1_Config.Z80Write = GalagaZ80ProgWrite;
	Z80_1_Config.Z80ReadOpArg = GalagaZ80OpArgRead;
	Z80_1_Config.Z80ReadOp = GalagaZ80OpRead;
	
	Z80_2_Config.Z80In = GalagaZ80PortRead;
	Z80_2_Config.Z80Out = GalagaZ80PortWrite;
	Z80_2_Config.Z80Read = GalagaZ80ProgRead;
	Z80_2_Config.Z80Write = GalagaZ80ProgWrite;
	Z80_2_Config.Z80ReadOpArg = GalagaZ80OpArgRead;
	Z80_2_Config.Z80ReadOp = GalagaZ80OpRead;
	
	Z80_3_Config.Z80In = GalagaZ804PortRead;
	Z80_3_Config.Z80Out = GalagaZ804PortWrite;
	Z80_3_Config.Z80Read = GalagaZ804ProgRead;
	Z80_3_Config.Z80Write = GalagaZ804ProgWrite;
	Z80_3_Config.Z80ReadOpArg = GalagaZ804OpArgRead;
	Z80_3_Config.Z80ReadOp = GalagaZ804OpRead;

	GenericTilesInit();

	// Reset the driver
	DrvDoReset();

	return 0;
}

static int DrvExit()
{
	GenericTilesExit();
	
	free(Mem);
	Mem = NULL;
	
	DrvCPU1FireIRQ = 0;
	DrvCPU2FireIRQ = 0;
	DrvCPU3FireIRQ = 0;
	DrvCPU2Halt = 0;
	DrvCPU3Halt = 0;
	
	IOChipCustomCommand = 0;
	IOChipCPU1FireIRQ = 0;
	IOChipMode = 0;
	IOChipCredits = 0;
	IOChipCoinPerCredit = 0;
	IOChipCreditPerCoin = 0;
	for (int i = 0; i < 16; i++) {
		IOChipCustom[i] = 0;
	}
	
	nActiveCPU = 0;

	return 0;
}

static void DrvCalcPalette()
{
	int i;
	unsigned int Palette[96];
	
	for (i = 0; i < 32; i++) {
		int bit0, bit1, bit2, r, g, b;
		
		bit0 = (DrvPromPalette[i] >> 0) & 0x01;
		bit1 = (DrvPromPalette[i] >> 1) & 0x01;
		bit2 = (DrvPromPalette[i] >> 2) & 0x01;
		r = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
		bit0 = (DrvPromPalette[i] >> 3) & 0x01;
		bit1 = (DrvPromPalette[i] >> 4) & 0x01;
		bit2 = (DrvPromPalette[i] >> 5) & 0x01;
		g = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
		bit0 = 0;
		bit1 = (DrvPromPalette[i] >> 6) & 0x01;
		bit2 = (DrvPromPalette[i] >> 7) & 0x01;
		b = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
		
		Palette[i] = BurnHighCol(r, g, b, 0);
	}
	
	for (i = 0; i < 64; i++) {
		int bits, r, g, b;
		static const int map[4] = { 0x00, 0x47, 0x97, 0xde };
		
		bits = (i >> 0) & 0x03;
		r = map[bits];
		bits = (i >> 2) & 0x03;
		g = map[bits];
		bits = (i >> 4) & 0x03;
		b = map[bits];
		
		Palette[32 + i] = BurnHighCol(r, g, b, 0);
	}
	
	for (i = 0; i < 256; i++) {
		DrvPalette[i] = Palette[((DrvPromCharLookup[i]) & 0x0f) + 0x10];
	}
	
	for (i = 0; i < 256; i++) {
		DrvPalette[256 + i] = Palette[DrvPromSpriteLookup[i] & 0x0f];
	}
	
	for (i = 0; i < 64; i++) {
		DrvPalette[512 + i] = Palette[32 + i];
	}
}

static void DrvRenderTilemap()
{
	int mx, my, Code, Colour, x, y, TileIndex, Row, Col;

	for (mx = 0; mx < 28; mx++) {
		for (my = 0; my < 36; my++) {
			Row = mx + 2;
			Col = my - 2;
			if (Col & 0x20) {
				TileIndex = Row + ((Col & 0x1f) << 5);
			} else {
				TileIndex = Col + (Row << 5);
			}
			
/*			int color = galaga_videoram[tile_index + 0x400] & 0x3f;
	SET_TILE_INFO(
			0,
			(galaga_videoram[tile_index] & 0x7f) | (flip_screen_get() ? 0x80 : 0) | (galaga_gfxbank << 8),
			color,
			flip_screen_get() ? TILE_FLIPX : 0);
	tileinfo->group = color;*/
			
			Code = DrvVideoRam[TileIndex + 0x000] & 0x7f;
			Colour = DrvVideoRam[TileIndex + 0x400] & 0x3f;

			y = 8 * mx;
			x = 8 * my;
			
//			x -= xScroll;
//			if (x < -16) x += 512;
			
//			y -= yScroll;
//			if (y < -16) y += 512;
			
//			y -= 16;

			if (x > 0 && x < 280 && y > 0 && y < 216) {
				Render8x8Tile(pTransDraw, Code, x, y, Colour, 2, 0, DrvChars);
			} else {
				Render8x8Tile_Clip(pTransDraw, Code, x, y, Colour, 2, 0, DrvChars);
			}
		}
	}
}

static void DrvRenderSprites()
{
	unsigned char *SpriteRam1 = DrvSharedRam1 + 0x380;
	unsigned char *SpriteRam2 = DrvSharedRam2 + 0x380;
	unsigned char *SpriteRam3 = DrvSharedRam3 + 0x380;
	
	for (int Offset = 0; Offset < 0x80; Offset += 2) {
		static const int GfxOffset[2][2] = {
			{ 0, 1 },
			{ 2, 3 }
		};
		int Sprite = SpriteRam1[Offset + 0] & 0x7f;
		int Colour = SpriteRam1[Offset + 1] & 0x3f;
		int sx = SpriteRam2[Offset + 1] - 40 + (0x100 * (SpriteRam3[Offset + 1] & 0x03));
		int sy = 256 - SpriteRam2[Offset + 0] + 1;
		int xFlip = (SpriteRam3[Offset + 0] & 0x01);
		int yFlip = (SpriteRam3[Offset + 0] & 0x02) >> 1;
		int xSize = (SpriteRam3[Offset + 0] & 0x04) >> 2;
		int ySize = (SpriteRam3[Offset + 0] & 0x08) >> 3;

		sy -= 16 * ySize;
		sy = (sy & 0xff) - 32;

//		if (flip_screen_get(machine))
//		{
//			flipx ^= 1;
//			flipy ^= 1;
//			sy += 48;
//		}

		for (int y = 0; y <= ySize; y++) {
			for (int x = 0; x <= xSize; x++) {
//				drawgfx(bitmap,machine->gfx[1],
//					sprite + gfx_offs[y ^ (sizey * flipy)][x ^ (sizex * flipx)],
//					color,
//					flipx,flipy,
//					sx + 16*x, sy + 16*y,
//					cliprect,TRANSPARENCY_PENS,
//					colortable_get_transpen_mask(machine->colortable, machine->gfx[1], color, 0x0f));
				int Code = Sprite + GfxOffset[y ^ (ySize * yFlip)][x ^ (xSize * xFlip)];
				Render16x16Tile_Mask_Clip(pTransDraw, Code, sx + 16 * x, sy + 16 * y, Colour, 2, 0, 256, DrvSprites);
			}
		}
	}
}

static void DrvDraw()
{
	BurnTransferClear();
	DrvCalcPalette();
	
	DrvRenderTilemap();
	DrvRenderSprites();	
	BurnTransferCopy(DrvPalette);
}

static int DrvFrame()
{
	int nInterleave = 200;

	if (DrvReset) DrvDoReset();

	DrvMakeInputs();

	nCyclesTotal[0] = (18432000 / 6) / 60;
	nCyclesTotal[1] = (18432000 / 6) / 60;
	nCyclesTotal[2] = (18432000 / 6) / 60;
	nCyclesDone[0] = nCyclesDone[1] = nCyclesDone[2] = 0;
	
	for (int i = 0; i < nInterleave; i++) {
		int nCurrentCPU, nNext;
		
		// Run Z80 #1
		nCurrentCPU = 0;
		OpenCPU(nCurrentCPU);
		nNext = (i + 1) * nCyclesTotal[nCurrentCPU] / nInterleave;
		nCyclesSegment = nNext - nCyclesDone[nCurrentCPU];
		nCyclesSegment = Z80Execute(nCyclesSegment);
		nCyclesDone[nCurrentCPU] += nCyclesSegment;
		if (i == (nInterleave - 1) && DrvCPU1FireIRQ) {
			Z80SetIrqLine(0, 1);
			Z80Execute(0);
			Z80SetIrqLine(0, 0);
			Z80Execute(0);
		}		
		if ((i == 50 || i == 100 || i == 150) && IOChipCPU1FireIRQ) {
			Z80SetIrqLine(Z80_INPUT_LINE_NMI, 1);
			Z80Execute(0);
			Z80SetIrqLine(Z80_INPUT_LINE_NMI, 0);
			Z80Execute(0);	
		}
		CloseCPU();
		
		if (!DrvCPU2Halt) {
			nCurrentCPU = 1;
			OpenCPU(nCurrentCPU);
			nNext = (i + 1) * nCyclesTotal[nCurrentCPU] / nInterleave;
			nCyclesSegment = nNext - nCyclesDone[nCurrentCPU];
			nCyclesSegment = Z80Execute(nCyclesSegment);
			nCyclesDone[nCurrentCPU] += nCyclesSegment;
			if (i == (nInterleave - 1) && DrvCPU2FireIRQ) {
				Z80SetIrqLine(0, 1);
				Z80Execute(0);
				Z80SetIrqLine(0, 0);
				Z80Execute(0);
			}
			CloseCPU();
		}
		
		if (!DrvCPU3Halt) {
			nCurrentCPU = 2;
			OpenCPU(nCurrentCPU);
			nNext = (i + 1) * nCyclesTotal[nCurrentCPU] / nInterleave;
			nCyclesSegment = nNext - nCyclesDone[nCurrentCPU];
			nCyclesSegment = Z80Execute(nCyclesSegment);
			nCyclesDone[nCurrentCPU] += nCyclesSegment;
			if ((i == 99 || i == 199) && DrvCPU3FireIRQ) {
				Z80SetIrqLine(Z80_INPUT_LINE_NMI, 1);
				Z80Execute(0);
				Z80SetIrqLine(Z80_INPUT_LINE_NMI, 0);
				Z80Execute(0);
			}
			CloseCPU();
		}
	}

	if (pBurnDraw) DrvDraw();

	return 0;
}

static int DrvScan(int nAction, int *pnMin)
{
	struct BurnArea ba;
	
	if (pnMin != NULL) {			// Return minimum compatible version
		*pnMin = 0x029698;
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

struct BurnDriverD BurnDrvGalaga = {
	"galaga", NULL, NULL, "1981",
	"Galaga (Namco rev. B)\0", NULL, "Namco", "Miscellaneous",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_ORIENTATION_VERTICAL | BDF_ORIENTATION_FLIPPED, 2, HARDWARE_MISC_PRE90S, GBF_VERSHOOT, 0,
	NULL, DrvRomInfo, DrvRomName, DrvInputInfo, DrvDIPInfo,
	DrvInit, DrvExit, DrvFrame, NULL, DrvScan,
	0, NULL, NULL, NULL, NULL, 224, 288, 3, 4
};

struct BurnDriverD BurnDrvGallag = {
	"gallag", "galaga", NULL, "1981",
	"Gallag\0", NULL, "bootleg", "Miscellaneous",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_CLONE | BDF_ORIENTATION_VERTICAL | BDF_ORIENTATION_FLIPPED | BDF_BOOTLEG, 2, HARDWARE_MISC_PRE90S, GBF_VERSHOOT, 0,
	NULL, GallagRomInfo, GallagRomName, DrvInputInfo, DrvDIPInfo,
	GallagInit, DrvExit, DrvFrame, NULL, DrvScan,
	0, NULL, NULL, NULL, NULL, 224, 288, 3, 4
};
