// metmqstr
#include "cave.h"
#include "burn_ym2151.h"
#include "msm6295.h"

#define CAVE_VBLANK_LINES 12

static unsigned char DrvJoy1[11] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
static unsigned char DrvJoy2[11] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
static unsigned short DrvInput[2] = {0x0000, 0x0000};

static unsigned char *Mem = NULL, *MemEnd = NULL;
static unsigned char *RamStart, *RamEnd;
static unsigned char *Rom01, *RomZ80;
static unsigned char *Ram01, *RamZ80;
static unsigned char *MSM6295ROMSrc1, *MSM6295ROMSrc2;

static unsigned char DrvReset = 0;
static unsigned char bDrawScreen;
static bool bVBlank;

static char nVideoIRQ;
static char nSoundIRQ;
static char nUnknownIRQ;

static char nIRQPending;

static int nCyclesTotal[2];
static int nCyclesDone[2];

static int SoundLatch;
static int SoundLatchReply[48];
static int SoundLatchStatus;

static int SoundLatchReplyIndex;
static int SoundLatchReplyMax;

static unsigned char DrvZ80Bank;
static unsigned char DrvOkiBank1_1;
static unsigned char DrvOkiBank1_2;
static unsigned char DrvOkiBank2_1;
static unsigned char DrvOkiBank2_2;

static struct BurnInputInfo metmqstrInputList[] = {
	{"P1 Coin",		BIT_DIGITAL,	DrvJoy1 + 8,	"p1 coin"},
	{"P1 Start",	BIT_DIGITAL,	DrvJoy1 + 7,	"p1 start"},

	{"P1 Up",		BIT_DIGITAL,	DrvJoy1 + 0, 	"p1 up"},
	{"P1 Down",		BIT_DIGITAL,	DrvJoy1 + 1, 	"p1 down"},
	{"P1 Left",		BIT_DIGITAL,	DrvJoy1 + 2, 	"p1 left"},
	{"P1 Right",	BIT_DIGITAL,	DrvJoy1 + 3, 	"p1 right"},
	{"P1 Button 1",	BIT_DIGITAL,	DrvJoy1 + 4,	"p1 fire 1"},
	{"P1 Button 2",	BIT_DIGITAL,	DrvJoy1 + 5,	"p1 fire 2"},
	{"P1 Button 3",	BIT_DIGITAL,	DrvJoy1 + 6,	"p1 fire 3"},
	{"P1 Button 4",	BIT_DIGITAL,	DrvJoy1 + 10,	"p1 fire 4"},

	{"P2 Coin",		BIT_DIGITAL,	DrvJoy2 + 8,	"p2 coin"},
	{"P2 Start",	BIT_DIGITAL,	DrvJoy2 + 7,	"p2 start"},

	{"P2 Up",		BIT_DIGITAL,	DrvJoy2 + 0, 	"p2 up"},
	{"P2 Down",		BIT_DIGITAL,	DrvJoy2 + 1, 	"p2 down"},
	{"P2 Left",		BIT_DIGITAL,	DrvJoy2 + 2, 	"p2 left"},
	{"P2 Right",	BIT_DIGITAL,	DrvJoy2 + 3, 	"p2 right"},
	{"P2 Button 1",	BIT_DIGITAL,	DrvJoy2 + 4,	"p2 fire 1"},
	{"P2 Button 2",	BIT_DIGITAL,	DrvJoy2 + 5,	"p2 fire 2"},
	{"P2 Button 3",	BIT_DIGITAL,	DrvJoy2 + 6,	"p2 fire 3"},
	{"P2 Button 4",	BIT_DIGITAL,	DrvJoy2 + 10,	"p2 fire 4"},

	{"Reset",		BIT_DIGITAL,	&DrvReset,		"reset"},
	{"Diagnostics",	BIT_DIGITAL,	DrvJoy1 + 9,	"diag"},
	{"Service",		BIT_DIGITAL,	DrvJoy2 + 9,	"service"},
};

STDINPUTINFO(metmqstr);

static void UpdateIRQStatus()
{
	nIRQPending = (nVideoIRQ == 0 || nSoundIRQ == 0 || nUnknownIRQ == 0);
	SekSetIRQLine(1, nIRQPending ? SEK_IRQSTATUS_ACK : SEK_IRQSTATUS_NONE);
}

unsigned char __fastcall metmqstrReadByte(unsigned int sekAddress)
{
/*
	switch (sekAddress) {
		default: {
 			bprintf(PRINT_NORMAL, _T("Attempt to read byte value of location %x\n"), sekAddress);
		}
	}
*/
	return 0;
}

