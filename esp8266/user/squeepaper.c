#include "squeepaper.h"

//For 5.65 reference this.  I am not sure where the original came from.
//https://github.com/waveshare/e-Paper/blob/master/RaspberryPi%26JetsonNano/c/lib/e-Paper/EPD_5in65f.c

//For the 7.5 reference, I used:
//https://github.com/waveshare/e-Paper/blob/master/RaspberryPi%26JetsonNano/c/lib/e-Paper/EPD_7in5.c

//For the 4.2 inch version, I used: https://www.waveshare.com/wiki/4.2inch_e-Paper_Module_(B)
//https://github.com/waveshare/e-Paper/blob/master/RaspberryPi%26JetsonNano/c/lib/e-Paper/EPD_4in2b_V2.c

#define height SQUEEPAPER_HEIGHT
#define width  SQUEEPAPER_WIDTHD

#ifndef ICACHE_FLASH_ATTR
#define ICACHE_FLASH_ATTR
#endif

#ifdef SQUEE_ASYNC
static uint8_t squee_buffer[SQUEE_ASYNC];
static int     squee_head;
static int     squee_tail;
static int     squee_elapsed_on_command;

#define SC_BUSYHI   0
#define SC_BUSYLO   1
#define SC_WAIT     2
#define SC_XFER_CMD 3
#define SC_XFER_DAT 4
#define SC_RESET_HI 5
#define SC_RESET_LO 6

//Return value of 0 is done.
//Return value of 1 is please wait.
//Return value of 2+ is processed command, you can keep going if you'd like.
int ICACHE_FLASH_ATTR SQUEEPAPER_Poll( int milliseconds )
{
	int ret = 0;
	int remain = (squee_head - squee_tail + SQUEE_ASYNC) % SQUEE_ASYNC;
	if( !remain ) return 0;
	char s = squee_buffer[squee_tail];
	char snext1 = squee_buffer[(squee_tail+1)%SQUEE_ASYNC];
	char snext2 = squee_buffer[(squee_tail+2)%SQUEE_ASYNC];
	char snext3 = squee_buffer[(squee_tail+3)%SQUEE_ASYNC];
	char snext4 = squee_buffer[(squee_tail+4)%SQUEE_ASYNC];
	char snext5 = squee_buffer[(squee_tail+5)%SQUEE_ASYNC];
	char snext6 = squee_buffer[(squee_tail+6)%SQUEE_ASYNC];
	int popoff = 0;

	if( s <= SC_WAIT && remain < 3 ) return 0;

//	printf( "%02x [%02x] [%02x] [%02x] [%02x] [%02x] [%02x] @ %d/%d\n", 
//		s, snext1, snext2, snext3, snext4, snext5, snext6, squee_tail, squee_head );

	switch( s )
	{
	case SC_BUSYHI: //Busy_high
	case SC_BUSYLO: //Busy_low
		//If the busy matches, we're good
		if( !!(SQUEEPAPER_BUSY) == s ) { popoff = 3; break; }
	case SC_WAIT:
		//Otherwise timeout.
		squee_elapsed_on_command += milliseconds; 
		uint16_t to_wait = snext1 | (snext2<<8);
		if( squee_elapsed_on_command >= to_wait )  { popoff = 3; break; }
		return 1; 
	case SC_XFER_CMD:
	case SC_XFER_DAT:
		if( remain < 2 ) return 0;
		{
			if( s == SC_XFER_CMD )
			{
				SQUEEPAPER_DC0
				SQUEEPAPER_DELAY
			}
			else
			{
				SQUEEPAPER_DC1
				SQUEEPAPER_DELAY
			}
			uint8_t bit;
			uint8_t data = snext1;
			SQUEEPAPER_CS0; SQUEEPAPER_DELAY;
			SQUEEPAPER_CLK0; SQUEEPAPER_DELAY;
			for( bit = 8; bit; bit-- )
			{
				if( data & 0x80 ) { SQUEEPAPER_DIN1 }
				else { SQUEEPAPER_DIN0; }
				data<<=1;
				SQUEEPAPER_DELAY;
				SQUEEPAPER_CLK1;
				SQUEEPAPER_DELAY;
				SQUEEPAPER_CLK0;
			}
			SQUEEPAPER_DELAY
			SQUEEPAPER_CS1
		}
		popoff = 2;
		ret = 2;
		break;
	case SC_RESET_HI: SQUEEPAPER_RESET1; popoff = 1; ret = 2; break;
	case SC_RESET_LO: SQUEEPAPER_RESET0; popoff = 1; ret = 2; break;
	}

	if( popoff )
	{
		squee_tail = ( squee_tail + popoff ) % SQUEE_ASYNC;
	}
	return ret;
}

