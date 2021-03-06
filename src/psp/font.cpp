
#include <string.h>
#include "font.h"
#include "psp.h"

// Tahoma Font 12px Bold

static unsigned short font12[] = {
	0x0000,0x0000,0x0000,0x0000,		/*   */
	0x0000,0x7EC0,0x7EC0,0x0000,0x0000,		/* ! */
	0xE000,0xE000,0x0000,0xE000,0xE000,0x0000,		/* " */
	0x0300,0x1BC0,0x1F00,0x7B00,0x1BC0,0x1F00,0x7B00,0x1800,0x0000,		/* # */
	0x1880,0x3C40,0x2640,0xFFF0,0x2640,0x23C0,0x1180,0x0000,		/* $ */
	0x3800,0x7C00,0x4400,0x7C40,0x3880,0x0300,0x0400,0x1800,0x2380,0x47C0,0x0440,0x07C0,0x0380,0x0000,		/* % */
	0x3380,0x7FC0,0x4C40,0x4E40,0x7BC0,0x3180,0x07C0,0x0640,0x0000,		/* & */
	0xE000,0xE000,0x0000,		/* ' */
	0x1F80,0x7FE0,0xE070,0x8010,0x0000,		/* ( */
	0x8010,0xE070,0x7FE0,0x1F80,0x0000,		/* ) */
	0x0000,0x5000,0x2000,0xF800,0xF800,0x2000,0x5000,0x0000,		/* * */
	0x0000,0x0400,0x0400,0x0400,0x3F80,0x0400,0x0400,0x0400,0x0000,0x0000,		/* + */
	0x0000,0x00F0,0x00E0,0x0000,		/* , */
	0x0400,0x0400,0x0400,0x0400,0x0000,		/* - */
	0x0000,0x00C0,0x00C0,0x0000,		/* . */
	0x0030,0x01C0,0x0600,0x3800,0xC000,0x0000,0x0000,		/* / */
	0x3F80,0x7FC0,0x4040,0x4040,0x4040,0x7FC0,0x3F80,0x0000,		/* 0 */
	0x0000,0x2040,0x2040,0x7FC0,0x7FC0,0x0040,0x0040,0x0000,		/* 1 */
	0x2040,0x40C0,0x41C0,0x4340,0x4640,0x7C40,0x3840,0x0000,		/* 2 */
	0x2080,0x4040,0x4440,0x4440,0x4440,0x7FC0,0x3B80,0x0000,		/* 3 */
	0x0600,0x0A00,0x1200,0x2200,0x7FC0,0x7FC0,0x0200,0x0000,		/* 4 */
	0x0080,0x7840,0x7840,0x4840,0x4840,0x4FC0,0x4780,0x0000,		/* 5 */
	0x1F80,0x3FC0,0x6840,0x4840,0x4840,0x4FC0,0x0780,0x0000,		/* 6 */
	0x4000,0x4000,0x41C0,0x47C0,0x5E00,0x7800,0x6000,0x0000,		/* 7 */
	0x3B80,0x7FC0,0x4440,0x4440,0x4440,0x7FC0,0x3B80,0x0000,		/* 8 */
	0x3C00,0x7E40,0x4240,0x4240,0x42C0,0x7F80,0x3F00,0x0000,		/* 9 */
	0x0000,0x18C0,0x18C0,0x0000,		/* : */
	0x0000,0x18F0,0x18E0,0x0000,		/* ; */
	0x0000,0x0600,0x0600,0x0900,0x0900,0x1080,0x1080,0x2040,0x2040,0x0000,		/* < */
	0x0000,0x0900,0x0900,0x0900,0x0900,0x0900,0x0900,0x0900,0x0000,0x0000,		/* = */
	0x0000,0x2040,0x2040,0x1080,0x1080,0x0900,0x0900,0x0600,0x0600,0x0000,		/* > */
	0x2000,0x4000,0x46C0,0x4EC0,0x7800,0x3000,0x0000,		/* ? */
	0x1F80,0x2040,0x4F20,0x5FA0,0x50A0,0x5F20,0x5FA0,0x2080,0x1F00,0x0000,		/* @ */
	0x01C0,0x0FC0,0x3F00,0x7100,0x7100,0x3F00,0x0FC0,0x01C0,0x0000,		/* A */
	0x7FC0,0x7FC0,0x4440,0x4440,0x4440,0x7FC0,0x3B80,0x0000,		/* B */
	0x3F80,0x7FC0,0x4040,0x4040,0x4040,0x4040,0x3180,0x0000,		/* C */
	0x7FC0,0x7FC0,0x4040,0x4040,0x4040,0x60C0,0x3F80,0x1F00,0x0000,		/* D */
	0x7FC0,0x7FC0,0x4440,0x4440,0x4440,0x4440,0x0000,		/* E */
	0x7FC0,0x7FC0,0x4400,0x4400,0x4400,0x0000,		/* F */
	0x3F80,0x7FC0,0x4040,0x4040,0x4440,0x47C0,0x37C0,0x0000,		/* G */
	0x7FC0,0x7FC0,0x0400,0x0400,0x0400,0x0400,0x7FC0,0x7FC0,0x0000,		/* H */
	0x4040,0x7FC0,0x7FC0,0x4040,0x0000,		/* I */
	0x0040,0x4040,0x4040,0x7FC0,0x7F80,0x0000,		/* J */
	0x7FC0,0x7FC0,0x0E00,0x1B00,0x3180,0x60C0,0x4040,0x0000,		/* K */
	0x7FC0,0x7FC0,0x0040,0x0040,0x0040,0x0040,0x0000,		/* L */
	0x7FC0,0x7000,0x3800,0x1C00,0x0E00,0x0C00,0x1800,0x3000,0x7FC0,0x7FC0,0x0000,		/* M */
	0x7FC0,0x7000,0x3800,0x0E00,0x0780,0x01C0,0x7FC0,0x0000,		/* N */
	0x3F80,0x7FC0,0x4040,0x4040,0x4040,0x4040,0x7FC0,0x3F80,0x0000,		/* O */
	0x7FC0,0x7FC0,0x4200,0x4200,0x4200,0x7E00,0x3C00,0x0000,		/* P */
	0x3F80,0x7FC0,0x4040,0x4040,0x4060,0x4070,0x7FD0,0x3F90,0x0000,		/* Q */
	0x7FC0,0x7FC0,0x4400,0x4600,0x4700,0x7D80,0x38C0,0x0040,0x0000,		/* R */
	0x3980,0x7C40,0x4440,0x4440,0x4440,0x47C0,0x3380,0x0000,		/* S */
	0x4000,0x4000,0x7FC0,0x7FC0,0x4000,0x4000,0x0000,		/* T */
	0x7F80,0x7FC0,0x0040,0x0040,0x0040,0x7FC0,0x7F80,0x0000,		/* U */
	0x7000,0x7E00,0x0FC0,0x01C0,0x0FC0,0x7E00,0x7000,0x0000,		/* V */
	0x7800,0x7F00,0x07C0,0x07C0,0x3F00,0x7000,0x3F00,0x07C0,0x07C0,0x7F00,0x7800,0x0000,		/* W */
	0x60C0,0x71C0,0x1F00,0x0E00,0x1F00,0x71C0,0x60C0,0x0000,		/* X */
	0x7000,0x7C00,0x0FC0,0x0FC0,0x7C00,0x7000,0x0000,		/* Y */
	0x41C0,0x43C0,0x4640,0x4C40,0x7840,0x7040,0x0000,		/* Z */
	0xFFF0,0xFFF0,0x8010,0x8010,0x0000,		/* [ */
	0xC000,0x3800,0x0600,0x01C0,0x0030,0x0000,0x0000,		/* \ */
	0x8010,0x8010,0xFFF0,0xFFF0,0x0000,		/* ] */
	0x0000,0x0800,0x1000,0x2000,0x4000,0x4000,0x2000,0x1000,0x0800,0x0000,		/* ^ */
	0x0010,0x0010,0x0010,0x0010,0x0010,0x0010,0x0010,0x0010,		/* _ */
	0x0000,0x0000,0x8000,0xC000,0x4000,0x0000,0x0000,		/* ` */
	0x0180,0x0BC0,0x1240,0x1240,0x1FC0,0x0FC0,0x0000,		/* a */
	0xFFC0,0xFFC0,0x0840,0x1040,0x1040,0x1FC0,0x0F80,0x0000,		/* b */
	0x0F80,0x1FC0,0x1040,0x1040,0x1040,0x0000,		/* c */
	0x0F80,0x1FC0,0x1040,0x1040,0x1080,0xFFC0,0xFFC0,0x0000,		/* d */
	0x0F80,0x1FC0,0x1240,0x1240,0x1E40,0x0E80,0x0000,		/* e */
	0x1000,0x7FC0,0xFFC0,0x9000,0x8000,		/* f */
	0x0F80,0x1FD0,0x1050,0x1050,0x1090,0x1FF0,0x1FE0,0x0000,		/* g */
	0xFFC0,0xFFC0,0x0800,0x1000,0x1000,0x1FC0,0x0FC0,0x0000,		/* h */
	0x5FC0,0x5FC0,0x0000,		/* i */
	0x1010,0x5FF0,0x5FE0,0x0000,		/* j */
	0xFFC0,0xFFC0,0x0700,0x0D80,0x18C0,0x1040,0x0000,		/* k */
	0xFFC0,0xFFC0,0x0000,		/* l */
	0x1FC0,0x1FC0,0x1000,0x1000,0x1FC0,0x0FC0,0x1000,0x1000,0x1FC0,0x0FC0,0x0000,		/* m */
	0x1FC0,0x1FC0,0x0800,0x1000,0x1000,0x1FC0,0x0FC0,0x0000,		/* n */
	0x0F80,0x1FC0,0x1040,0x1040,0x1040,0x1FC0,0x0F80,0x0000,		/* o */
	0x1FF0,0x1FF0,0x0840,0x1040,0x1040,0x1FC0,0x0F80,0x0000,		/* p */
	0x0F80,0x1FC0,0x1040,0x1040,0x1080,0x1FF0,0x1FF0,0x0000,		/* q */
	0x1FC0,0x1FC0,0x0800,0x1800,0x0000,		/* r */
	0x0C80,0x1E40,0x1240,0x13C0,0x0980,0x0000,		/* s */
	0x1000,0x7F80,0x7FC0,0x1040,0x1040,0x0000,		/* t */
	0x1F80,0x1FC0,0x0040,0x0040,0x0080,0x1FC0,0x1FC0,0x0000,		/* u */
	0x1C00,0x1F00,0x03C0,0x03C0,0x1F00,0x1C00,0x0000,		/* v */
	0x1800,0x1F00,0x07C0,0x01C0,0x1F00,0x1F00,0x01C0,0x07C0,0x1F00,0x1800,0x0000,		/* w */
	0x18C0,0x1DC0,0x0700,0x0700,0x1DC0,0x18C0,0x0000,		/* x */
	0x1C00,0x1F30,0x03F0,0x03C0,0x1F00,0x1C00,0x0000,		/* y */
	0x11C0,0x13C0,0x1640,0x1C40,0x1840,0x0000,		/* z */
	0x0400,0x0400,0x7FE0,0xFBF0,0x8010,0x8010,0x0000,		/* { */
	0x0000,0x0000,0xFFF0,0xFFF0,0x0000,0x0000,0x0000,		/* | */
	0x8010,0x8010,0xFBF0,0x7FE0,0x0400,0x0400,0x0000,		/* } */
	0x0700,0x0C00,0x0C00,0x0C00,0x0600,0x0300,0x0300,0x0300,0x0E00,0x0000,		/* ~ */
};