void __fastcall metmqstrWriteByte(unsigned int sekAddress, unsigned char byteValue)
{
/*
	switch (sekAddress) {
		default: {
			bprintf(PRINT_NORMAL, _T("Attempt to write byte value %x to location %x\n"), byteValue, sekAddress);

		}
	}
*/
}

unsigned short __fastcall metmqstrReadWord(unsigned int sekAddress)
{
	switch (sekAddress) {
		case 0xa80000:
		case 0xa80002: {
			unsigned char nRet = (nUnknownIRQ << 1) | nVideoIRQ;
			return nRet;
		}
		case 0xa80004: {
			unsigned char nRet = (nUnknownIRQ << 1) | nVideoIRQ;
			nVideoIRQ = 1;
			UpdateIRQStatus();
			return nRet;
		}
		case 0xa80006: {
			unsigned char nRet = (nUnknownIRQ << 1) | nVideoIRQ;
			nUnknownIRQ = 1;
			UpdateIRQStatus();
			return nRet;
		}
		
		case 0xa8006C:
			if (SoundLatchReplyIndex > SoundLatchReplyMax) {
				return 2;
			}
			return 0;

		case 0xa8006E:
			if (SoundLatchReplyIndex > SoundLatchReplyMax) {
				SoundLatchReplyIndex = 0;
				SoundLatchReplyMax = -1;
				return 0;
			}
			return SoundLatchReply[SoundLatchReplyIndex++];
		
		case 0xc80000:
			return DrvInput[0] ^ 0xFFFF;
		case 0xc80002:
			return DrvInput[1] ^ 0xF7FF | (EEPROMRead() << 11);		
		/*
		default: {
 			bprintf(PRINT_NORMAL, _T("Attempt to read word value of location %x\n"), sekAddress);
		}
		*/
	}
	return 0;
}

void __fastcall metmqstrWriteWord(unsigned int sekAddress, unsigned short wordValue)
{
	if (sekAddress >= 0xa8000a && sekAddress <= 0xa80068) return;
	if (sekAddress >= 0xa8006a && sekAddress <= 0xa8006c) return;
	if (sekAddress >= 0xa80004 && sekAddress <= 0xa80006) return;
	
	switch (sekAddress) {
		case 0xa80000:
			nCaveXOffset = wordValue;
			return;
		case 0xa80002:
			nCaveYOffset = wordValue;
			return;
			
		case 0xa80008:
			CaveSpriteBuffer();
			nCaveSpriteBank = wordValue;
			return;
			
		case 0xa8006E:
			SoundLatch = wordValue;
			SoundLatchStatus |= 0x0C;

			ZetOpen(0);
			ZetNmi();
			nCyclesDone[1] += ZetRun(0x0400);
			ZetClose();
			return;
			
		case 0xb00000:
			CaveTileReg[2][0] = wordValue;
			break;
		case 0xb00002:
			CaveTileReg[2][1] = wordValue;
			break;
		case 0xb00004:
			CaveTileReg[2][2] = wordValue;
			break;
			
		case 0xb80000:
			CaveTileReg[1][0] = wordValue;
			break;
		case 0xb80002:
			CaveTileReg[1][1] = wordValue;
			break;
		case 0xb80004:
			CaveTileReg[1][2] = wordValue;
			break;
			
		case 0xc00000:
			CaveTileReg[0][0] = wordValue;
			break;
		case 0xc00002:
			CaveTileReg[0][1] = wordValue;
			break;
		case 0xc00004:
			CaveTileReg[0][2] = wordValue;
			break;
		
		case 0xd00000:
			wordValue >>= 8;
			EEPROMWrite(wordValue & 0x04, wordValue & 0x02, wordValue & 0x08);
			break;
/*
		default: {
			bprintf(PRINT_NORMAL, _T("Attempt to write word value %x to location %x\n"), wordValue, sekAddress);
		}
*/
	}
}

unsigned char __fastcall metmqstrZIn(unsigned short nAddress)
{
	nAddress &= 0xFF;

	switch (nAddress) {
		case 0x20: {
			return 0;
		}
		
		case 0x30:
			SoundLatchStatus |= 0x04;
			return SoundLatch & 0xFF;

		case 0x40:
			SoundLatchStatus |= 0x08;
			return SoundLatch >> 8;
		
		case 0x51:
			return BurnYM2151ReadStatus();
		/*
		default: {
			bprintf(PRINT_NORMAL, _T("Z80 Port Read %x\n"), nAddress);
		}
		*/
	}

	return 0;
}

