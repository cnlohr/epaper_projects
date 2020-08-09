//Copyright 2012-2013, 2020 <>< Charles Lohr, All Rights Reserved, May be licensed under MIT/x11 or newBSD licenses, you choose.

#include "fullsd.h"
#include <avr/io.h>
#include "avr_print.h"
#include <avr/pgmspace.h>
#include <util/delay.h>
#include <util/crc16.h>

#ifdef MUTE_PRINTF
#undef sendhex2
#define sendhex2( x ) x;
#endif

#define NOOP asm volatile("nop" ::)

void tickSDUntilLowCMD();

//may go away.
static unsigned char WaitFor( unsigned char c );

void microSPIWCMD(unsigned char ins);
unsigned char microSPIRCMD();

void tickSDUntilLowDAT();
void microSPIWDAT(unsigned char ins);
unsigned char microSPIRDAT();
void dumpSD( unsigned short count ); //for raw dumping
void microSPID();

static unsigned char	highcap; //the 0x40 bit indicates if it's SDXC (1) or SDSC (0)
uint32_t RCA;


//New SD Functions!

//Call command, return R1
void SDCommand( uint8_t cmd, uint16_t valueh, uint16_t valuel );


static uint8_t tackcrc(uint8_t crc, uint8_t data);
static uint8_t cmdcrcw;


void crctest();
void RunStatus( uint8_t id, uint8_t line );

