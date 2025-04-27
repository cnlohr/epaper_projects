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

static const char * palette[10] = {
	"1111111111111111", // Black
	"2222222222222222", // White
	"1212121212122222", // Yellow
	"1112121212121212", // Green
	"1211111112111111", // Red
	"2120222221202222", // Navy Blue
	"1122222211222222", // Greenish Blue
	"2211112122111121", // Light Purple
	"1212211212122112", // Orange
	"1221121212211212", // Brown
};
static const char pstopat[10] = {
	0, 0, 1, 1, 1, 1, 1, 1, 1, 0
};

static int palettepaused[10] = { 0 };


int main()
{
	SystemInit();

	// Enable GPIOs
	funGpioInitAll();

	//EPD_7IN3F_Show7Block();

    unsigned long i, k;

	// [--][--][--]
	// |0123
    // |4567
    // |89ab
    // |cdef

	int frame = 0;

	SetupEPaperDisplay();
//while(1);
	while( 1 )
	{
_rand_lfsr = 5;
		SetupEPaperDisplayTrunc();

#if 1
	SendCommand(0x20);// An interesting value!
	SendData((frame &1)?0xff:0x41);
	SendData((frame &1)?0xff:0x41);
	SendData((frame &1)?0xff:0x41);
	SendData((frame &1)?0xff:0x41);
	SendData((frame &1)?0xff:0x41);
	SendData((frame &1)?0xff:0x41);
	SendData(0x00);
	SendData(0x00);
#endif
		SendCommand(0x04); // "PON"

		int cp;
		SetupEPaperForData();
		int sframe = frame & 0x3f;


		while( SPI1->STATR & 0x80 );
		GPIOOn( WVS_DC );
		GPIOOff( WVS_CS );

#ifdef LEARNING_PALETTE
		for(i=0; i<480; i++) {
		    for(k = 0 ; k < 400; k ++) {

				int ix = (i >> 6)&0x3;
				int iy = (k >> 5)&0x3;

				if( (frame & 3) == 1 )
				{
					cp = ix;
				}
				else
				{
					cp = iy;
				}

				//int cp = (k & 0x2) | (( i & 0x3 )<<2);
		        //SendData( cp | (cp+1) );
				//int cp = ((k>>6)&3) | (((i>>7)&3)<<2);
				//cp = (cp + ((frame) & 7)) & 0xf;
// 0 Does nothing?
// 1, 5 seems to "lean" dark?
// 2, 3, 4, 6 tend toward white
// 7 does nothing.

//				if(frame&1)
//					cp = ( (cp>>0) & 1 ) ?  5 : 6;
//				else
//					cp = ( (cp>>2) & 1 ) ?  1 : 3;

	if( i >100 && i < 200 && k > 100 && k < 150 )
				cp = "2122121212121212"[(frame&0xf)];
	else
		cp = 0;
//				cp = 1;

				const static char ccs[8] = {
					5, 2, 2, 2, 5, 7, 7, 7 };
				const static char ccs2[8] = {
					5, 4, 4, 4, 1, 7, 7, 7 };

				//cp = (frame&1)?6:5;
//				cp = (i<240)?ccs[frame&7]:ccs2[frame&7];	
//				cp = 4;
#if 0
if( i > 240)
				cp =
					( k>200)?
					 ((frame & 1)?0x1:0x2)
                :
					 ((frame & 1)?0x2:0x1);
#endif
				while( SPI1->STATR & 0x80 ) { }
				Delay_Us(1);

				// I do not know why this needs to be here and in the inner transfer
				MiniTransfer( cp | (cp<<4) );
				//SendData( (rand()&1)?0x55:0x44 );


		    }
		}
#else

		for(i=0; i<480; i++) {
		    for(k = 0 ; k < 400/80; k ++) {
				int acol = (i>240)?1:0;
				int arow = k;//(k/80);
				int pcol = arow + (acol?5:0);

				cp = palette[pcol][frame&0xf];
				cp &= palettepaused[pcol];

				while( SPI1->STATR & 0x80 ) { }
				// I do not know why this needs to be here and in the inner transfer
				int sp = 0;
				for( sp = 0; sp < 80; sp++ )
				{
					MiniTransfer( cp | (cp<<4) );
				}
				//SendData( (rand()&1)?0x55:0x44 );
			}
		}
#endif
		while( SPI1->STATR & 0x80 );

		//FlushAndDisplayEPaper();
		GPIOOn( WVS_CS );

		SendCommand(0x12); // "DRF"
		SendData(0x00);//previously nothing
		Delay_Us(90000);
		GPIOOff( WVS_RESET );


		int pcheck;
		for( pcheck = 0; pcheck < 10; pcheck++ )
		{
			if( sframe == 0 ) palettepaused[pcheck] = 0xf;
			if( sframe >= 0x30 && (pstopat[pcheck] == (sframe&0xf)) )
				palettepaused[pcheck] = 0;

		}
//while(1);
		frame++;
//		printf( "%d  ", frame & 0x7f );
//		int kk;
//		for( kk = 0; kk < 10; kk++ ) printf( "%d", palettepaused[kk] );
//		printf( "\n" );
//while(1);
	}

	while(1)
	{
	}
}
