#ifndef _ePaperColor_H
#define _ePaperColor_H

#include "ch32fun.h"

//ORIGINALLY: ePaper Module from Waveshare, it's this one: https://www.waveshare.com/wiki/5.65inch_e-Paper_Module_(F)
//NOW: https://www.waveshare.com/wiki/7.3inch_e-Paper_HAT_(F)_Manual#Resources "7in3f.c"


// Display resolution
#define EPD_WIDTH       800
#define EPD_HEIGHT      480

/**********************************
Color Index
**********************************/
#define EPD_7IN3F_BLACK   0x0	/// 000
#define EPD_7IN3F_WHITE   0x1	///	001
#define EPD_7IN3F_GREEN   0x2	///	010
#define EPD_7IN3F_BLUE    0x3	///	011
#define EPD_7IN3F_RED     0x4	///	100
#define EPD_7IN3F_YELLOW  0x5	///	101
#define EPD_7IN3F_ORANGE  0x6	///	110
#define EPD_7IN3F_CLEAN   0x7	///	111   unavailable  Afterimage

void SetupEPaperForData();
void SendEPaperData( uint8_t * data, int len );
void FlushAndDisplayEPaper();

void ClearEpaper(uint8_t color);

void EPD_7IN3F_Show7Block(void);


#define GPIOOff( x )  { funDigitalWrite( x, 0 ); }
#define GPIOOn( x )   { funDigitalWrite( x, 1 ); }
#define GPIORead( x ) ( funDigitalRead( x ) )

#define Delay() Delay_Us(2);
// { asm volatile( "nop" ); }

static void EPD_7IN3F_BusyHigh( uint32_t timeout )// If BUSYN=0 then waiting
{
    while(!(GPIORead(WVS_BUSY)))
	{
		if( timeout-- == 0 ) break;
		Delay_Ms(1);
	}
}

static void EPD_7IN3F_BusyLow( uint32_t timeout )// If BUSYN=1 then waiting
{
    while(GPIORead(WVS_BUSY))
	{
		if( timeout-- == 0 ) break;
		Delay_Ms(1);
	}
}

static void MiniTransfer( uint8_t data )
{
	while( SPI1->STATR & 0x80 );
	SPI1->DATAR = data;
	return;
}

static void SpiTransfer( uint8_t data )
{
	GPIOOff( WVS_CS );  Delay();
	while( SPI1->STATR & 0x80 ) { }
	SPI1->DATAR = data;
	while( SPI1->STATR & 0x80 ) { }

	GPIOOn( WVS_CS );
	return;
}

static void SendCommand( uint8_t command  )
{
    GPIOOff( WVS_DC );
	Delay();
    SpiTransfer( command );
}

static void SendData( uint8_t data  )
{
    GPIOOn( WVS_DC );
	Delay();
    SpiTransfer( data );
}

void SetupEPaperForData()
{
#if 1
    SendCommand(0x61);//Set Resolution setting
	SendData(0x03);
	SendData(0x20);
	SendData(0x01); 
	SendData(0xE0);
    SendCommand(0x10);
#endif
}

void SendEPaperData( uint8_t * data, int len )
{
	int i;
	for( i = 0; i < len; i++ )
		SendData( data[i] );
}

void FlushAndDisplayEPaper()
{
    SendCommand(0x04); // "PON"
    EPD_7IN3F_BusyHigh(150000);
    SendCommand(0x12); // "DRF"
    SendData(0x00);//previously nothing
    EPD_7IN3F_BusyHigh(150000);
    SendCommand(0x02);  // "POF"
    SendData(0x00);//previously nothing
    EPD_7IN3F_BusyHigh(150000);
	Delay_Us(20000);
}


void ClearEpaper(uint8_t color)
{
	SetupEPaperForData();
	uint8_t cv = (color<<4)|color;
    for(int i=0; i<EPD_WIDTH/2; i++) {
        for(int j=0; j<EPD_HEIGHT; j++)
            SendData(cv);
    }
	FlushAndDisplayEPaper();
}


void EPD_7IN3F_Show7Block(void)
{
    unsigned long i,j,k;
    unsigned char const Color_seven[8] =
	{
		EPD_7IN3F_BLACK,EPD_7IN3F_BLUE,EPD_7IN3F_GREEN,EPD_7IN3F_ORANGE,
		EPD_7IN3F_RED,EPD_7IN3F_YELLOW,EPD_7IN3F_WHITE,7
	};
	SetupEPaperForData();

	float color7 = 0;

    for(i=0; i<240-color7; i++) {
        for(k = 0 ; k < 4; k ++) {
            for(j = 0 ; j < 100; j ++) {
                SendData((Color_seven[k]<<4) |Color_seven[k]);
            }
        }
    }

    for(i=0; i<color7*2; i++) {
        for(k = 0 ; k < 4; k ++) {
            for(j = 0 ; j < 100; j ++) {
                SendData((Color_seven[7]<<4) |Color_seven[7]);
            }
        }
	}

    for(i=0; i<240-color7; i++) {
        for(k = 4 ; k < 8; k ++) {
            for(j = 0 ; j < 100; j ++) {
                SendData((Color_seven[k]<<4) |Color_seven[k]);
            }
        }
    }
	FlushAndDisplayEPaper();
}