unsigned char initSD()
{
	unsigned long ocrr;
	unsigned char i = 0;
	unsigned short j;
	unsigned char tries = 0;

//	crctest();
//	while(1);
//SDCMD, SDCLK, SDDAT0
restart:
	SDDDR |= SDCLK;
	SDDDR &= ~(SDCMD);
	SDDDR &= 0xF0; //Make DATA0...up inputs
	
	SDPORT |= SDCMD | 0x0f; //Make DATA0 up... high
	SDDDR |= (SDCMD | 0x0f);

	_delay_ms(100);

	for( i = 0; i < 74; i++ )
	{
		SDPORT |= SDCLK;
		SDPORT &=~SDCLK;
	}

//	SDDDR &= ~(SDCMD);

/*
	//??? LINUXS????
	SDCommand( 0x40+52, 0, 0x0c00 );
	sendstr( "CMD52: " );
	sendhex2( microSPIRCMD() );
	sendhex2( microSPIRCMD() );
	sendhex2( microSPIRCMD() );
	sendhex2( microSPIRCMD() );
	sendhex2( microSPIRCMD() );
	sendhex2( microSPIRCMD() );
	sendhex2( microSPIRCMD() );
	sendchr( '\n' );
	dumpsd(100);

	SDCommand( 0x40+52, 0x8000, 0x0c08 ); dumpsd(100);
	sendstr( "CMD52: " );
	sendhex2( microSPIRCMD() );
	sendhex2( microSPIRCMD() );
	sendhex2( microSPIRCMD() );
	sendhex2( microSPIRCMD() );
	sendhex2( microSPIRCMD() );
	sendhex2( microSPIRCMD() );
	sendhex2( microSPIRCMD() );
	sendchr( '\n' );
	dumpsd(100);

	//END LINUXSXX
*/

	SDCommand( 0x40+0, 0, 0 ); dumpSD(8);
	SDCommand( 0x40+8, 0, 0x1aa );  //cmd 8 (0x01 = 2.7-3.6v, 0xaa = pattern)
	if( microSPIRCMD() & 0x80 )
	{
		tries++;
		_delay_ms(10);
		if( tries > 10 )
			goto fail_cmd8;
		else
			goto restart;
	}
	dumpSD( 2 );
	if( microSPIRCMD() != 0x01 )
		goto fail_cmd8;
	if( microSPIRCMD() != 0xaa )
		goto fail_cmd8;
	dumpSD( 2 );

	RunStatus( 0x02, 0 );
cmd1:
	SDCommand( 0x40+55, 0, 0 ); dumpSD(16); //cmd 55 (then 41)
//	SDCommand( 0x40+41, 0x5030, 0x0000 ); //ACMD41 High capacity, max perf, valid window of 3.2-3.4V.  XXX SHOULD BE 5030  LINUX SAYS 5130

	//XXX ALTERED BY CHARLES LATER DON'T KNOW?
	SDCommand( 0x40+41, 0x4030, 0x0000 ); //ACMD41 High capacity, max perf, valid window of 3.2-3.4V.  XXX SHOULD BE 5030  LINUX SAYS 5130

	i = microSPIRCMD();
	if( i & 0x80 ) goto fail_cmd1;
	highcap = microSPIRCMD();
	sendchr( 0 );
	sendstr( "ACMD41: " );
	sendhex2( i );
	sendhex2( highcap );
	sendhex2( microSPIRCMD() );
	sendhex2( microSPIRCMD() );
	sendhex2( microSPIRCMD() );
	sendhex2( microSPIRCMD() );
	sendhex2( microSPIRCMD() );
	sendchr( '\n' );
	dumpSD(10);
	if( !( highcap & 0x80) )
	{
		_delay_ms(10);
		if( tries++ < 10 )
		{
			_delay_ms(10);
			goto cmd1;
		}
		else
			goto fail_cmd1;
	}

	highcap = (highcap&0x40)?1:0;
	RunStatus( 0x03, 0 );


	//TODO: Should check against host's voltage capabilities.  For Transcend, it looks like it's FF8, so the whole range is oaky.

	//Now, we are effectively at sd.c's "mmc_sd_init_card" line.   Also check out chart 3.9.4 on page 17 of summary spec

	SDCommand( 0x40+2, 0x0000, 0x0000 );  //Read CID from any available cards.
	sendchr(0);
	sendstr( "\nCMD2:" );
	sendhex2( microSPIRCMD() );
	sendhex2( microSPIRCMD() );
	sendhex2( microSPIRCMD() );
	sendhex2( microSPIRCMD() );
	sendhex2( microSPIRCMD() );
	sendhex2( microSPIRCMD() );
	sendchr('\n');
	dumpSD(40); //We could do something with this data,but we don't need to.  It is laid out in 5.2

	SDCommand( 0x40+3, 0x0000, 0x0000 ); //Request new relative address.
	microSPIRCMD();
	RCA = microSPIRCMD();
	RCA = (RCA<<8) | microSPIRCMD();

	sendstr(":RCA:" ); 
	sendhex2( RCA >> 8);
	sendhex2( RCA );
	sendstr("\n");
	dumpSD(20);

	RunStatus( 0x04, 0 );

	//CMD9 IS ABSOLUTELY CRITICAL
	SDCommand( 0x40+9, RCA, 0x0000 );  //Read CID from any available cards.
	sendchr(0);
	sendstr( "\nCMD9:" );

	sendhex2( microSPIRCMD() );
	sendhex2( microSPIRCMD() );
	sendhex2( microSPIRCMD() );
	sendhex2( microSPIRCMD() );
	sendhex2( microSPIRCMD() );
	sendhex2( microSPIRCMD() );
	sendhex2( microSPIRCMD() );

	sendchr('\n');
	dumpSD(40); //We could do something with this data,but we don't need to.  It is laid out in 5.2

/*
	SDCommand( 0x40+55, RCA, 0 ); dumpSD(32); //cmd 55 (then 51)
	SDCommand( 0x40+13, 0, 0x0000 );  //Read CID from any available cards.
	sendchr(0);
	sendstr( "\nACMD13:" );

	sendhex2( microSPIRCMD() );
	sendhex2( microSPIRCMD() );
	sendhex2( microSPIRCMD() );
	sendhex2( microSPIRCMD() );
	sendhex2( microSPIRCMD() );
	sendhex2( microSPIRCMD() );
	sendhex2( microSPIRCMD() );

	sendchr('\n');
	dumpSD(40); //We could do something with this data,but we don't need to.  It is laid out in 5.2
	dumpSD(512);
	sendchr( '\n' );
*/

	SDCommand( 0x40+12, 0, 0 ); //CMD12 (done block transfer)
	dumpSD(128);


/*
	SDCommand( 0x40+3, 0x0000, 0x0000 ); //Request new relative address.
	microSPIRCMD();
	RCA = microSPIRCMD();
	RCA = (RCA<<8) | microSPIRCMD();

	sendstr(":RCA:" ); 
	sendhex2( RCA >> 8);
	sendhex2( RCA );
	sendstr("\n");
	dumpSD(20);*/
	RunStatus( 0x05, 0 );

	//Select card.
	SDCommand( 0x40+7, RCA, 0 );  //CMD7
	sendchr(0);
	sendstr("CMD7:");
	sendhex2( microSPIRCMD() );
	sendhex2( microSPIRCMD() );
	sendhex2( microSPIRCMD() );
	sendhex2( microSPIRCMD() );
	sendhex2( microSPIRCMD() );
	sendhex2( microSPIRCMD() );
	sendhex2( microSPIRCMD() );
	sendstr("\n");
	dumpSD(20);

/*
	SDCommand( 0x40+59, 0, 0 );  //CMD59
	sendchr(0);
	sendstr("CMD59:");
	sendhex2( microSPIRCMD() );
	sendhex2( microSPIRCMD() );
	sendhex2( microSPIRCMD() );
	sendhex2( microSPIRCMD() );
	sendhex2( microSPIRCMD() );
	sendhex2( microSPIRCMD() );
	sendhex2( microSPIRCMD() );
	sendstr("\n");
	dumpSD(20);
*/



	//Works: Do you want 4-bit mode???
	//ACMD6
	SDCommand( 0x40+55, RCA, 0 ); //dumpSD(8); //cmd 55 (then 41)
	sendchr(0);
	sendstr("CMD55:");
	sendhex2( microSPIRCMD() );
	sendhex2( microSPIRCMD() );
	sendhex2( microSPIRCMD() );
	sendhex2( microSPIRCMD() );
	sendhex2( microSPIRCMD() );
	sendhex2( microSPIRCMD() );
	sendhex2( microSPIRCMD() );
	dumpSD(20);
	SDCommand( 0x40+6, 0x00, 0x0002 ); //ACMD6 Switch 4-bit mode with CMD6
	sendchr( 0 );
	sendstr( "\nACMD6:" );
	for( j = 0; j < 10; j++ )
	{
		sendchr(0);

		sendhex2( microSPIRCMD() );
	}
	sendchr('\n' );
	dumpSD(100);

	RunStatus( 0x06, 0 );


	//Get the card's SCR
	SDCommand( 0x40+55, RCA, 0 ); dumpSD(32); //cmd 55 (then 51)
	SDCommand( 0x40+51, 0, 0 );  //CMD51 - SD_APP_SEND_SCR
	sendchr(0);
	sendstr("ACMD51:");
	sendhex2( microSPIRCMD() );
	sendhex2( microSPIRCMD() );
	sendhex2( microSPIRCMD() );
	sendhex2( microSPIRCMD() );

	sendchr('\n');
	tickSDUntilLowDAT();

	for( j = 0; j < 32; j++ )
	{
		sendhex2( microSPIRDAT() );
		sendchr( ' ' );
	}
	dumpSD(10);
	sendchr( '\n' );

	RunStatus( 0xA7, 0 );

/*
	SDCommand( 0x40+12, 0, 0 ); //CMD12 (done block transfer)
	_delay_ms(200);
	tickSDUntilLowCMD();
	sendchr( 0 );
	sendstr( "\nCMD12:" );
	sendchr('!');
	for( j = 0; j < 30; j++ )
	{
		sendchr(0);

		sendhex2( microSPIRCMD() );
		sendchr( ' ' );
	}
	sendchr('\n' );
	dumpSD(100);
*/

//	SDCommand( 0x40+23, 0x0000, 0xffff ); //CMD23? NOPE. Does NOT HELP
//	dumpSD(100);

//	SDCommand( 0x40+20, 0x0000, 0x0000 ); //CMD20? NOPE. Does NOT HELP
//	dumpSD(100);
//	_delay_ms(100);

retry_16:

	SDCommand( 0x40+16, 0x0000, 0x0200 ); //CMD16 Switch to 512-byte sectors  (Only availalble on smaller cards) (This can fail and it is OK)
	sendchr( 0 );
	sendstr( "CMD16: ");
	sendhex2( microSPIRCMD() );
	sendhex2( microSPIRCMD() );
	sendhex2( microSPIRCMD() );

	sendhex2( i = microSPIRCMD() );
	sendchr( '\n' );
	dumpSD(20);

	if( i != 9 )
	{
		_delay_ms(10);
		tries++;

		if( tries > 1000 )
			goto fail_cmd16;
		else
			goto retry_16;
	}

	RunStatus( 0x08, 0 );

	dumpSD(10);
	SDCommand( 0x40+6, 0x80ff, 0xfff1 ); //CMD6 play with this  (Probaly should try "supporting" higher power cards)

	sendchr(0);
	sendstr( "CMD6:" );

	sendhex2( microSPIRCMD() );
	sendhex2( microSPIRCMD() );
	sendhex2( microSPIRCMD() );
	sendhex2( microSPIRCMD() );
	sendhex2( microSPIRCMD() );
	sendhex2( microSPIRCMD() );

	sendchr('\n');
	tickSDUntilLowDAT();

	for( j = 0; j < 73; j++ )
	{
		sendhex2( microSPIRDAT() );
		sendchr( ' ' );
	}
	dumpSD(10);
	sendchr( '\n' );

/*
	SDCommand( 0x40+12, 0, 0 ); //CMD12 (done block transfer)
	sendchr( 0 );
	sendstr( "\nCMD12:" );
	sendchr('!');
	for( j = 0; j < 30; j++ )
	{
		sendchr(0);

		sendhex2( microSPIRCMD() );
		sendchr( ' ' );
	}
	sendchr('\n' );
	dumpSD(100);
*/

//Do we want to get the card status? 
	SDCommand( 0x40+55, RCA, 0 ); dumpSD(32); //cmd 55 (then 13)
	SDCommand( 0x40+13, 0, 0 );  //CMD13 - GET SD CARD STATUS
	sendchr(0);
	sendstr("ACMD13:");
	sendhex2( microSPIRCMD() );
	sendhex2( microSPIRCMD() );
	sendhex2( microSPIRCMD() );
	sendhex2( microSPIRCMD() );

	sendchr('\n');
	tickSDUntilLowDAT();

	for( j = 0; j < 73; j++ )
	{
		sendhex2( microSPIRDAT() );
		sendchr( ' ' );
	}
	dumpSD(512);
	sendchr( '\n' );

	SDCommand( 0x40+12, 0, 0 ); //CMD12 (done block transfer)
	dumpSD(10);
	dumpSD(512);


	RunStatus( 0x09, 0 );


	SDCommand( 0x40+55, RCA, 0 );
	sendstr("CMD55:");
	sendhex2( microSPIRCMD() );
	sendhex2( microSPIRCMD() );
	sendhex2( microSPIRCMD() );
	sendhex2( microSPIRCMD() );
	sendhex2( microSPIRCMD() );
	sendhex2( microSPIRCMD() );
	sendhex2( microSPIRCMD() );
	sendchr ('\n' );
	dumpSD(20);
	SDCommand( 0x40+42, 0x0000, 0x0000 ); //ACMD42 change resistor properties
	sendstr("ACMD42:");
	sendhex2( microSPIRCMD() );
	sendhex2( microSPIRCMD() );
	sendhex2( microSPIRCMD() );
	sendhex2( microSPIRCMD() );
	sendhex2( microSPIRCMD() );
	sendhex2( microSPIRCMD() );
	sendhex2( microSPIRCMD() );
	sendchr ('\n' );
	dumpSD(20);

dumpSD(250);
	return 0;

fail_cmd16:
	sendstr( "Failed on CMD16\n" );
	return 2;
fail_cmd7:
	sendstr( "Failed on CMD7\n" );
	return 3;
fail_cmd8:
	sendstr( "Failed on CMD8\n" );
	return 4;
fail_cmd1:
	sendstr( "Failed on CMD1\n" );
	return 5;

}