static struct font12inf_t {
	unsigned short offset;
	unsigned short width;
} font12inf[] = {
    {0,4}, {4,5}, {9,6}, {15,9}, {24,8}, {32,14}, {46,9}, {55,3}, {58,5}, {63,5},
    {68,8}, {76,10}, {86,4}, {90,5}, {95,4}, {99,7}, {106,8}, {114,8}, {122,8},
    {130,8}, {138,8}, {146,8}, {154,8}, {162,8}, {170,8}, {178,8}, {186,4},
    {190,4}, {194,10}, {204,10}, {214,10}, {224,7}, {231,10}, {241,9}, {250,8},
    {258,8}, {266,9}, {275,7}, {282,6}, {288,8}, {296,9}, {305,5}, {310,6},
    {316,8}, {324,7}, {331,11}, {342,8}, {350,9}, {359,8}, {367,9}, {376,9},
    {385,8}, {393,7}, {400,8}, {408,8}, {416,12}, {428,8}, {436,7}, {443,7},
    {450,5}, {455,7}, {462,5}, {467,10}, {477,8}, {485,7}, {492,7}, {499,8},
    {507,6}, {513,8}, {521,7}, {528,5}, {533,8}, {541,8}, {549,3}, {552,4},
    {556,7}, {563,3}, {566,11}, {577,8}, {585,8}, {593,8}, {601,8}, {609,5},
    {614,6}, {620,6}, {626,8}, {634,7}, {641,11}, {652,7}, {659,7}, {666,6},
    {672,7}, {679,7}, {686,7}, {693,10},
};