static void Clear(uint8_t color)
{
	SetupEPaperForData();
    for(int i=0; i<EPD_WIDTH/2; i++) {
        for(int j=0; j<EPD_HEIGHT; j++)
            SendData((color<<4)|color);
    }
	FlushAndDisplayEPaper();
}

void SetupEPaperDisplayTrunc()
{

	GPIOOn( WVS_CS );
	GPIOOn( WVS_BUSY );
	GPIOOff( WVS_CLK );
	GPIOOff( WVS_DIN );
	GPIOOff( WVS_RESET );

	funPinMode( WVS_DIN,  GPIO_CFGLR_OUT_50Mhz_AF_PP );
	funPinMode( WVS_CLK,  GPIO_CFGLR_OUT_50Mhz_AF_PP );
	funPinMode( WVS_CS,   GPIO_CFGLR_OUT_30Mhz_PP );
	funPinMode( WVS_DC,   GPIO_CFGLR_OUT_30Mhz_PP );
	funPinMode( WVS_RESET,GPIO_CFGLR_OUT_30Mhz_PP );
	funPinMode( WVS_BUSY, GPIO_CFGLR_IN_PUPD );


	// Enable SPI + Peripherals
	RCC->APB2PCENR |= RCC_APB2Periph_SPI1;

	// Configure SPI 
	SPI1->CTLR1 = 
		SPI_NSS_Soft | SPI_CPHA_1Edge | SPI_CPOL_Low | SPI_DataSize_8b |
		SPI_Mode_Master | SPI_Direction_1Line_Tx |
		SPI_BaudRatePrescaler_2;

	// enable SPI port
	SPI1->CTLR1 |= CTLR1_SPE_Set;

	// high speed
	SPI1->HSCR = 1;

	//Reset for 1ms
	Delay_Us( 20 );

	GPIOOn( WVS_RESET );

    EPD_7IN3F_BusyHigh( 30 );
#if 0
	// At end
    Delay_Us(50000);
    SendCommand(0x50);
    SendData(0x37);
    Delay_Us(50000);


	EPD_7IN3F_Reset();
	EPD_7IN3F_ReadBusyH();
	DEV_Delay_ms(30);
#endif

	SendCommand(0xAA);    // CMDH
	SendData(0x49);
	SendData(0x55);
	SendData(0x20);
	SendData(0x08);
	SendData(0x09);
	SendData(0x18);

	SendCommand(0x01);  // "PWR"
	SendData(0x3F);
	SendData(0x00);
	SendData(0x32);
	SendData(0x2A);
	SendData(0x0E);
	SendData(0x2A);

	// toying with this can make things more faded.
	// Originally 0x00, 0x5f, 0x69
	SendCommand(0x00); // "PSR"
	SendData(0x5f);
	SendData(0x69);

	{
		// Can't tell what this does.
		SendCommand(0x03); // "POFS"
		SendData(0x00);
		SendData(0x54);
		SendData(0x00);
		SendData(0x44); 

		// Can't tell what this does. (No visual difference)
		SendCommand(0x05); // "BTST1"
		SendData(0x40);
		SendData(0x1F);
		SendData(0x1F);
		SendData(0x2C);

		SendCommand(0x06); // "BTST2"
		SendData(0x6F);
		SendData(0x1F);
		SendData(0x1F);
		SendData(0x22);

		// Can't tell what this does. (No visual difference)
		SendCommand(0x08); // "BTST3"
		SendData(0x6F);
		SendData(0x1F);
		SendData(0x1F);
		SendData(0x22);
	}

	// based on testing, registers 2-18 do basically nothing.


	// 0x20 seems to have something to do with the intensity of what the sets should be?
	// 0x20 looks VERY PROMISING
	// 0x66, 0x74 doesn't seem to do anything

	// ???
	SendCommand(0x13);    // IPC
	SendData(0x00);
	SendData(0x04);

	// ???
	SendCommand(0x30);
	SendData(0x3C);

	// No changes?
	SendCommand(0x41);     // TSE
	SendData(0x00);

	// No change?
	SendCommand(0x50);
	SendData(0x3F);

	SendCommand(0x60); // "TCON"
	SendData(0x02);
	SendData(0x00);

	// This controls the "active area"
	SendCommand(0x61); // "TRES"
	SendData(0x03);
	SendData(0x20);
	SendData(0x01); 
	SendData(0xE0);

	// Some sort of offset.
	SendCommand(0x82);  // "VDCS"
	SendData(0x1E); 

	// ??? No effect?
	SendCommand(0x84); // "T_VDCS"
	SendData(0x00);

	// Radically changes pattern, but it looks like a memory mapping thing.
	// Probably not worth investigating.
	SendCommand(0x86);    // AGID
	SendData(0x00);

	// ?? No visual change
	SendCommand(0xE3); // "PWS"
	SendData(0x2F);

	// lsb of data = 1 seems to disable screen updates.
	SendCommand(0xE0);   // CCSET
	SendData(0x00); 

	// No change?
	SendCommand(0xE6);   // TSSET
	SendData(0x00);
}