uint8_t enablecrc;
uint16_t opsleftSD;
uint8_t nibblecrcs[16];
uint16_t nibcount;
uint8_t nibpl = 0;

uint8_t nibcrc( uint8_t v )
{
	uint8_t npl;
	uint8_t ret = nibblecrcs[nibpl];
	uint8_t mux = ret ^ v;

	if( !enablecrc ) return ret;

	npl = (nibpl-12)&0x0f;
	nibblecrcs[npl] = nibblecrcs[npl]^mux;
	npl = (nibpl-5)&0x0f;
	nibblecrcs[npl] = nibblecrcs[npl]^mux;

	nibblecrcs[nibpl] = mux;

	nibpl = (nibpl+1)&0x0f;

	nibcount++;
	return ret;
}


uint8_t nibcrcpop()
{
	uint8_t ret = nibblecrcs[nibpl];
	nibpl = (nibpl+1)&0x0f;
	return ret;
}


void nibreset()
{
	uint8_t i;
	for( i = 0; i < 16; i++ )
		nibblecrcs[i] = 0x00;
	nibpl = 0;
	nibcount = 0;
	enablecrc = 1;
}


//returns nonzero if failure.
unsigned char startSDread( uint32_t sector )
{
	SDDDR |= 0x0f;

	dumpSD(100);
#ifndef READBLMODE

#endif

//	SDCommand( 0x40+7, RCA, 0 ); dumpSD(10);//Activate card.
	SDCommand( 0x40+17, sector>>16, sector&0xffff ); //read cmd
	//XXX Should read out the command here, I think.
	dumpSD(5);
	SDDDR &= ~0x0f; //don't drive now.
	tickSDUntilLowDAT();
	SDPORT |= SDCLK;

#ifndef DISABLE_READ_CRC
	nibreset();
#endif

	opsleftSD = 512;

}