void __fastcall metmqstrZOut(unsigned short nAddress, unsigned char nValue)
{
	nAddress &= 0xFF;

	switch (nAddress) {
		case 0x00: {
			DrvZ80Bank = nValue & 0x0f;
			
			ZetMapArea(0x4000, 0x7FFF, 0, RomZ80 + (DrvZ80Bank * 0x4000));
			ZetMapArea(0x4000, 0x7FFF, 2, RomZ80 + (DrvZ80Bank * 0x4000));
			return;
		}
		
		case 0x50:
			BurnYM2151SelectRegister(nValue);
			break;
		case 0x51:
			BurnYM2151WriteRegister(nValue);
			break;
			
		case 0x60: {
			MSM6295Command(0, nValue);
			return;
		}
		
		case 0x70: {
			DrvOkiBank1_1 = (nValue >> 0) & 0x07;
			DrvOkiBank1_2 = (nValue >> 4) & 0x07;
			
			memcpy(MSM6295ROM + 0x000000, MSM6295ROMSrc1 + 0x20000 * DrvOkiBank1_1, 0x20000);
			memcpy(MSM6295ROM + 0x020000, MSM6295ROMSrc1 + 0x20000 * DrvOkiBank1_2, 0x20000);
			return;
		}
		
		case 0x80: {
			MSM6295Command(1, nValue);
			return;
		}
		
		case 0x90: {
			DrvOkiBank2_1 = (nValue >> 0) & 0x07;
			DrvOkiBank2_2 = (nValue >> 4) & 0x07;
			
			memcpy(MSM6295ROM + 0x100000, MSM6295ROMSrc2 + 0x20000 * DrvOkiBank2_1, 0x20000);
			memcpy(MSM6295ROM + 0x120000, MSM6295ROMSrc2 + 0x20000 * DrvOkiBank2_2, 0x20000);
			return;
		}
/*
		default: {
			bprintf(PRINT_NORMAL, _T("Z80 Port Write %x, %x\n"), nAddress, nValue);
		}
*/
	}
}

unsigned char __fastcall metmqstrZRead(unsigned short a)
{
/*
	switch (a) {
		default: {
			bprintf(PRINT_NORMAL, _T("Z80 Read => %04X\n"), a);
		}
	}
*/
	return 0;
}

void __fastcall metmqstrZWrite(unsigned short a, unsigned char d)
{
/*
	switch (a) {
		default: {
			bprintf(PRINT_NORMAL, _T("Z80 Write => %04X, %02X\n"), a, d);
		}
	}
*/
}

static int DrvExit()
{
	EEPROMExit();

	BurnYM2151Exit();
	MSM6295Exit(0);
	MSM6295Exit(1);

	CaveTileExit();
	CaveSpriteExit();
    CavePalExit();

	SekExit();				// Deallocate 68000s
	ZetExit();
	
	SoundLatch = 0;
	DrvZ80Bank = 0;
	DrvOkiBank1_1 = 0;
	DrvOkiBank1_2 = 0;
	DrvOkiBank2_1 = 0;
	DrvOkiBank2_2 = 0;

	// Deallocate all used memory
	free(Mem);
	Mem = NULL;
destroyUniCache();
	return 0;
}

static int DrvDoReset()
{
	SekOpen(0);
	SekReset();
	SekRun(10000);	// Need to run for a bit and reset to make it start - Watchdog would force reset?
	SekReset();
	SekClose();
	
	ZetOpen(0);
	ZetReset();
	ZetClose();
	
	BurnYM2151Reset();
	MSM6295Reset(0);
	MSM6295Reset(1);

	EEPROMReset();
	
	nVideoIRQ = 1;
	nSoundIRQ = 1;
	nUnknownIRQ = 1;

	nIRQPending = 0;
	
	SoundLatch = 0;
	DrvZ80Bank = 0;
	DrvOkiBank1_1 = 0;
	DrvOkiBank1_2 = 0;
	DrvOkiBank2_1 = 0;
	DrvOkiBank2_2 = 0;
	
	SoundLatch = 0;
	SoundLatchStatus = 0x0C;

	memset(SoundLatchReply, 0, sizeof(SoundLatchReply));
	SoundLatchReplyIndex = 0;
	SoundLatchReplyMax = -1;
	
	return 0;
}

static int DrvDraw()
{
	CavePalUpdate4Bit(0, 128);

	CaveClearScreen(CavePalette[0x7F00]);

	if (bDrawScreen) {
//		CaveGetBitmap();

		CaveTileRender(1);					// Render tiles
	}

	return 0;
}

inline static int CheckSleep(int)
{
	return 0;
}

