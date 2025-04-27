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
_rand_lfsr = 5+frame;
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
		SendCommand(0x04); // "PON"

		int cp;
		SetupEPaperForData();
		for(i=0; i<480; i++) {
		    for(k = 0 ; k < 400; k ++) {
				int ix = (i >> 6)&0x7;
				int iy = (k >> 5)&0x7;

				if( frame & 1 )
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

//				cp = (frame&1) ? 7 : 4;
//				cp = 5;

				const static char ccs[8] = {
					5, 2, 2, 5, 5, 7, 7, 7 };
cp = 5;
				//cp = (frame&1)?6:5;
				//cp = ccs[frame&7];				

				//cp = (frame & 1)?0x6:0x5;
//				SendData( cp | (cp<<4) );
//				SendData( (rand()&1)?0x55:0x44 );
				SendData( rand() );
		    }
		}
		FlushAndDisplayEPaper();
#if 0 // For glitching
//		EPD_7IN3F_BusyHigh(150000);
		SendCommand(0x12); // "DRF"
		SendData(0x00);//previously nothing
		Delay_Us(87000);
		GPIOOff( WVS_RESET );
#endif
		frame++;
		Delay_Ms(10000);
	}

	while(1)
	{
	}
}