unsigned char popSDread()
{
	uint8_t ret;
	if( opsleftSD )
	{
		opsleftSD--;
		ret =  microSPIRDAT();
		return ret;
	}
	return 0;
}


void dumpSDDAT( unsigned short todump )
{
	uint8_t ret;
	uint16_t i;
	for(i = 0; i < todump;i++)
	{
		if( opsleftSD == 0 ) return;
		opsleftSD--;
		ret =  microSPIRDAT();
	}
}

unsigned char endSDread()
{
	uint8_t badcrc = 0;
	uint16_t gotcrc[4];
	uint8_t r;

	while( opsleftSD )
	{
		opsleftSD--;
		microSPIRDAT();
//		datacrc = _crc_xmodem_update( datacrc, r);
	}
	enablecrc = 0;

	for( r = 0; r < 8; r++ )
	{
		uint8_t got = microSPIRDAT();

#ifndef DISABLE_READ_CRC
		uint8_t exp = nibcrcpop();
		exp = (exp<<4) | nibcrcpop();

		if( exp != got )
		{
			badcrc = 1;
/*			sendstr( "Bad CRC:" );
			sendhex2( exp );
			sendchr(' ' );
			sendhex2( got );
			sendchr( '\n' );
*/
		}
#endif

	}

/*
	if( badcrc )
	{
		sendchr ( 0 );
		sendchr ( 'b' );
	}
*/

/*	if( gotcrc != datacrc )
	{
		sendchr( 0 );
		sendstr( "BAD CRC.\n" );
		return 1;
	}
*/


//	SDCommand( 0x40+7, 0, 0 );  //CMD7
	dumpSD(10);

	return badcrc;
//	uint16_t readcrc
}