int squee_free() { return (SQUEE_ASYNC - ((squee_head - squee_tail + SQUEE_ASYNC) % SQUEE_ASYNC)) - 1; }
void squee_tack( uint8_t ch ) { squee_buffer[squee_head] = ch; squee_head = ( squee_head + 1 ) % SQUEE_ASYNC; }

static void SQUEEPAPER_BusyHigh( uint16_t timeout )// If BUSYN=0 then waiting
{
	while( squee_free() < 3 ) { SQUEEPAPER_Poll(1); SQUEEPAPER_DELAY_MS(1); }
	squee_tack( SC_BUSYHI );
	squee_tack( timeout & 0xff );
	squee_tack( timeout >> 8 );
}

#if defined( SKU18295 )
static void SQUEEPAPER_BusyLow( uint16_t timeout )// If BUSYN=1 then waiting
{
	while( squee_free() < 3 ) { SQUEEPAPER_Poll(1); SQUEEPAPER_DELAY_MS(1); }
	squee_tack( SC_BUSYLO );
	squee_tack( timeout & 0xff );
	squee_tack( timeout >> 8 );
}
#endif

static void SQUEEPAPER_Wait( int timeout )
{
	while( squee_free() < 3 ) { printf( "SQUEEFREE: %d\n", squee_free() ); SQUEEPAPER_Poll(1); SQUEEPAPER_DELAY_MS(1); }
	squee_tack( SC_WAIT );
	squee_tack( timeout & 0xff );
	squee_tack( timeout >> 8 );
}

static void SQUEEPAPER_SendCommand( uint8_t command  )
{
	while( squee_free() < 2 ) { SQUEEPAPER_Poll(1); SQUEEPAPER_DELAY_MS(1); }
	squee_tack(	SC_XFER_CMD );
	squee_tack( command );
}

void SQUEEPAPER_SendData( uint8_t data  )
{
	while( squee_free() < 2 ) { SQUEEPAPER_Poll(1); SQUEEPAPER_DELAY_MS(1); }
	squee_tack(	SC_XFER_DAT );
	squee_tack( data );
}

void SQUEEPAPER_ResetHi()
{
	while( squee_free() < 1 ) { SQUEEPAPER_Poll(1); SQUEEPAPER_DELAY_MS(1); }
	squee_tack(	SC_RESET_HI );
}


void SQUEEPAPER_ResetLo()
{
	while( squee_free() < 1 ) { SQUEEPAPER_Poll(1); SQUEEPAPER_DELAY_MS(1); }
	squee_tack(	SC_RESET_LO );
}


#else

static void SQUEEPAPER_BusyHigh( uint16_t timeout )// If BUSYN=0 then waiting
{
	SQUEEPAPER_DELAY_MS(1);
	while(!SQUEEPAPER_BUSY)
	{
		if( timeout-- == 0 ) break;
		SQUEEPAPER_DELAY_MS(1);
	}
}

#if defined( SKU18295 )
static void SQUEEPAPER_BusyLow( uint16_t timeout )// If BUSYN=1 then waiting
{
	SQUEEPAPER_DELAY_MS(1);
	while(SQUEEPAPER_BUSY)
	{
		if( timeout-- == 0 ) break;
		SQUEEPAPER_DELAY_MS(1);
	}
}
#endif

static void SQUEEPAPER_SPITransfer( uint8_t data )
{
	uint8_t bit;
	SQUEEPAPER_CS0; SQUEEPAPER_DELAY;
	SQUEEPAPER_CLK0; SQUEEPAPER_DELAY;
	for( bit = 8; bit; bit-- )
	{
		if( data & 0x80 ) { SQUEEPAPER_DIN1 }
		else { SQUEEPAPER_DIN0; }
		data<<=1;
		SQUEEPAPER_DELAY;
		SQUEEPAPER_CLK1;
		SQUEEPAPER_DELAY;
		SQUEEPAPER_CLK0;
	}
	SQUEEPAPER_DELAY
	SQUEEPAPER_CS1
}

static void SQUEEPAPER_SendCommand( uint8_t command  )
{
	SQUEEPAPER_DC0
	SQUEEPAPER_DELAY
	SQUEEPAPER_SPITransfer( command );
}

void SQUEEPAPER_SendData( uint8_t data  )
{
	SQUEEPAPER_DC1
	SQUEEPAPER_DELAY
	SQUEEPAPER_SPITransfer( data );
}


#define SQUEEPAPER_ResetHi() {SQUEEPAPER_RESET1}
#define SQUEEPAPER_ResetLo() {SQUEEPAPER_RESET0}
#define SQUEEPAPER_Wait( x ) {SQUEEPAPER_DELAY_MS(x);}
#endif