void SetupEPaperDisplay()
{
	GPIOOn( WVS_CS );
	GPIOOn( WVS_BUSY );
	GPIOOff( WVS_CLK );
	GPIOOff( WVS_DIN );
	GPIOOff( WVS_RESET );

	funPinMode( WVS_DIN,  GPIO_CFGLR_OUT_50Mhz_AF_PP );
	funPinMode( WVS_CLK,  GPIO_CFGLR_OUT_50Mhz_AF_PP );
	funPinMode( WVS_CS,   GPIO_CFGLR_OUT_30Mhz_PP );
	funPinMode( WVS_DC,   GPIO_CFGLR_OUT_30Mhz_PP );
	funPinMode( WVS_RESET,GPIO_CFGLR_OUT_30Mhz_PP );
	funPinMode( WVS_BUSY, GPIO_CFGLR_IN_PUPD );


	// Enable SPI + Peripherals
	RCC->APB2PCENR |= RCC_APB2Periph_SPI1;

	// Configure SPI 
	SPI1->CTLR1 = 
		SPI_NSS_Soft | SPI_CPHA_1Edge | SPI_CPOL_Low | SPI_DataSize_8b |
		SPI_Mode_Master | SPI_Direction_1Line_Tx |
		SPI_BaudRatePrescaler_2;

	// enable SPI port
	SPI1->CTLR1 |= CTLR1_SPE_Set;

	// high speed
	SPI1->HSCR = 1;

	//Reset for 1ms
	Delay_Ms( 1 );

	GPIOOn( WVS_RESET );

	Delay_Ms( 30 );

    EPD_7IN3F_BusyHigh( 20 );
#if 0
	// At end
    Delay_Us(50000);
    SendCommand(0x50);
    SendData(0x37);
    Delay_Us(50000);


	EPD_7IN3F_Reset();
	EPD_7IN3F_ReadBusyH();
	DEV_Delay_ms(30);
#endif

	SendCommand(0xAA);    // CMDH
	SendData(0x49);
	SendData(0x55);
	SendData(0x20);
	SendData(0x08);
	SendData(0x09);
	SendData(0x18);

	SendCommand(0x01);  // "PWR"
	SendData(0x3F);
	SendData(0x00);
	SendData(0x32);
	SendData(0x2A);
	SendData(0x0E);
	SendData(0x2A);

	// toying with this can make things more faded.
	// Originally 0x00, 0x5f, 0x69
	SendCommand(0x00); // "PSR"
	SendData(0x5f);
	SendData(0x69);

	{
		// Can't tell what this does.
		SendCommand(0x03); // "POFS"
		SendData(0x00);
		SendData(0x54);
		SendData(0x00);
		SendData(0x44); 

		// Can't tell what this does. (No visual difference)
		SendCommand(0x05); // "BTST1"
		SendData(0x40);
		SendData(0x1F);
		SendData(0x1F);
		SendData(0x2C);

		SendCommand(0x06); // "BTST2"
		SendData(0x6F);
		SendData(0x1F);
		SendData(0x1F);
		SendData(0x22);

		// Can't tell what this does. (No visual difference)
		SendCommand(0x08); // "BTST3"
		SendData(0x6F);
		SendData(0x1F);
		SendData(0x1F);
		SendData(0x22);
	}

	// based on testing, registers 2-18 do basically nothing.


	// 0x20 seems to have something to do with the intensity of what the sets should be?
	// 0x20 looks VERY PROMISING
	// 0x66, 0x74 doesn't seem to do anything

	// ???
	SendCommand(0x13);    // IPC
	SendData(0x00);
	SendData(0x04);

	// ???
	SendCommand(0x30);
	SendData(0x3C);

	// No changes?
	SendCommand(0x41);     // TSE
	SendData(0x00);

	// No change?
	SendCommand(0x50);
	SendData(0x3F);

	SendCommand(0x60); // "TCON"
	SendData(0x02);
	SendData(0x00);

	// This controls the "active area"
	SendCommand(0x61); // "TRES"
	SendData(0x03);
	SendData(0x20);
	SendData(0x01); 
	SendData(0xE0);

	// Some sort of offset.
	SendCommand(0x82);  // "VDCS"
	SendData(0x1E); 

	// ??? No effect?
	SendCommand(0x84); // "T_VDCS"
	SendData(0x00);

	// Radically changes pattern, but it looks like a memory mapping thing.
	// Probably not worth investigating.
	SendCommand(0x86);    // AGID
	SendData(0x00);

	// ?? No visual change
	SendCommand(0xE3); // "PWS"
	SendData(0x2F);

	// lsb of data = 1 seems to disable screen updates.
	SendCommand(0xE0);   // CCSET
	SendData(0x00); 

	// No change?
	SendCommand(0xE6);   // TSSET
	SendData(0x00);

	// Messing around
	// 0x01 -> First byte needs to be something like 0x07 or no updates happen.
	//  No other visual changes seem to happen

}


#endif