//returns nonzero if failure.
unsigned char startSDwrite( uint32_t sector )
{
	dumpSD(10);
//	SDCommand( 0x40+7, RCA, 0 ); dumpSD(10);//Activate card.
	SDCommand( 0x40+24, sector>>16, sector&0xffff ); //write cmd
	dumpSD(15);

	SDDDR |= 0x0f; //drive data lines

	//Do a data write.

	microSPIWDAT( 0xF0 );

	nibreset();
	opsleftSD = 512;
}

void pushSDwrite( unsigned char c )
{
	if( opsleftSD )
	{
		opsleftSD--;
		microSPIWDAT( c );
	}
}

//nonzero indicates failure
unsigned char endSDwrite()
{
	uint8_t r;

	while( opsleftSD )
	{
		opsleftSD--;
		pushSDwrite( 0xff );
	}

	//send CRCs

	enablecrc = 0;

	for( r = 0; r < 8; r++ )
	{
		uint8_t exp = nibcrcpop();
		exp = (exp<<4) | nibcrcpop();

		microSPIWDAT( exp );
	}

	microSPIWDAT( 0x0f );

	dumpSD( 10 );

//	SDCommand( 0x40+12, 0, 0 ); //CMD12 (done block transfer)

//	dumpSD(600);

	SDDDR &= 0xf0;

	_delay_ms(20);

}