static int printChar(unsigned short *screenbuf, unsigned char c, unsigned short cl, int mw)
{
	//if (c > 0x7e || c < 0x20) return font12inf[0x1f].width;	// "?"
	if (c > 0x7e || c < 0x20) c = '?';
	
	int w = font12inf[c - 0x20].width;
    if (w > mw) w = mw;

	unsigned short * pz = &font12[font12inf[c-0x20].offset];
	for (int i=0; i<w; i++, pz++, screenbuf++) {
		if (*pz & 0x8000) {screenbuf[ 512 *  0 ] = cl; screenbuf[ 512 *  1 +1 ]=0;}
		if (*pz & 0x4000) {screenbuf[ 512 *  1 ] = cl; screenbuf[ 512 *  2 +1 ]=0;}
		if (*pz & 0x2000) {screenbuf[ 512 *  2 ] = cl; screenbuf[ 512 *  3 +1 ]=0;}
		if (*pz & 0x1000) {screenbuf[ 512 *  3 ] = cl; screenbuf[ 512 *  4 +1 ]=0;}

		if (*pz & 0x0800) {screenbuf[ 512 *  4 ] = cl; screenbuf[ 512 *  5 +1 ]=0;}
		if (*pz & 0x0400) {screenbuf[ 512 *  5 ] = cl; screenbuf[ 512 *  6 +1 ]=0;}
		if (*pz & 0x0200) {screenbuf[ 512 *  6 ] = cl; screenbuf[ 512 *  7 +1 ]=0;}
		if (*pz & 0x0100) {screenbuf[ 512 *  7 ] = cl; screenbuf[ 512 *  8 +1 ]=0;}
		if (*pz & 0x0080) {screenbuf[ 512 *  8 ] = cl; screenbuf[ 512 *  9 +1 ]=0;}
		if (*pz & 0x0040) {screenbuf[ 512 *  9 ] = cl; screenbuf[ 512 *  10 +1 ]=0;}
		if (*pz & 0x0020) {screenbuf[ 512 * 10 ] = cl; screenbuf[ 512 *  11 +1 ]=0;}
		if (*pz & 0x0010) {screenbuf[ 512 * 11 ] = cl; screenbuf[ 512 *  12 +1 ]=0;}
	}
	return w;
}