void SQUEEPAPER_BeginData()
{
#ifdef SKU18295
	SQUEEPAPER_SendCommand(0x61);//Set Resolution setting
	SQUEEPAPER_SendData(0x02);
	SQUEEPAPER_SendData(0x58);
	SQUEEPAPER_SendData(0x01);
	SQUEEPAPER_SendData(0xC0);
	SQUEEPAPER_SendCommand(0x10);
#elif( defined ( SKU14144 ) || defined( SKU13379 ) )
	SQUEEPAPER_SendCommand(0x10);
#endif
	//NOTE: SKU13379 seems to have a part 2 which does 0x13
}

#ifdef SKU13379
void SQUEEPAPER_BeginData2()
{
	SQUEEPAPER_SendCommand(0x13);
}
#endif


void SQUEEPAPER_SendArray( uint8_t * data, int len )
{
	int i;
	for( i = 0; i < len; i++ )
		SQUEEPAPER_SendData( data[i] );
}

void SQUEEPAPER_FinishData()
{
#ifdef SKU18295
	SQUEEPAPER_SendCommand(0x04);//0x04
	SQUEEPAPER_BusyHigh(150);
	SQUEEPAPER_SendCommand(0x12);//0x12
	SQUEEPAPER_BusyHigh(15000);
	SQUEEPAPER_SendCommand(0x02);  //0x02
	SQUEEPAPER_BusyLow(150);
	SQUEEPAPER_Wait(20);
#elif( defined ( SKU14144 ) || defined( SKU13379 ) )
    SQUEEPAPER_SendCommand(0x12); // DISPLAY_REFRESH
    SQUEEPAPER_Wait(100);
    SQUEEPAPER_BusyHigh(35000);
#endif

}

void SQUEEPAPER_TestPattern()
{
	int x, y;
#ifdef SKU13379
	SQUEEPAPER_BeginData();
	for( y = 0; y < SQUEEPAPER_HEIGHT; y++ )
	for( x = 0; x < SQUEEPAPER_WIDTHD/8; x++ )
	{
		SQUEEPAPER_SendData( (x>SQUEEPAPER_WIDTHD/16)?0xff:0x00 );
	}
	SQUEEPAPER_FinishData();
	SQUEEPAPER_BeginData2();
	for( y = 0; y < SQUEEPAPER_HEIGHT; y++ )
	for( x = 0; x < SQUEEPAPER_WIDTHD/8; x++ )
	{
		SQUEEPAPER_SendData( (y>SQUEEPAPER_HEIGHT/2)?0xff:0x00 );
	}
	SQUEEPAPER_FinishData();

#else
	SQUEEPAPER_BeginData();
	for( y = 0; y < SQUEEPAPER_HEIGHT; y++ )
	for( x = 0; x < SQUEEPAPER_WIDTHD/2; x++ )
	{
		int color = (x * 4) / (SQUEEPAPER_WIDTHD/2) + ( (y*4) / SQUEEPAPER_HEIGHT ) * 4;
		color &= 0xf;
		SQUEEPAPER_SendData( color | (color<<4) );
	}
	SQUEEPAPER_FinishData();
#endif
}


void SQUEEPAPER_Clear(uint8_t color)
{
	int i;
	int j;
#ifdef SKU13379
	SQUEEPAPER_BeginData();
	uint8_t cs = (color & 1)?0xff:0x00;
    for(i=0; i<SQUEEPAPER_WIDTHD/8; i++) {
        for(j=0; j<height; j++)
            SQUEEPAPER_SendData(cs );
    }
	SQUEEPAPER_FinishData();

	SQUEEPAPER_BeginData2();
	cs = (color / 2)?0x00:0xff;
    for(i=0; i<SQUEEPAPER_WIDTHD/8; i++) {
        for(j=0; j<height; j++)
            SQUEEPAPER_SendData(cs );
    }
	SQUEEPAPER_FinishData();
#else
	SQUEEPAPER_BeginData();
    for(i=0; i<width/2; i++) {
        for(j=0; j<height; j++)
            SQUEEPAPER_SendData((color<<4)|color);
    }
	SQUEEPAPER_FinishData();
#endif
}