void SDCommand( uint8_t cmd, uint16_t valueh, uint16_t valuel )
{
	uint8_t ret;
	cmdcrcw = 0;
	microSPIWCMD( cmd );
	microSPIWCMD( valueh >> 8 );
	microSPIWCMD( valueh & 0xff );
	microSPIWCMD( valuel >> 8 );
	microSPIWCMD( valuel & 0xff );
	microSPIWCMD( (cmdcrcw<<1)|1 );
	tickSDUntilLowCMD();
}


void tickSDUntilLowCMD()
{
	unsigned short i = 8192;
	SDPORT &= ~SDCLK;
	SDDDR &= ~SDCMD;
	while( i-- )
	{
		SDPORT |= SDCLK; NOOP; NOOP; NOOP; NOOP;
		if( !( SDPIN & SDCMD ) ) break;
		SDPORT &= ~SDCLK; NOOP;
	}
}



unsigned char microSPIRCMD()
{
	unsigned char ret = 0;
	unsigned char i;

	SDDDR &= ~SDCMD;

	for( i = 0; i < 7; i++ )
	{
		if( SDPIN & SDCMD )
			ret |= 0x01;
		SDPORT &= ~SDCLK;
		ret <<= 1;
		SDPORT |= SDCLK;
	}
	if( SDPIN & SDCMD )
		ret |= 0x01;
	SDPORT &= ~SDCLK;
	NOOP;
	SDPORT |= SDCLK;

	return ret;
}

void microSPIWCMD(unsigned char ins)
{
	unsigned char i;
	SDPORT &= ~SDCLK;
	SDDDR |= SDCMD;

	cmdcrcw = tackcrc( cmdcrcw, ins );

	for( i = 0; i < 8; i++ )
	{
		SDPORT &= ~SDCLK;
		if( ins & 0x80 )
			SDPORT |= SDCMD;
		else
			SDPORT &= ~SDCMD;
		ins <<= 1;
		SDPORT |= SDCLK;
	}
	//SDPORT &= ~SDCLK;
	SDDDR &= ~SDCMD;
}



