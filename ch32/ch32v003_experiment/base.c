#include "ch32fun.h"
#include <stdio.h>

#define WVS_DIN  PC6
#define WVS_CLK  PC5
#define WVS_CS   PC4
#define WVS_DC   PC3
#define WVS_RESET  PC2
#define WVS_BUSY PC1

#include "ePaperColor.h"

#define RANDOM_STRENGTH 1
#include "lib_rand.h"



	// a=0xff b=0x41
	//              babababa
	// Best yellow: 1212121212121212
	//               ^
	//
	// Green?:      2212121212121221
	//              ^
	//
    //    To clear: Make dark.
	// Red:         babababa              
	//              1211111112111111
	//               ^
	// Navy Blue (Kind of redish)
	//              2120222221202222
    //               ^
	// Blue (kind of greenish):         
	//              1122222211222222
	//               ^
	// Grey:        2211112122111121
    //               ^
	// Orange:      1212211212122112
    //               ^
	// Brown:       1221121212211212
    //              ^
	//

static const char * palette[16] = {
	"5552555555555555", // Black
	"2221222222222222", // White
	"1212121212122222", // Yellow
	"1112121212121212", // Green
	"1211111112111111", // Red
	"2120222221202222", // Navy Blue
	"1122222211222222", // Greenish Blue
	"2211112122111121", // Light Purple
	"1212211212122112", // Orange
	"1221121212211212", // Brown
	"2222222222222121", // Cyan
	"1111111111111111", // Nul
	"1111111111111111", // Nul
	"1111111111111111", // Nul
	"1111111111111111", // Nul
	"1111111111111111", // Nul
};
static const char pstopat[16] = {
	0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0
};

static int palettepaused[16] = { 0 };


int main()
{
	SystemInit();

	// Enable GPIOs
	funGpioInitAll();

	//EPD_7IN3F_Show7Block();

    unsigned long i, k;

	int frame = 0;

	{
		_rand_lfsr = 6;
		SetupEPaperDisplayTrunc();

		SendCommand(0x04); // "PON"
		SetupEPaperForData();
		while( SPI1->STATR & 0x80 ) { }
		GPIOOn( WVS_CS );
		Delay();
		GPIOOn( WVS_DC );
		Delay();
		GPIOOff( WVS_CS );

		for(i=0; i<480*400; i++) {
			MiniTransfer( rand() );
		}

		while( SPI1->STATR & 0x80 ) { }

		//FlushAndDisplayEPaper();
		GPIOOn( WVS_CS );

	    EPD_7IN3F_BusyHigh( 2000 );

	    SendCommand(0x04); // "PON"
		SendCommand(0x12); // "DRF"
		SendData(0x01);

	    EPD_7IN3F_BusyHigh( 2000 );
		Delay_Ms(1000);
		GPIOOff( WVS_RESET );
	}

	while( 1 )
	{
		_rand_lfsr = 5+frame;

		int sframe = frame & 0x7f;
		SetupEPaperDisplayTrunc();

#if 0
	SendCommand(0x20);// An interesting value!
	SendData((frame &1)?0xff:0x11);
	SendData((frame &1)?0xff:0x11);
	SendData((frame &1)?0xff:0x11);
	SendData((frame &1)?0xff:0x11);
	SendData((frame &1)?0xff:0x11);
	SendData((frame &1)?0xff:0x11);
	SendData(0x00);
	SendData(0x00);
#endif

		int cp;

		SendCommand(0x04); // "PON"
		SetupEPaperForData();

		while( SPI1->STATR & 0x80 ) { }
		GPIOOn( WVS_CS );
		Delay();
		GPIOOn( WVS_DC );
		Delay();
		GPIOOff( WVS_CS );


		for(i=0; i<480; i++) {
			int palettecolumns = sizeof(palettepaused)/sizeof(palettepaused[0])/2;
		    for(k = 0 ; k < palettecolumns; k ++) {
				int acol = (i>240)?1:0;
				int arow = k;//(k/80);
				int pcol = arow + (acol?palettecolumns:0);

				cp = palette[pcol][frame&0xf];
				cp &= palettepaused[pcol];
				if( sframe < 0x10 ) cp = 5;
				else if( sframe < 0x20 ) cp = 4;

				// I do not know why this needs to be here and in the inner transfer
				int sp = 0;
				for( sp = 0; sp < 400/palettecolumns; sp++ )
				{
					MiniTransfer( cp | (cp<<4) );
					//MiniTransfer( rand() );
				}
				//SendData( (rand()&1)?0x55:0x44 );
			}
		}

		while( SPI1->STATR & 0x80 ) { }

		GPIOOn( WVS_CS );

/*

	    EPD_7IN3F_BusyHigh( 2000 );
	    SendCommand(0x04); // "PON"
		SendCommand(0x12); // "DRF"
		SendData(0x01);
	    EPD_7IN3F_BusyHigh( 50000 );
		Delay_Ms(1000);
		GPIOOff( WVS_RESET );
*/


	    EPD_7IN3F_BusyHigh( 2000 );

		SendCommand(0x12); // "DRF"
		SendData(0x01);

		Delay_Us(80000);

		GPIOOff( WVS_RESET );

		int pcheck;
		for( pcheck = 0; pcheck < sizeof(palettepaused)/sizeof(palettepaused[0]); pcheck++ )
		{
			if( sframe == 0 ) palettepaused[pcheck] = 0xf;
			if( sframe >= 0x40 && (pstopat[pcheck] == (sframe&0xf)) )
				palettepaused[pcheck] = 0;

		}

		frame++;
	}

	while(1)
	{
	}
}
