#include "epaper-cnlohr.h"

//For 5.65 reference this.  I am not sure where the original came from.
//https://github.com/waveshare/e-Paper/blob/master/RaspberryPi%26JetsonNano/c/lib/e-Paper/EPD_5in65f.c

//For the 7.5 reference, I used:
//https://github.com/waveshare/e-Paper/blob/master/RaspberryPi%26JetsonNano/c/lib/e-Paper/EPD_7in5.c

//For the 4.2 inch version, I used: https://www.waveshare.com/wiki/4.2inch_e-Paper_Module_(B)
//https://github.com/waveshare/e-Paper/blob/master/RaspberryPi%26JetsonNano/c/lib/e-Paper/EPD_4in2b_V2.c

#define height EPCN_HEIGHT
#define width  EPCN_WIDTHD

static void EPCN_BusyHigh( uint16_t timeout )// If BUSYN=0 then waiting
{
	EPCN_DELAY_MS(1);
	while(!EPCN_BUSY)
	{
		if( timeout-- == 0 ) break;
		EPCN_DELAY_MS(1);
	}
}

#if defined( SKU18295 )
static void EPCN_BusyLow( uint16_t timeout )// If BUSYN=1 then waiting
{
	EPCN_DELAY_MS(1);
	while(EPCN_BUSY)
	{
		if( timeout-- == 0 ) break;
		EPCN_DELAY_MS(1);
	}
}
#endif

static void EPCN_SPITransfer( uint8_t data )
{
	uint8_t bit;
	EPCN_CS0; EPCN_DELAY;
	EPCN_CLK0; EPCN_DELAY;
	for( bit = 8; bit; bit-- )
	{
		if( data & 0x80 ) { EPCN_DIN1 }
		else { EPCN_DIN0; }
		data<<=1;
		EPCN_DELAY;
		EPCN_CLK1;
		EPCN_DELAY;
		EPCN_CLK0;
	}
	EPCN_DELAY
	EPCN_CS1
}

static void EPCN_SendCommand( uint8_t command  )
{
	EPCN_DC0
	EPCN_DELAY
	EPCN_SPITransfer( command );
}

void EPCN_SendData( uint8_t data  )
{
	EPCN_DC1
	EPCN_DELAY
	EPCN_SPITransfer( data );
}

void EPCN_BeginData()
{
#ifdef SKU18295
	EPCN_SendCommand(0x61);//Set Resolution setting
	EPCN_SendData(0x02);
	EPCN_SendData(0x58);
	EPCN_SendData(0x01);
	EPCN_SendData(0xC0);
	EPCN_SendCommand(0x10);
#elif( defined ( SKU14144 ) || defined( SKU13379 ) )
	EPCN_SendCommand(0x10);
#endif
	//NOTE: SKU13379 seems to have a part 2 which does 0x13
}

#ifdef SKU13379
void EPCN_BeginData2()
{
	EPCN_SendCommand(0x13);
}
#endif


void EPCN_SendArray( uint8_t * data, int len )
{
	int i;
	for( i = 0; i < len; i++ )
		EPCN_SendData( data[i] );
}

void EPCN_FinishData()
{
#ifdef SKU18295
	EPCN_SendCommand(0x04);//0x04
	EPCN_BusyHigh(150);
	EPCN_SendCommand(0x12);//0x12
	EPCN_BusyHigh(15000);
	EPCN_SendCommand(0x02);  //0x02
	EPCN_BusyLow(150);
	EPCN_DELAY_MS(20);
#elif( defined ( SKU14144 ) || defined( SKU13379 ) )
    EPCN_SendCommand(0x12); // DISPLAY_REFRESH
    EPCN_DELAY_MS(100);
    EPCN_BusyHigh(35000);
#endif

}

void EPCN_TestPattern()
{
	int x, y;
#ifdef SKU13379
	EPCN_BeginData();
	for( y = 0; y < EPCN_HEIGHT; y++ )
	for( x = 0; x < EPCN_WIDTHD/8; x++ )
	{
		EPCN_SendData( (x>EPCN_WIDTHD/16)?0xff:0x00 );
	}
	EPCN_FinishData();
	EPCN_BeginData2();
	for( y = 0; y < EPCN_HEIGHT; y++ )
	for( x = 0; x < EPCN_WIDTHD/8; x++ )
	{
		EPCN_SendData( (y>EPCN_HEIGHT/2)?0xff:0x00 );
	}
	EPCN_FinishData();

#else
	EPCN_BeginData();
	for( y = 0; y < EPCN_HEIGHT; y++ )
	for( x = 0; x < EPCN_WIDTHD/2; x++ )
	{
		int color = (x * 4) / (EPCN_WIDTHD/2) + ( (y*4) / EPCN_HEIGHT ) * 4;
		color &= 0xf;
		EPCN_SendData( color | (color<<4) );
	}
	EPCN_FinishData();
#endif
}