void tickSDUntilLowDAT()
{
	unsigned short i = 8192;
	SDPORT |= 0x0f;
	SDPORT &= ~SDCLK;
	SDDDR &= 0xf0;
	while( i-- )
	{
		SDPORT |= SDCLK; NOOP; NOOP;NOOP; NOOP;
		if( !( SDPIN & (SDDAT0) ) ) break;
		SDPORT &= ~SDCLK; NOOP; NOOP;  NOOP; NOOP;
//		if( !( SDPIN & (SDDAT0) ) ) break;
	}

	SDPORT &= ~SDCLK;
	NOOP;
	SDPORT |= SDCLK;

}


unsigned char microSPIRDAT()
{
	unsigned char ret = 0;
	unsigned char i;

	SDDDR &= 0xF0; //Set data as input

	SDPORT &= ~SDCLK;
	NOOP;
	SDPORT |= SDCLK;

	ret = (SDPIN)<<4;
#ifndef DISABLE_READ_CRC
	nibcrc( (ret>>4) );
#endif

	SDPORT &= ~SDCLK;
	NOOP;
	SDPORT |= SDCLK;

	ret |= (SDPIN&0x0f);
#ifndef DISABLE_READ_CRC
	nibcrc( ret & 0x0f );
#endif

	SDPORT &= ~SDCLK;
	return ret;
}


void microSPIWDAT(unsigned char ins)
{
	unsigned char ret = 0;
	unsigned char i;

	SDDDR |= 0x0F; //Set data as output


	SDPORT &= ~SDCLK;

	SDPORT &= 0xf0;
	SDPORT |= ins>>4;

	nibcrc( ins>>4 );

	SDPORT |= SDCLK;
	NOOP;	NOOP;

	SDPORT &= ~SDCLK;

	SDPORT &= 0xf0;
	SDPORT |= ins&0x0f;

	nibcrc( ins & 0x0f );

	SDPORT |= SDCLK;
	NOOP; NOOP;

	return;

}

void microSPID()
{
	unsigned char i;
//	SDPORT |= SDMOSI;
	for( i = 0; i < 8; )
	{
		SDPORT &= ~SDCLK;
		i++;
		SDPORT |= SDCLK;
	}
}


void dumpSD( unsigned short todump )
{
	uint16_t i;
	for(i = 0; i < todump;i++)
	{
		microSPID();
	}
}

