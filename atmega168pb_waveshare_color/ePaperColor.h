#ifndef _ePaperColor_H
#define _ePaperColor_H

#include "common.h"

//ePaper Module from Waveshare, it's this one: https://www.waveshare.com/wiki/5.65inch_e-Paper_Module_(F)

#define WVS_BUSY  1
#define WVS_RESET 2
#define WVS_DC    3
#define WVS_CS    4
#define WVS_CLK   5
#define WVS_DIN   6
#define WVS_PORT  PORTD
#define WVS_PIN   PIND
#define WVS_DDR   DDRD

// Display resolution
#define EPD_WIDTH       600
#define EPD_HEIGHT      448

/**********************************
Color Index
**********************************/
#define EPD_5IN65F_BLACK   0x0	/// 000
#define EPD_5IN65F_WHITE   0x1	///	001
#define EPD_5IN65F_GREEN   0x2	///	010
#define EPD_5IN65F_BLUE    0x3	///	011
#define EPD_5IN65F_RED     0x4	///	100
#define EPD_5IN65F_YELLOW  0x5	///	101
#define EPD_5IN65F_ORANGE  0x6	///	110
#define EPD_5IN65F_CLEAN   0x7	///	111   unavailable  Afterimage


void SetupEPaperDisplay();

void SetupEPaperForData();
void SendEPaperData( uint8_t * data, int len );
void FlushAndDisplayEPaper();

void ClearEpaper(uint8_t color);

void EPD_5IN65F_Show7Block(void);

#endif

