//Copyright 2013 <>< Charles Lohr, All Rights Reserved, May be licensed under MIT/x11 or newBSD licenses, you choose.

#ifndef _MICROSD_H
#define _MICROSD_H

//Uses ~4 bytes of static ram

#include <stdint.h>
#include "config.h"

//#define DISABLE_READ_CRC

//You may define (in eth_config.h)
// SDCMDP
// SDCLKP
// SDDAT0P
// SDPORT, SDPIN, SDDDR
//NOTE: Data 0..3 MUST be connected to the first 4 bits of the port.  Therefore DAT0P MUST ALWAYS BE 0!

#ifndef SDCMDP
#define SDCMDP 5
#endif

#ifndef SDCLKP
#define SDCLKP  4
#endif

#ifndef SDDAT0P
#define SDDAT0P 0
#endif

#define SDCLK  _BV( SDCLKP )
#define SDCMD  _BV( SDCMDP )
#define SDDAT0  _BV( SDDAT0P )

#ifndef SDPORT
#define SDPORT PORTC
#endif

#ifndef SDPIN
#define SDPIN  PINC
#endif

#ifndef SDDDR
#define SDDDR  DDRC
#endif


//returns nonzero if failure.
unsigned char initSD();

//returns nonzero if failure.
unsigned char startSDwrite( uint32_t sector );
void pushSDwrite( unsigned char c );
//nonzero indicates failure
unsigned char endSDwrite();

//returns nonzero if failure.
unsigned char startSDread( uint32_t sector );
unsigned char popSDread();
unsigned char endSDread(); //returns nonzero if the CRC failed.

//Send a specified number of 0xFF's bytes.  (uses control)
void dumpSDDAT( unsigned short count );

//Do this for data.
void dumpSDR( unsigned short todump );

extern uint16_t opsleftSD;


#endif