static int DrvFrame()
{
	int nCyclesVBlank;
	int nInterleave = 8;
	int nSoundBufferPos = 0;

	int nCyclesSegment;

	

	if (DrvReset) {														// Reset machine
		DrvDoReset();
	}

	// Compile digital inputs
	DrvInput[0] = 0x0000;  												// Player 1
	DrvInput[1] = 0x0000;  												// Player 2
	for (int i = 0; i < 11; i++) {
		DrvInput[0] |= (DrvJoy1[i] & 1) << i;
		DrvInput[1] |= (DrvJoy2[i] & 1) << i;
	}
	CaveClearOpposites(&DrvInput[0]);
	CaveClearOpposites(&DrvInput[1]);

	SekNewFrame();
	ZetNewFrame();
	
	SekOpen(0);
	
	nCyclesTotal[0] = (int)((long long)16000000 * nBurnCPUSpeedAdjust / (0x0100 * CAVE_REFRESHRATE));
	nCyclesTotal[1] = (int)(8000000 / CAVE_REFRESHRATE);
	nCyclesDone[0] = nCyclesDone[1] = 0;

	nCyclesVBlank = nCyclesTotal[0] - (int)((nCyclesTotal[0] * CAVE_VBLANK_LINES) / 271.5);
	bVBlank = false;

	for (int i = 1; i <= nInterleave; i++) {
    	int nCurrentCPU = 0;
		int nNext = i * nCyclesTotal[nCurrentCPU] / nInterleave;

		// Run 68000

		// See if we need to trigger the VBlank interrupt
		if (!bVBlank && nNext > nCyclesVBlank) {
			if (nCyclesDone[nCurrentCPU] < nCyclesVBlank) {
				nCyclesSegment = nCyclesVBlank - nCyclesDone[nCurrentCPU];
				if (!CheckSleep(nCurrentCPU)) {							// See if this CPU is busywaiting
					nCyclesDone[nCurrentCPU] += SekRun(nCyclesSegment);
				} else {
					nCyclesDone[nCurrentCPU] += SekIdle(nCyclesSegment);
				}
			}

			if (pBurnDraw != NULL) {
				DrvDraw();												// Draw screen if needed
			}
			
			unsigned char Temp = nCaveSpriteBank;
			nCaveSpriteBank = nCaveSpriteBankDelay;
			nCaveSpriteBankDelay = Temp;

			bVBlank = true;
			nVideoIRQ = 0;
			nUnknownIRQ = 0;
			UpdateIRQStatus();
		}
		
		SekSetIRQLine(1, SEK_IRQSTATUS_AUTO);
		
		nCyclesSegment = nNext - nCyclesDone[nCurrentCPU];
		if (!CheckSleep(nCurrentCPU)) {									// See if this CPU is busywaiting
			nCyclesDone[nCurrentCPU] += SekRun(nCyclesSegment);
		} else {
			nCyclesDone[nCurrentCPU] += SekIdle(nCyclesSegment);
		}
		
		nCurrentCPU = 1;
		ZetOpen(0);
		nNext = (i + 1) * nCyclesTotal[nCurrentCPU] / nInterleave;
		nCyclesSegment = nNext - nCyclesDone[nCurrentCPU];
		nCyclesSegment = ZetRun(nCyclesSegment);
		nCyclesDone[nCurrentCPU] += nCyclesSegment;
		ZetClose();
		
		if (pBurnSoundOut) {
			int nSegmentLength = nBurnSoundLen / nInterleave;
			short* pSoundBuf = pBurnSoundOut + (nSoundBufferPos << 1);
			ZetOpen(0);
			BurnYM2151Render(pSoundBuf, nSegmentLength);
			ZetClose();
			MSM6295Render(0, pSoundBuf, nSegmentLength);
			MSM6295Render(1, pSoundBuf, nSegmentLength);
			nSoundBufferPos += nSegmentLength;
		}
	}
	
	// Make sure the buffer is entirely filled.
	if (pBurnSoundOut) {
		int nSegmentLength = nBurnSoundLen - nSoundBufferPos;
		short* pSoundBuf = pBurnSoundOut + (nSoundBufferPos << 1);

		if (nSegmentLength) {
			ZetOpen(0);
			BurnYM2151Render(pSoundBuf, nSegmentLength);
			ZetClose();
			MSM6295Render(0, pSoundBuf, nSegmentLength);
			MSM6295Render(1, pSoundBuf, nSegmentLength);
		}
	}
	
	SekClose();
	
	return 0;
}