void drawString(const char *s, unsigned short *screenbuf, int x, int y, unsigned short c, int w)
{
    if (!w) w = 512 - x;
	screenbuf += x + y * 512;
	int len = strlen(s);
    for (int i = 0; i < len; i++) {
        int tw = printChar(screenbuf, s[i], c, w);
        screenbuf += tw;
        w -= tw;
        if (w <= 0) break;
    }
}

int getDrawStringLength(const char *s)
{
    int res = 0;
    for (; *s!=0; s++) {
        if ((unsigned char)(*s) > 0x7e || (unsigned char)(*s) < 0x20)
            res += font12inf[0x1f].width;	// "?"
        else
            res += font12inf[*s - 0x20].width;
    }
    return res;
}


void drawRect(unsigned short *screenbuf, int x, int y, int w, int h, unsigned short c, unsigned char alpha)
{
	unsigned char r,g,b;
	screenbuf += x + y * 512;
	for ( ;h>0; h--, screenbuf+=512) {
		unsigned short * p = screenbuf;
		for(int i=0; i<w; i++,p++)
		{
			 r = ((*p & 0x1f)*(255-alpha)+(c & 0x1f)*alpha)/255; 
             g = (((*p >> 5) & 0x3f)*(255-alpha)+((c >> 5) & 0x3f)*alpha)/255;
             b = (((*p  >> 11) & 0x1f)*(255-alpha)+((c >> 11) & 0x1f)*alpha)/255;
             *p = (r& 0x1f)|((g&0x3f)<<5)|((b&0x1f)<<11);
		} 
	}
}

void drawImage(unsigned short *screenbuf, int x, int y, int w,int h, unsigned short *imgBuf, int imgW, int imgH)
{
	int i,j;
	screenbuf += x + y * 512;
	if(w==imgW&&h==imgH)
	{
		for (j=0 ;j<h; j++, screenbuf+=512,imgBuf+=512 ) 
		{
			for(i=0; i<w; i++) screenbuf[i] = imgBuf[i];
		}
	}else
	{
		float wRatio= ((float)imgW)/w;
		float hRatio= ((float)imgH)/h;
		
		for (j=0 ;j<h; j++, screenbuf+=512) {
			unsigned short * p = screenbuf;
			for(i=0; i<w; i++,p++) *p =
			*(imgBuf+(int)(wRatio*i+0.5)+(int)(hRatio*j+0.5)*PSP_LINE_SIZE);
		}
	}
}