//CRC TOOLS
//X^7 + X^3 + 1
static const uint8_t crc7_table[256] PROGMEM = {
        0x00, 0x09, 0x12, 0x1b, 0x24, 0x2d, 0x36, 0x3f,
        0x48, 0x41, 0x5a, 0x53, 0x6c, 0x65, 0x7e, 0x77,
        0x19, 0x10, 0x0b, 0x02, 0x3d, 0x34, 0x2f, 0x26,
        0x51, 0x58, 0x43, 0x4a, 0x75, 0x7c, 0x67, 0x6e,
        0x32, 0x3b, 0x20, 0x29, 0x16, 0x1f, 0x04, 0x0d,
        0x7a, 0x73, 0x68, 0x61, 0x5e, 0x57, 0x4c, 0x45,
        0x2b, 0x22, 0x39, 0x30, 0x0f, 0x06, 0x1d, 0x14,
        0x63, 0x6a, 0x71, 0x78, 0x47, 0x4e, 0x55, 0x5c,
        0x64, 0x6d, 0x76, 0x7f, 0x40, 0x49, 0x52, 0x5b,
        0x2c, 0x25, 0x3e, 0x37, 0x08, 0x01, 0x1a, 0x13,
        0x7d, 0x74, 0x6f, 0x66, 0x59, 0x50, 0x4b, 0x42,
        0x35, 0x3c, 0x27, 0x2e, 0x11, 0x18, 0x03, 0x0a,
        0x56, 0x5f, 0x44, 0x4d, 0x72, 0x7b, 0x60, 0x69,
        0x1e, 0x17, 0x0c, 0x05, 0x3a, 0x33, 0x28, 0x21,
        0x4f, 0x46, 0x5d, 0x54, 0x6b, 0x62, 0x79, 0x70,
        0x07, 0x0e, 0x15, 0x1c, 0x23, 0x2a, 0x31, 0x38,
        0x41, 0x48, 0x53, 0x5a, 0x65, 0x6c, 0x77, 0x7e,
        0x09, 0x00, 0x1b, 0x12, 0x2d, 0x24, 0x3f, 0x36,
        0x58, 0x51, 0x4a, 0x43, 0x7c, 0x75, 0x6e, 0x67,
        0x10, 0x19, 0x02, 0x0b, 0x34, 0x3d, 0x26, 0x2f,
        0x73, 0x7a, 0x61, 0x68, 0x57, 0x5e, 0x45, 0x4c,
        0x3b, 0x32, 0x29, 0x20, 0x1f, 0x16, 0x0d, 0x04,
        0x6a, 0x63, 0x78, 0x71, 0x4e, 0x47, 0x5c, 0x55,
        0x22, 0x2b, 0x30, 0x39, 0x06, 0x0f, 0x14, 0x1d,
        0x25, 0x2c, 0x37, 0x3e, 0x01, 0x08, 0x13, 0x1a,
        0x6d, 0x64, 0x7f, 0x76, 0x49, 0x40, 0x5b, 0x52,
        0x3c, 0x35, 0x2e, 0x27, 0x18, 0x11, 0x0a, 0x03,
        0x74, 0x7d, 0x66, 0x6f, 0x50, 0x59, 0x42, 0x4b,
        0x17, 0x1e, 0x05, 0x0c, 0x33, 0x3a, 0x21, 0x28,
        0x5f, 0x56, 0x4d, 0x44, 0x7b, 0x72, 0x69, 0x60,
        0x0e, 0x07, 0x1c, 0x15, 0x2a, 0x23, 0x38, 0x31,
        0x46, 0x4f, 0x54, 0x5d, 0x62, 0x6b, 0x70, 0x79
};


static uint8_t tackcrc(uint8_t crc, uint8_t data)
{
	return pgm_read_byte( crc7_table + ((crc << 1) ^ data) );
}















/*

void crctest()
{
	uint16_t crcs[4];
	uint16_t j;
	uint16_t tcrc = 0;

	nibreset();
	for( j = 0; j < 512*8; j++ )
	{
		nibcrc( 0x01 );
	}

	sendchr ('\n' );
	crcs[0] = crcs[1] = crcs[2] = crcs[3] = 0;


	for( j = 0; j < 512; j++ )
	{
		tcrc = _crc_xmodem_update( tcrc, 0xff );
//		sendhex4( tcrc );
//		sendchr ('\n' );
	}

	for( j = 0; j < 16; j++ )
	{
		uint8_t v = 
			//nibcrc(0xff);
			nibcrcpop();
		sendchr('!');
		sendhex2( v );
		sendchr('!');
		crcs[0] = (crcs[0]<<1) | ((v&1)?1:0);
		crcs[1] = (crcs[1]<<1) | ((v&2)?1:0);
		crcs[2] = (crcs[2]<<1) | ((v&4)?1:0);
		crcs[3] = (crcs[3]<<1) | ((v&8)?1:0);

	}
	sendchr ('\n' );

	sendchr( 0 );
	sendhex4( crcs[0] );
	sendchr( '/' );
	sendhex4( crcs[1] );
	sendchr( '/' );
	sendhex4( crcs[2] );
	sendchr( '/' );
	sendhex4( crcs[3] );
	sendchr( '\n' );
	sendhex4( tcrc );
}*/
