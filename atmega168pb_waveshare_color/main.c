#include "common.h"
#include "ePaperColor.h"
#include "basicfat.h"
#include "fullsd.h"
#include "avr_print.h"
#include <avr/sleep.h>

void RunStatus()
{
}

int pictureno = 0;
uint16_t seconds_over_8;

ISR(TIMER2_OVF_vect)
{
	seconds_over_8++;
	sendstr( "Seconds:" );
	sendhex4( seconds_over_8);
}

void rtc_init()
{
	TCCR2A = 0x00;  //overflow
	TCCR2B = 0x07;  //7 gives 8 sec. prescale
	TIMSK2 = 0x01;  //enable timer2A overflow interrupt
	ASSR  = 0x20;   //enable asynchronous mode
}

//Pictureno = 1 to n.
int UpdatePictureFromSDCard( int pictureno )
{
	//Micro SD Power (Off)
	PORTE |= _BV(2);
	DDRE |= _BV(2);

	sendstr( "Init epaper" );

	//Turn off power to ePaper
	PORTD |= _BV(7);
	DDRD |= _BV(7);
	_delay_ms(50);
	//Turn ePaper Power On
	PORTD &= ~_BV(7);
	DDRD |= _BV(6);

	SetupEPaperDisplay();
//	EPD_5IN65F_Show7Block();

	//Micro SD Card
	PORTC |= _BV(3);
	DDRC |= _BV(3);

	//Micro SD Power
	PORTE &= ~_BV(2);

	sendstr( "Init SD\n" );
	int of = initSD();

	if( of != 0 )
	{
		ClearEpaper( EPD_5IN65F_ORANGE );
		return -5;
	}

	sendstr( "Open FAT\n" );
	of = openFAT();
	sendstr( "Got FAT:" );
	sendhex4( of );

	if( of )
	{
		ClearEpaper( EPD_5IN65F_RED );
		return -6;
	}

	uint32_t filelen = 0;

	char fname[MAX_LONG_FILENAME];
//	memcpy( fname, "001.dat", 8 );
	uint32_t r = FindClusterFileInDir( fname, ROOT_CLUSTER, pictureno, &filelen );

	sendstr( "Got FN:");
	sendhex4( r>>16 );
	sendhex4( r&0xffff );
	sendhex4( filelen>>16 );
	sendhex4( filelen&0xffff );
	sendstr( fname );

	if( r < 0 )
	{
		ClearEpaper( EPD_5IN65F_BLUE );
		return -7;
	}
	else if( filelen == 0 )
	{
		ClearEpaper( EPD_5IN65F_WHITE );
		return -8;
	}
	else if( filelen == 0xffffffff )
	{
		ClearEpaper( EPD_5IN65F_GREEN );
		return -9;
	}
	else if( r )
	{
		filelen = 134400;
		struct FileInfo f;
		InitFileStructure( &f, r );
		StartReadFAT(&f); //Sector-aligned.

		SetupEPaperForData();
		uint32_t i;
		int j;
		for( i = 0; i < filelen; i+=512 )
		{
			char dat[1];
			for( j = 0; j < 512; j++ )
			{
				dat[0] = read8FAT();
				//dat[0] = EPD_5IN65F_YELLOW | (EPD_5IN65F_YELLOW<<4);
				SendEPaperData( dat, 1 );
			}
		}
		EndReadFAT();

		sendstr( "Read file completely.\n" );

		//Turn SD Card off
		PORTC &= 0xc0;
		PORTE |= _BV(2);
		DDRE |= _BV(2);

		FlushAndDisplayEPaper();
	}
	else
	{
		ClearEpaper( EPD_5IN65F_YELLOW );
		return -10;
	}

	sendstr( "Done\n" );

	//Turning everything back off.
	PORTD |= _BV(7);
	PORTD &= 0x82;

	//Turn SD Card off (if it was not already turned off)
	PORTC &= 0xc0;
	PORTE |= _BV(2);

	return 0;
}

int main()
{
	CLKPR = 0x80;
	CLKPR = 0;
	sendstr( "Initting RTC\n" );
	rtc_init();
	set_sleep_mode(SLEEP_MODE_PWR_SAVE);

	seconds_over_8 = 0;
	pictureno++;
	int r = UpdatePictureFromSDCard( pictureno );

	while(1)
	{
		sleep_enable();
		sleep_mode();
		sleep_disable();
		if( seconds_over_8 > 10 )
		{
			seconds_over_8 = 0;
			pictureno++;
			r = UpdatePictureFromSDCard( pictureno );
			sendstr( "Update picture:" );
			sendhex2( r );
			if( r < -6 )
			{
				pictureno = 0;
				pictureno++;
				r = UpdatePictureFromSDCard( pictureno );
			}
		}
		sendstr( "Awoken\n" );
	}
}

