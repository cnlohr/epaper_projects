#include "ch32fun.h"
#include <stdio.h>

#define WVS_DIN  PC6
#define WVS_CLK  PC5
#define WVS_CS   PC4
#define WVS_DC   PC3
#define WVS_RESET  PC2
#define WVS_BUSY PC1

#include "ePaperColor.h"

int main()
{
	SystemInit();

	// Enable GPIOs
	funGpioInitAll();

	SetupEPaperDisplay();
	//EPD_7IN3F_Show7Block();

    unsigned long i, k;
	SetupEPaperForData();

	// [--][--][--]
	// |0123
    // |4567
    // |89ab
    // |cdef

    for(i=0; i<480; i++) {
        for(k = 0 ; k < 400; k ++) {
			//int cp = (k & 0x2) | (( i & 0x3 )<<2);
            //SendData( cp | (cp+1) );
			int cp = ((k>>2)&3) | (((i>>3)&3)<<2);
			SendData( cp | (cp<<4) );
        }
    }
	FlushAndDisplayEPaper();

	while(1)
	{
	}
}