// This routine is called first to determine how much memory is needed (MemEnd-(unsigned char *)0),
// and then afterwards to set up all the pointers
static int MemIndex()
{
	unsigned char* Next; Next = Mem;
	Rom01			= Next; Next += 0x180000;		// 68K program
	RomZ80			= Next; Next += 0x040000;
/*	CaveSpriteROM	= Next; Next += 0x1000000;
	CaveTileROM[0]	= Next; Next += 0x400000;		// Tile layer 0
	CaveTileROM[1]	= Next; Next += 0x400000;		// Tile layer 1
	CaveTileROM[2]	= Next; Next += 0x400000;		// Tile layer 2
*/
	MSM6295ROM		= Next; Next += 0x140000;
	MSM6295ROMSrc1		= Next; Next += 0x200000;
	MSM6295ROMSrc2		= Next; Next += 0x200000;
	RamStart		= Next;
	Ram01			= Next; Next += 0x018000;		// CPU #0 work RAM
	RamZ80			= Next; Next += 0x002000;
	CaveTileRAM[0]	= Next; Next += 0x008000;
	CaveTileRAM[1]	= Next; Next += 0x008000;
	CaveTileRAM[2]	= Next; Next += 0x008000;
	CaveSpriteRAM	= Next; Next += 0x010000;
	CavePalSrc		= Next; Next += 0x010000;		// palette
	RamEnd			= Next;
	MemEnd			= Next;

	return 0;
}

static void NibbleSwap1(unsigned char* pData, int nLen)
{
	unsigned char* pOrg = pData + nLen - 1;
	unsigned char* pDest = pData + ((nLen - 1) << 1);

	for (int i = 0; i < nLen; i++, pOrg--, pDest -= 2) {
		pDest[0] = *pOrg & 15;
		pDest[1] = *pOrg >> 4;
	}

	return;
}

static void NibbleSwap2(unsigned char* pData, int nLen)
{
	unsigned char* pOrg = pData + nLen - 1;
	unsigned char* pDest = pData + ((nLen - 1) << 1);

	for (int i = 0; i < nLen; i++, pOrg--, pDest -= 2) {
		pDest[1] = *pOrg & 15;
		pDest[0] = *pOrg >> 4;
	}

	return;
}

static int LoadRoms()
{
	BurnLoadRom(Rom01 + 0x000000, 0, 1);
	BurnLoadRom(Rom01 + 0x080000, 1, 1);
	BurnLoadRom(Rom01 + 0x100000, 2, 1);
	
	BurnLoadRom(RomZ80, 3, 1);
/*
	BurnLoadRom(CaveSpriteROM + 0x000000, 4, 1);
	BurnLoadRom(CaveSpriteROM + 0x200000, 5, 1);
	BurnLoadRom(CaveSpriteROM + 0x400000, 6, 1);
	BurnLoadRom(CaveSpriteROM + 0x600000, 7, 1);
	NibbleSwap1(CaveSpriteROM, 0x800000);

	BurnLoadRom(CaveTileROM[0], 8, 1);
	NibbleSwap2(CaveTileROM[0], 0x200000);
	
	BurnLoadRom(CaveTileROM[1], 9, 1);
	NibbleSwap2(CaveTileROM[1], 0x200000);
	
	BurnLoadRom(CaveTileROM[2], 10, 1);
	NibbleSwap2(CaveTileROM[2], 0x200000);
*/	
	// Load MSM6295 ADPCM data
	BurnLoadRom(MSM6295ROMSrc1, 11, 1);
	BurnLoadRom(MSM6295ROMSrc2, 12, 1);

	return 0;
}

// Scan ram
static int DrvScan(int nAction, int *pnMin)
{
	struct BurnArea ba;

	if (pnMin) {						// Return minimum compatible version
		*pnMin = 0x020902;
	}

	EEPROMScan(nAction, pnMin);			// Scan EEPROM

	if (nAction & ACB_VOLATILE) {		// Scan volatile ram
		memset(&ba, 0, sizeof(ba));
    		ba.Data		= RamStart;
		ba.nLen		= RamEnd - RamStart;
		ba.szName	= "RAM";
		BurnAcb(&ba);

		SekScan(nAction);
		ZetScan(nAction);

		BurnYM2151Scan(nAction);
		MSM6295Scan(0, nAction);
		MSM6295Scan(1, nAction);

		SCAN_VAR(nVideoIRQ);
		SCAN_VAR(nSoundIRQ);
		SCAN_VAR(nUnknownIRQ);
		SCAN_VAR(bVBlank);

		CaveScanGraphics();

		SCAN_VAR(DrvInput);
		SCAN_VAR(SoundLatch);
		SCAN_VAR(DrvZ80Bank);
		SCAN_VAR(DrvOkiBank1_1);
		SCAN_VAR(DrvOkiBank1_2);
		SCAN_VAR(DrvOkiBank2_1);
		SCAN_VAR(DrvOkiBank2_2);
		
		if (nAction & ACB_WRITE) {
			ZetOpen(0);
			ZetMapArea(0x4000, 0x7FFF, 0, RomZ80 + (DrvZ80Bank * 0x4000));
			ZetMapArea(0x4000, 0x7FFF, 2, RomZ80 + (DrvZ80Bank * 0x4000));
			ZetClose();
			
			memcpy(MSM6295ROM + 0x000000, MSM6295ROMSrc1 + 0x20000 * DrvOkiBank1_1, 0x20000);
			memcpy(MSM6295ROM + 0x020000, MSM6295ROMSrc1 + 0x20000 * DrvOkiBank1_2, 0x20000);
			
			memcpy(MSM6295ROM + 0x100000, MSM6295ROMSrc2 + 0x20000 * DrvOkiBank2_1, 0x20000);
			memcpy(MSM6295ROM + 0x120000, MSM6295ROMSrc2 + 0x20000 * DrvOkiBank2_2, 0x20000);

			CaveRecalcPalette = 1;
		}
	}

	return 0;
}

