#include "ePaperColor.h"
#include "avr_print.h"

#define width EPD_WIDTH
#define height EPD_HEIGHT

#define GPIOOff( x )  { WVS_PORT &= ~_BV(x); }
#define GPIOOn( x )   { WVS_PORT |= _BV(x); }
#define GPIORead( x ) ( !! (WVS_PIN & _BV(x) ) )
//#define WVS_PORT  PORTD
//#define WVS_PIN   PIND
//#define WVS_DDR   DDRD


/*static void Delay()
{
		asm volatile( "nop" );
		asm volatile( "nop" );
}
*/

//#define Delay() { asm volatile( "nop" ); }
#define Delay()

static void EPD_5IN65F_BusyHigh( uint16_t timeout )// If BUSYN=0 then waiting
{
    while(!(GPIORead(WVS_BUSY)))
	{
		if( timeout-- == 0 ) break;
		_delay_ms(1);
	}
}

static void EPD_5IN65F_BusyLow( uint16_t timeout )// If BUSYN=1 then waiting
{
    while(GPIORead(WVS_BUSY))
	{
		if( timeout-- == 0 ) break;
		_delay_ms(1);
	}
}

static void SpiTransfer( uint8_t data )
{
	int bit;
	GPIOOff( WVS_CS );  Delay();
	GPIOOff( WVS_CLK ); Delay();

//	for( bit = 0x80; bit; bit>>= 1)
	for( bit = 8; bit; bit-- )
	{
		if( data & 0x80 )
		{
			GPIOOn( WVS_DIN );
		}
		else
		{
			GPIOOff( WVS_DIN );
		}
		data<<=1;

		Delay();
		GPIOOn( WVS_CLK );
		Delay();
		GPIOOff( WVS_CLK );
	}
	GPIOOn( WVS_CS );
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
    SendCommand(0x61);//Set Resolution setting
    SendData(0x02);
    SendData(0x58);
    SendData(0x01);
    SendData(0xC0);
    SendCommand(0x10);
}

void SendEPaperData( uint8_t * data, int len )
{
	int i;
	for( i = 0; i < len; i++ )
		SendData( data[i] );
}

void FlushAndDisplayEPaper()
{
    SendCommand(0x04);//0x04
    EPD_5IN65F_BusyHigh(150);
    SendCommand(0x12);//0x12
    EPD_5IN65F_BusyHigh(15000);
    SendCommand(0x02);  //0x02
    EPD_5IN65F_BusyLow(150);
	_delay_us(20000);
}


void ClearEpaper(uint8_t color)
{
	SetupEPaperForData();
	uint8_t cv = (color<<4)|color;
    for(int i=0; i<width/2; i++) {
        for(int j=0; j<height; j++)
            SendData(cv);
    }
	FlushAndDisplayEPaper();
}


void EPD_5IN65F_Show7Block(void)
{
    unsigned long i,j,k;
    unsigned char const Color_seven[8] =
	{EPD_5IN65F_BLACK,EPD_5IN65F_BLUE,EPD_5IN65F_GREEN,EPD_5IN65F_ORANGE,
	EPD_5IN65F_RED,EPD_5IN65F_YELLOW,EPD_5IN65F_WHITE,7};
	SetupEPaperForData();

    for(i=0; i<224; i++) {
        for(k = 0 ; k < 4; k ++) {
            for(j = 0 ; j < 75; j ++) {
                SendData((Color_seven[k]<<4) |Color_seven[k]);
            }
        }
    }
    for(i=0; i<224; i++) {
        for(k = 4 ; k < 8; k ++) {
            for(j = 0 ; j < 75; j ++) {
                SendData((Color_seven[k]<<4) |Color_seven[k]);
            }
        }
    }
	FlushAndDisplayEPaper();
}




static void Clear(uint8_t color)
{
	SetupEPaperForData();
    for(int i=0; i<width/2; i++) {
        for(int j=0; j<height; j++)
            SendData((color<<4)|color);
    }
	FlushAndDisplayEPaper();
}


void SetupEPaperDisplay()
{
#if 0
	ConfigureGPIO( WVS_BUSY, INOUT_IN );
	ConfigureGPIO( WVS_RESET, INOUT_OUT | DEFAULT_OFF );
	ConfigureGPIO( WVS_DC, INOUT_OUT );
	ConfigureGPIO( WVS_CS, INOUT_OUT | DEFAULT_ON );
	ConfigureGPIO( WVS_CLK, INOUT_OUT | DEFAULT_OFF );
	ConfigureGPIO( WVS_DIN, INOUT_OUT | DEFAULT_OFF );
#endif

//#define WVS_PORT  PORTD
//#define WVS_PIN   PIND
//#define WVS_DDR   DDRD

	GPIOOn( WVS_CS );
	GPIOOff( WVS_CLK );
	GPIOOff( WVS_DIN );
	GPIOOff( WVS_RESET );

	WVS_DDR &= ~_BV(WVS_BUSY);
	WVS_DDR |= _BV(WVS_DIN);
	WVS_DDR |= _BV(WVS_RESET);
	WVS_DDR |= _BV(WVS_DC);
	WVS_DDR |= _BV(WVS_CS);
	WVS_DDR |= _BV(WVS_CLK);

	WVS_PORT |= _BV(WVS_BUSY); //Pull-up.

	//Reset for 1ms
	_delay_ms( 10 );

	GPIOOn( WVS_RESET );

	_delay_ms( 10 );

#if 0
	_delay_ms(100);
	while(1)
	{
		GPIOOn( WVS_CS );
		GPIOOff( WVS_CS );
	}
#endif
	sendstr( "Epaper Leaving Reset\n" );
    EPD_5IN65F_BusyHigh( 20 );
	sendstr( "Epaper Busy High Done\n" );



    SendCommand(0x00);
    SendData(0xEF);
    SendData(0x08);
    SendCommand(0x01);
    SendData(0x37);
    SendData(0x00);
    SendData(0x23);
    SendData(0x23);
    SendCommand(0x03);
    SendData(0x00);
    SendCommand(0x06);
    SendData(0xC7);
    SendData(0xC7);
    SendData(0x1D);
    SendCommand(0x30);
    SendData(0x3C);
    SendCommand(0x40);
    SendData(0x00);
    SendCommand(0x50);
    SendData(0x37);
    SendCommand(0x60);
    SendData(0x22);
    SendCommand(0x61);
    SendData(0x02);
    SendData(0x58);
    SendData(0x01);
    SendData(0xC0);
    SendCommand(0xE3);
    SendData(0xAA);
	
    _delay_us(50000);
    SendCommand(0x50);
    SendData(0x37);
    _delay_us(50000);
}