void SQUEEPAPER_Setup()
{
	SQUEEPAPER_GPIO_SETUP
	SQUEEPAPER_Wait( 10 );
	SQUEEPAPER_ResetHi();
	SQUEEPAPER_Wait( 10 );

#ifdef SKU18295
    SQUEEPAPER_BusyHigh( 20 );
    SQUEEPAPER_SendCommand(0x00);
    SQUEEPAPER_SendData(0xEF);
    SQUEEPAPER_SendData(0x08);
    SQUEEPAPER_SendCommand(0x01);
    SQUEEPAPER_SendData(0x37);
    SQUEEPAPER_SendData(0x00);
    SQUEEPAPER_SendData(0x23);
    SQUEEPAPER_SendData(0x23);
    SQUEEPAPER_SendCommand(0x03);
    SQUEEPAPER_SendData(0x00);
    SQUEEPAPER_SendCommand(0x06);
    SQUEEPAPER_SendData(0xC7);
    SQUEEPAPER_SendData(0xC7);
    SQUEEPAPER_SendData(0x1D);
    SQUEEPAPER_SendCommand(0x30);
    SQUEEPAPER_SendData(0x3C);
    SQUEEPAPER_SendCommand(0x40);
    SQUEEPAPER_SendData(0x00);
    SQUEEPAPER_SendCommand(0x50);
    SQUEEPAPER_SendData(0x37);
    SQUEEPAPER_SendCommand(0x60);
    SQUEEPAPER_SendData(0x22);
    SQUEEPAPER_SendCommand(0x61);
    SQUEEPAPER_SendData(0x02);
    SQUEEPAPER_SendData(0x58);
    SQUEEPAPER_SendData(0x01);
    SQUEEPAPER_SendData(0xC0);
    SQUEEPAPER_SendCommand(0xE3);
    SQUEEPAPER_SendData(0xAA);
	
    SQUEEPAPER_Wait(50);
    SQUEEPAPER_SendCommand(0x50);
    SQUEEPAPER_SendData(0x37);
    SQUEEPAPER_Wait(50);

#elif defined( SKU14144 )

	SQUEEPAPER_SendCommand(0x01); // POWER_SETTING
	SQUEEPAPER_SendData(0x37);
	SQUEEPAPER_SendData(0x00);

	SQUEEPAPER_SendCommand(0x00); // PANEL_SETTING
	SQUEEPAPER_SendData(0xCF);
	SQUEEPAPER_SendData(0x08);

	SQUEEPAPER_SendCommand(0x06); // BOOSTER_SOFT_START
	SQUEEPAPER_SendData(0xc7);
	SQUEEPAPER_SendData(0xcc);
	SQUEEPAPER_SendData(0x28);

	SQUEEPAPER_SendCommand(0x04); // POWER_ON
	SQUEEPAPER_BusyHigh(1000);

	SQUEEPAPER_SendCommand(0x30); // PLL_CONTROL
	SQUEEPAPER_SendData(0x3c);

	SQUEEPAPER_SendCommand(0x41); // TEMPERATURE_CALIBRATION
	SQUEEPAPER_SendData(0x00);

	SQUEEPAPER_SendCommand(0x50); // VCOM_AND_DATA_INTERVAL_SETTING
	SQUEEPAPER_SendData(0x77);

	SQUEEPAPER_SendCommand(0x60); // TCON_SETTING
	SQUEEPAPER_SendData(0x22);

	SQUEEPAPER_SendCommand(0x61); // TCON_RESOLUTION
	SQUEEPAPER_SendData(SQUEEPAPER_WIDTH >> 8); // source 640
	SQUEEPAPER_SendData(SQUEEPAPER_WIDTH & 0xff);
	SQUEEPAPER_SendData(SQUEEPAPER_HEIGHT >> 8); // gate 384
	SQUEEPAPER_SendData(SQUEEPAPER_HEIGHT & 0xff);

	SQUEEPAPER_SendCommand(0x82); // VCM_DC_SETTING
	SQUEEPAPER_SendData(0x1E); // decide by LUT file

	SQUEEPAPER_SendCommand(0xe5); // FLASH MODE
	SQUEEPAPER_SendData(0x03);

#elif defined( SKU13379 )
    SQUEEPAPER_SendCommand(0x04); 
    SQUEEPAPER_BusyHigh(500);
    SQUEEPAPER_SendCommand(0x00);
    SQUEEPAPER_SendData(0x0f);
#endif

}


void SQUEEPAPER_Sleep()
{
#if defined( SKU18295 )
    SQUEEPAPER_Wait(100);
    SQUEEPAPER_SendCommand(0x07);
    SQUEEPAPER_SendData(0xA5);
    SQUEEPAPER_Wait(100);
	SQUEEPAPER_ResetLo();
#elif defined( SKU14144 )
    SQUEEPAPER_SendCommand(0x02); // POWER_OFF
    SQUEEPAPER_BusyHigh( 500 );
    SQUEEPAPER_SendCommand(0x07); // DEEP_SLEEP
    SQUEEPAPER_SendData(0XA5);;
#elif defined( SKU13379 )
    SQUEEPAPER_SendCommand(0X50);
    SQUEEPAPER_SendData(0xf7);		//border floating	
    SQUEEPAPER_SendCommand(0X02);  	//power off
    SQUEEPAPER_BusyHigh( 500 ); //waiting for the electronic paper IC to release the idle signal
    SQUEEPAPER_SendCommand(0X07);  	//deep sleep
    SQUEEPAPER_SendData(0xA5);
#endif
}