static int drvZInit()
{
	ZetInit(1);
	
	ZetOpen(0);

	ZetSetInHandler(metmqstrZIn);
	ZetSetOutHandler(metmqstrZOut);
	ZetSetReadHandler(metmqstrZRead);
	ZetSetWriteHandler(metmqstrZWrite);

	// ROM bank 1
	ZetMapArea    (0x0000, 0x3FFF, 0, RomZ80 + 0x0000); // Direct Read from ROM
	ZetMapArea    (0x0000, 0x3FFF, 2, RomZ80 + 0x0000); // Direct Fetch from ROM
	// ROM bank 2
	ZetMapArea    (0x4000, 0x7FFF, 0, RomZ80 + 0x4000); // Direct Read from ROM
	ZetMapArea    (0x4000, 0x7FFF, 2, RomZ80 + 0x4000); //
	// RAM
	ZetMapArea    (0xe000, 0xFFFF, 0, RamZ80 + 0x0000);			// Direct Read from RAM
	ZetMapArea    (0xe000, 0xFFFF, 1, RamZ80 + 0x0000);			// Direct Write to RAM
	ZetMapArea    (0xe000, 0xFFFF, 2, RamZ80 + 0x0000);			//

	ZetMemEnd();
	
	ZetClose();

	return 0;
}

static void DrvYM2151IrqHandler(int Irq)
{
	if (Irq) {
		ZetSetIRQLine(0, ZET_IRQSTATUS_ACK);
	} else {
		ZetSetIRQLine(0, ZET_IRQSTATUS_NONE);
	}
}