void EPCN_Clear(uint8_t color)
{
#ifdef SKU13379
	EPCN_BeginData();
	uint8_t cs = (color & 1)?0xff:0x00;
    for(int i=0; i<EPCN_WIDTHD/8; i++) {
        for(int j=0; j<height; j++)
            EPCN_SendData(cs );
    }
	EPCN_FinishData();

	EPCN_BeginData2();
	cs = (color / 2)?0x00:0xff;
    for(int i=0; i<EPCN_WIDTHD/8; i++) {
        for(int j=0; j<height; j++)
            EPCN_SendData(cs );
    }
	EPCN_FinishData();
#else
	EPCN_BeginData();
    for(int i=0; i<width/2; i++) {
        for(int j=0; j<height; j++)
            EPCN_SendData((color<<4)|color);
    }
	EPCN_FinishData();
#endif
}


void EPCN_Setup()
{
	EPCN_GPIO_SETUP
	EPCN_DELAY_MS( 10 );
	EPCN_RESET1
	EPCN_DELAY_MS( 10 );

#ifdef SKU18295
    EPCN_BusyHigh( 20 );
    EPCN_SendCommand(0x00);
    EPCN_SendData(0xEF);
    EPCN_SendData(0x08);
    EPCN_SendCommand(0x01);
    EPCN_SendData(0x37);
    EPCN_SendData(0x00);
    EPCN_SendData(0x23);
    EPCN_SendData(0x23);
    EPCN_SendCommand(0x03);
    EPCN_SendData(0x00);
    EPCN_SendCommand(0x06);
    EPCN_SendData(0xC7);
    EPCN_SendData(0xC7);
    EPCN_SendData(0x1D);
    EPCN_SendCommand(0x30);
    EPCN_SendData(0x3C);
    EPCN_SendCommand(0x40);
    EPCN_SendData(0x00);
    EPCN_SendCommand(0x50);
    EPCN_SendData(0x37);
    EPCN_SendCommand(0x60);
    EPCN_SendData(0x22);
    EPCN_SendCommand(0x61);
    EPCN_SendData(0x02);
    EPCN_SendData(0x58);
    EPCN_SendData(0x01);
    EPCN_SendData(0xC0);
    EPCN_SendCommand(0xE3);
    EPCN_SendData(0xAA);
	
    EPCN_DELAY_MS(50);
    EPCN_SendCommand(0x50);
    EPCN_SendData(0x37);
    EPCN_DELAY_MS(50);

#elif defined( SKU14144 )

	EPCN_SendCommand(0x01); // POWER_SETTING
	EPCN_SendData(0x37);
	EPCN_SendData(0x00);

	EPCN_SendCommand(0x00); // PANEL_SETTING
	EPCN_SendData(0xCF);
	EPCN_SendData(0x08);

	EPCN_SendCommand(0x06); // BOOSTER_SOFT_START
	EPCN_SendData(0xc7);
	EPCN_SendData(0xcc);
	EPCN_SendData(0x28);

	EPCN_SendCommand(0x04); // POWER_ON
	EPCN_BusyHigh(1000);

	EPCN_SendCommand(0x30); // PLL_CONTROL
	EPCN_SendData(0x3c);

	EPCN_SendCommand(0x41); // TEMPERATURE_CALIBRATION
	EPCN_SendData(0x00);

	EPCN_SendCommand(0x50); // VCOM_AND_DATA_INTERVAL_SETTING
	EPCN_SendData(0x77);

	EPCN_SendCommand(0x60); // TCON_SETTING
	EPCN_SendData(0x22);

	EPCN_SendCommand(0x61); // TCON_RESOLUTION
	EPCN_SendData(EPCN_WIDTH >> 8); // source 640
	EPCN_SendData(EPCN_WIDTH & 0xff);
	EPCN_SendData(EPCN_HEIGHT >> 8); // gate 384
	EPCN_SendData(EPCN_HEIGHT & 0xff);

	EPCN_SendCommand(0x82); // VCM_DC_SETTING
	EPCN_SendData(0x1E); // decide by LUT file

	EPCN_SendCommand(0xe5); // FLASH MODE
	EPCN_SendData(0x03);

#elif defined( SKU13379 )
    EPCN_SendCommand(0x04); 
    EPCN_BusyHigh(500);
    EPCN_SendCommand(0x00);
    EPCN_SendData(0x0f);
#endif

}


void EPCN_Sleep()
{
#if defined( SKU18295 )
    EPCN_DELAY_MS(100);
    EPCN_SendCommand(0x07);
    EPCN_SendData(0xA5);
    EPCN_DELAY_MS(100);
	EPCN_RESET0
#elif defined( SKU14144 )
    EPCN_SendCommand(0x02); // POWER_OFF
    EPCN_BusyHigh( 500 );
    EPCN_SendCommand(0x07); // DEEP_SLEEP
    EPCN_SendData(0XA5);;
#elif defined( SKU13379 )
    EPCN_SendCommand(0X50);
    EPCN_SendData(0xf7);		//border floating	
    EPCN_SendCommand(0X02);  	//power off
    EPCN_BusyHigh( 500 ); //waiting for the electronic paper IC to release the idle signal
    EPCN_SendCommand(0X07);  	//deep sleep
    EPCN_SendData(0xA5);
#endif
}
