#include "tiles_generic.h"
#include "ics2115.h"

// pgm_run 
extern unsigned char PgmJoy1[];
extern unsigned char PgmJoy2[];
extern unsigned char PgmJoy3[];
extern unsigned char PgmJoy4[];
extern unsigned char PgmBtn1[];
extern unsigned char PgmBtn2[];
extern unsigned char PgmInput[];
extern unsigned char PgmReset;

int pgmInit();
int pgmExit();
int pgmFrame();
int pgmDraw();
int pgmScan(int nAction, int *pnMin);

extern int nPGM68KROMLen;
extern int nPGMSPRColMaskLen;
extern int nPGMSPRMaskMaskLen;

extern unsigned char *USER0, *USER1, *USER2;
extern unsigned char *PGM68KROM, *PGMTileROM, *PGMTileROMExp, *PGMSPRColROM, *PGMSPRMaskROM, *PGMARMROM;
extern unsigned char *PGMARMRAM0, *PGMARMRAM1, *PGMARMRAM2, *PGMARMShareRAM;
extern unsigned short *RamRs, *RamPal, *RamVReg, *RamSpr, *RamSprBuf;
extern unsigned int *RamBg, *RamTx, *RamCurPal;
extern unsigned char nPgmPalRecalc;

extern void (*pPgmInitCallback)();
extern void (*pPgmResetCallback)();
extern int (*pPgmScanCallback)(int, int*);

void pgm_cpu_sync();

// pgm_draw
void pgmInitDraw();
void pgmExitDraw();
int pgmDraw();

// pgm_prot
void install_asic28_protection();
void install_pstars_protection();
void install_dw2_protection();
void install_killbldt_protection();
void install_asic3_protection();
void install_asic27A_protection();

void pstars_reset();
void killbldt_reset();
void asic28_reset();
void asic3_reset();

// pgm_crypt
void pgm_kov_decrypt();
void pgm_kovsh_decrypt();
void pgm_dw2_decrypt();
void pgm_djlzz_decrypt();
void pgm_pstar_decrypt();
void pgm_dw3_decrypt();
void pgm_killbld_decrypt();
void pgm_dfront_decrypt();
void pgm_ddp2_decrypt();
void pgm_mm_decrypt();
void pgm_kov2_decrypt();
void pgm_puzzli2_decrypt();
void pgm_kov2p_decrypt();
void pgm_theglad_decrypt();