static int DrvInit()
{
	int nLen;

	BurnSetRefreshRate(CAVE_REFRESHRATE);
	cacheFileSize=0x1C00000;
		
	extern char szAppCachePath[];
		
	strcpy(filePathName, szAppCachePath);
	strcat(filePathName, BurnDrvGetTextA(DRV_NAME));
	strcat(filePathName, "_LB");
	needCreateCache = false;
	cacheFile = sceIoOpen( filePathName, PSP_O_RDONLY, 0777);
	if (cacheFile<0)
	{
		needCreateCache = true;
		cacheFile = sceIoOpen( filePathName, PSP_O_RDWR|PSP_O_CREAT, 0777 );
	}else if(sceIoLseek(cacheFile,0,SEEK_END)!=cacheFileSize)
	{
		needCreateCache = true;
		sceIoClose(cacheFile);
		cacheFile = sceIoOpen( filePathName, PSP_O_RDWR|PSP_O_TRUNC, 0777 );
	}
	
	// Load Sprite and Tile
	CaveSpriteROMOffset=0;
	CaveTileROMOffset[0]=CaveSpriteROMOffset+0x1000000;
	CaveTileROMOffset[1]=CaveTileROMOffset[0]+0x400000;
	CaveTileROMOffset[2]=CaveTileROMOffset[1]+0x400000;
	if(needCreateCache)
	{
		if ((uniCacheHead = (unsigned char *)malloc(0x1000000)) == NULL) return 1;
		memset(uniCacheHead,0,0x1000000);

		BurnLoadRom(uniCacheHead + 0x000000, 4, 1);
		BurnLoadRom(uniCacheHead + 0x200000, 5, 1);
		BurnLoadRom(uniCacheHead + 0x400000, 6, 1);
		BurnLoadRom(uniCacheHead + 0x600000, 7, 1);
		NibbleSwap1(uniCacheHead, 0x800000);
	
		for(int j=0;j<5;j++)
		{
			sceIoLseek( cacheFile, 0, SEEK_SET );
			if( 0x1000000 == sceIoWrite(cacheFile,uniCacheHead, 0x1000000 ) )
				break;
		}
		
		BurnLoadRom(uniCacheHead, 8, 1);
		NibbleSwap2(uniCacheHead, 0x200000);
		
		BurnLoadRom(uniCacheHead+0x400000, 9, 1);
		NibbleSwap2(uniCacheHead+0x400000, 0x200000);
		
		BurnLoadRom(uniCacheHead+0x800000, 10, 1);
		NibbleSwap2(uniCacheHead+0x800000, 0x200000);
		
		for(int j=0;j<5;j++)
		{
			sceIoLseek( cacheFile,0x1000000, SEEK_SET );
			if( 0xC00000 == sceIoWrite(cacheFile,uniCacheHead, 0xC00000 ) )
				break;
		}

		sceIoClose( cacheFile );
		cacheFile = sceIoOpen( filePathName,PSP_O_RDONLY, 0777);
		free(uniCacheHead);
		uniCacheHead=NULL;
	}

	
	// Find out how much memory is needed
	Mem = NULL;
	MemIndex();
	nLen = MemEnd - (unsigned char *)0;
	if ((Mem = (unsigned char *)malloc(nLen)) == NULL) {
		return 1;
	}
	memset(Mem, 0, nLen);										// blank all memory
	MemIndex();													// Index the allocated memory

	EEPROMInit(1024, 16);										// EEPROM has 1024 bits, uses 16-bit words
	initCacheStructure(0.7);
	
	// Load the roms into memory
	if (LoadRoms()) {
		return 1;
	}

	{
		SekInit(0, 0x68000);													// Allocate 68000
	    SekOpen(0);

		// Map 68000 memory:
		SekMapMemory(Rom01,				0x000000, 0x07FFFF, SM_ROM);	// CPU 0 ROM
		SekMapMemory(Rom01 + 0x080000,			0x100000, 0x17FFFF, SM_ROM);
		SekMapMemory(Rom01 + 0x100000,			0x200000, 0x27FFFF, SM_ROM);
		SekMapMemory(CaveTileRAM[2],			0x880000, 0x887FFF, SM_RAM);
		SekMapMemory(Ram01 + 0x00000,			0x888000, 0x88FFFF, SM_RAM);
		SekMapMemory(CaveTileRAM[1],			0x900000, 0x907FFF, SM_RAM);
		SekMapMemory(Ram01 + 0x08000,			0x908000, 0x90FFFF, SM_RAM);
		SekMapMemory(CaveTileRAM[0],			0x980000, 0x987FFF, SM_RAM);
		SekMapMemory(Ram01 + 0x10000,			0x988000, 0x98FFFF, SM_RAM);
		SekMapMemory(CavePalSrc,		   	0x408000, 0x408FFF, SM_RAM);	// Palette RAM
		SekMapMemory(CaveSpriteRAM,			0xF00000, 0xF0FFFF, SM_RAM);
		SekSetReadByteHandler(0, metmqstrReadByte);
		SekSetWriteByteHandler(0, metmqstrWriteByte);
		SekSetReadWordHandler(0, metmqstrReadWord);
		SekSetWriteWordHandler(0, metmqstrWriteWord);
		SekClose();
	}
	
	drvZInit();

	CavePalInit(0x8000);
	CaveTileInit();
	CaveSpriteInit(2, 0x1000000);
	CaveTileInitLayer(0, 0x400000, 8, 0x4000);
	CaveTileInitLayer(1, 0x400000, 8, 0x4000);
	CaveTileInitLayer(2, 0x400000, 8, 0x4000);
	
	nCaveExtraXOffset = -126;
	CaveSpriteVisibleXOffset = -126;
	
	BurnYM2151Init(4000000, 25.0);
	BurnYM2151SetIrqHandler(&DrvYM2151IrqHandler);
	
	memcpy(MSM6295ROM, MSM6295ROMSrc1, 0x40000);
	memcpy(MSM6295ROM + 0x100000, MSM6295ROMSrc2, 0x40000);
	MSM6295Init(0, 2000000 / 132, 100.0, 1);
	MSM6295Init(1, 2000000 / 132, 100.0, 1);
	
	bDrawScreen = true;

#if defined FBA_DEBUG && defined USE_SPEEDHACKS
	bprintf(PRINT_IMPORTANT, _T("  * Using speed-hacks (detecting idle loops).\n"));
#endif

	DrvDoReset(); // Reset machine

	return 0;
}

// Rom information
static struct BurnRomInfo metmqstrRomDesc[] = {
	{ "bp947a.u25",   0x080000, 0x0a5c3442, BRF_ESS | BRF_PRG }, //  0 CPU #0 code
	{ "bp947a.u28",   0x080000, 0x8c55decf, BRF_ESS | BRF_PRG }, //  1
	{ "bp947a.u29",   0x080000, 0xcf0f3f3b, BRF_ESS | BRF_PRG }, //  2
	
	{ "bp947a.u20",   0x040000, 0xa4a36170, BRF_ESS | BRF_PRG }, //  3 Z80 Code

	{ "bp947a.u49",   0x200000, 0x09749531, BRF_GRA },	     //  4 Sprite data
	{ "bp947a.u50",   0x200000, 0x19cea8b2, BRF_GRA },	     //  5
	{ "bp947a.u51",   0x200000, 0xc19bed67, BRF_GRA },	     //  6
	{ "bp947a.u52",   0x200000, 0x70c64875, BRF_GRA },	     //  7

	{ "bp947a.u48",   0x200000, 0x04ff6a3d, BRF_GRA },			 //  8 Layer 0 Tile data
	{ "bp947a.u47",   0x200000, 0x0de42827, BRF_GRA },			 //  9 Layer 2 Tile data
	{ "bp947a.u46",   0x200000, 0x0f9c906e, BRF_GRA },			 //  10 Layer 2 Tile data

	{ "bp947a.u42",   0x200000, 0x2ce8ff2a, BRF_SND },			 //  11 MSM6295 #1 ADPCM data
	{ "bp947a.u37",   0x200000, 0xc3077c8f, BRF_SND },			 //  12 MSM6295 #2 ADPCM data
};


STD_ROM_PICK(metmqstr);
STD_ROM_FN(metmqstr);

static struct BurnRomInfo nmasterRomDesc[] = {
	{ "bp947a_n.u25", 0x080000, 0x748cc514, BRF_ESS | BRF_PRG }, //  0 CPU #0 code
	{ "bp947a.u28",   0x080000, 0x8c55decf, BRF_ESS | BRF_PRG }, //  1
	{ "bp947a.u29",   0x080000, 0xcf0f3f3b, BRF_ESS | BRF_PRG }, //  2
	
	{ "bp947a.u20",   0x040000, 0xa4a36170, BRF_ESS | BRF_PRG }, //  3 Z80 Code

	{ "bp947a.u49",   0x200000, 0x09749531, BRF_GRA },	     //  4 Sprite data
	{ "bp947a.u50",   0x200000, 0x19cea8b2, BRF_GRA },	     //  5
	{ "bp947a.u51",   0x200000, 0xc19bed67, BRF_GRA },	     //  6
	{ "bp947a.u52",   0x200000, 0x70c64875, BRF_GRA },	     //  7

	{ "bp947a.u48",   0x200000, 0x04ff6a3d, BRF_GRA },			 //  8 Layer 0 Tile data
	{ "bp947a.u47",   0x200000, 0x0de42827, BRF_GRA },			 //  9 Layer 2 Tile data
	{ "bp947a.u46",   0x200000, 0x0f9c906e, BRF_GRA },			 //  10 Layer 2 Tile data

	{ "bp947a.u42",   0x200000, 0x2ce8ff2a, BRF_SND },			 //  11 MSM6295 #1 ADPCM data
	{ "bp947a.u37",   0x200000, 0xc3077c8f, BRF_SND },			 //  12 MSM6295 #2 ADPCM data
};


STD_ROM_PICK(nmaster);
STD_ROM_FN(nmaster);

struct BurnDriver BurnDrvmetmqstr = {
	"metmqstr", NULL, NULL, "1995",
	"Metamoqester (International)\0", NULL, "Banpresto/Pandorabox", "Cave",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_16BIT_ONLY, 2, HARDWARE_CAVE_68K_Z80,
	NULL, metmqstrRomInfo, metmqstrRomName, metmqstrInputInfo, NULL,
	DrvInit, DrvExit, DrvFrame, DrvDraw, DrvScan,
	0, NULL, NULL, NULL,
	&CaveRecalcPalette, 384, 240, 4, 3
};

struct BurnDriver BurnDrvnmaster = {
	"nmaster", "metmqstr", NULL, "1995",
	"Oni - The Ninja Master (Japan)\0", NULL, "Banpresto/Pandorabox", "Cave",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_16BIT_ONLY | BDF_CLONE, 2, HARDWARE_CAVE_68K_Z80,
	NULL, nmasterRomInfo, nmasterRomName, metmqstrInputInfo, NULL,
	DrvInit, DrvExit, DrvFrame, DrvDraw, DrvScan,
	0, NULL, NULL, NULL,
	&CaveRecalcPalette, 384, 240, 4, 3
};
