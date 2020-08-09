//
// Generic Waveshare ePaper / eInk driver by CNLohr, intended for ESP, AVR and STM ports.  Tested with a few displays.
//
// Copyright 2020 <>< Charles Lohr, freely licensable under the MIT/x11 or newBSD Licenses.
// (Feel free to copy-and-paste any sections of this anywhere you want, for any fitness or purpose, just leave my copyright line)

#ifndef _EPAPER_CNLOHR
#define _EPAPER_CNLOHR


/*
  TODO: Test on AVR
  TODO: Test color on non-AVR.
  TODO: Figure out if we can abuse the modes to use the two-step update used on
	the red SKU13379 on the yellow SKU14144 to provide exclusive zones.
*/


#ifdef CONFIG_H
#include "config.h"
#endif

#include <stdint.h>


//#define SKU18295
#define SKU14144
//#define SKU13379



#ifdef SKU18295
//5.65 inch color epaper display
//https://www.waveshare.com/product/displays/e-paper/epaper-1/5.65inch-e-paper-module-f.htm
#define EPCN_WIDTH       600
#define EPCN_HEIGHT      448
#define EPD_5IN65F_BLACK   0x0	/// 000
#define EPD_5IN65F_WHITE   0x1	///	001
#define EPD_5IN65F_GREEN   0x2	///	010
#define EPD_5IN65F_BLUE    0x3	///	011
#define EPD_5IN65F_RED     0x4	///	100
#define EPD_5IN65F_YELLOW  0x5	///	101
#define EPD_5IN65F_ORANGE  0x6	///	110
#define EPD_5IN65F_CLEAN   0x7	///	111   unavailable  Afterimage

#elif( defined ( SKU14144 ) )
//7.5 inch Black+Yellow ePaper display
//https://www.waveshare.com/7.5inch-e-paper-c.htm
#define EPCN_WIDTH       640
#define EPCN_HEIGHT      384
#define EPD_4_COLOR_BLACK  0
#define EPD_4_COLOR_WHITE  1
#define EPD_4_COLOR_YELLOW 4
#define EPD_4_COLOR_CLEAR  5

#elif( defined ( SKU13379 ) )
//400x300, 4.2inch E-Ink raw display, three-color
//https://www.waveshare.com/4.2inch-e-paper-b.htm
#define EPCN_WIDTH       400
#define EPCN_HEIGHT      300


#else
#error Need at elast one SKU defined for epaper display.
#endif


#ifdef STM32F042

#define EPCN_GPIO_SETUP { \
	ConfigureGPIO( WVS_BUSY, INOUT_IN ); \
	ConfigureGPIO( WVS_RESET, INOUT_OUT | DEFAULT_OFF ); \
	ConfigureGPIO( WVS_DC, INOUT_OUT ); \
	ConfigureGPIO( WVS_CS, INOUT_OUT | DEFAULT_ON ); \
	ConfigureGPIO( WVS_CLK, INOUT_OUT | DEFAULT_OFF ); \
	ConfigureGPIO( WVS_DIN, INOUT_OUT | DEFAULT_OFF ); \
}
#define EPCN_DELAY_MS(x) _delay_ms(x)


#elif defined( AVR )

#include <avr/io.h>

#define WVS_BUSY  1
#define WVS_RESET 2
#define WVS_DC    3
#define WVS_CS    4
#define WVS_CLK   5
#define WVS_DIN   6
#define WVS_PORT  PORTD
#define WVS_PIN   PIND
#define WVS_DDR   DDRD


#define EPCN_DELAY

#define EPCN_BUSY      ( !! (WVS_PIN & _BV(WVS_BUSY) ) )
#define EPCN_RESET1    { WVS_PORT |= _BV(WVS_RESET); }
#define EPCN_RESET0    { WVS_PORT &= ~_BV(WVS_RESET); }
#define EPCN_DC1       { WVS_PORT |= _BV(WVS_DC); }
#define EPCN_DC0       { WVS_PORT &= ~_BV(WVS_DC); }
#define EPCN_CS1       { WVS_PORT |= _BV(WVS_CS); }
#define EPCN_CS0       { WVS_PORT &= ~_BV(WVS_CS); }
#define EPCN_CLK1      { WVS_PORT |= _BV(WVS_CLK); }
#define EPCN_CLK0      { WVS_PORT &= ~_BV(WVS_CLK); }
#define EPCN_DIN1      { WVS_PORT |= _BV(WVS_DIN); }
#define EPCN_DIN0      { WVS_PORT &= ~_BV(WVS_DIN); }
#define EPCN_DELAY_MS(x) _delay_ms(x)

#define EPCN_GPIO_SETUP { \
		GPIOOn( WVS_CS ); GPIOOff( WVS_CLK ); GPIOOff( WVS_DIN ); GPIOOff( WVS_RESET ); \
		WVS_DDR &= ~_BV(WVS_BUSY); WVS_DDR |= _BV(WVS_DIN); \
		WVS_DDR |= _BV(WVS_RESET); WVS_DDR |= _BV(WVS_DC); \
		WVS_DDR |= _BV(WVS_CS);    WVS_DDR |= _BV(WVS_CLK); \
		WVS_PORT |= _BV(WVS_BUSY); \
	}


#elif ESP_PLATFORM

#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define EPCN_BUSY      ( gpio_get_level(GPIO_NUM_25) )
#define EPCN_RESET1    { gpio_set_level(GPIO_NUM_26,1); }
#define EPCN_RESET0    { gpio_set_level(GPIO_NUM_26,0); }
#define EPCN_DC1       { gpio_set_level(GPIO_NUM_27,1); }
#define EPCN_DC0       { gpio_set_level(GPIO_NUM_27,0); }
#define EPCN_CS1       { gpio_set_level(GPIO_NUM_15,1); }
#define EPCN_CS0       { gpio_set_level(GPIO_NUM_15,0); }
#define EPCN_CLK1      { gpio_set_level(GPIO_NUM_13,1); }
#define EPCN_CLK0      { gpio_set_level(GPIO_NUM_13,0); }
#define EPCN_DIN1      { gpio_set_level(GPIO_NUM_14,1); }
#define EPCN_DIN0      { gpio_set_level(GPIO_NUM_14,0); }
#define EPCN_DELAY    { int i; for( i = 0; i < 10; i++ ) { asm volatile( "nop" ); } }
#define EPCN_DELAY_MS(x) vTaskDelay( ((x) * configTICK_RATE_HZ + 999) / 1000 );

#define EPCN_GPIO_SETUP { \
	gpio_reset_pin( GPIO_NUM_25 ); gpio_set_direction(GPIO_NUM_25, GPIO_MODE_INPUT); \
	gpio_reset_pin( GPIO_NUM_26 ); EPCN_RESET0; gpio_set_direction(GPIO_NUM_26, GPIO_MODE_OUTPUT); EPCN_RESET0; \
	gpio_reset_pin( GPIO_NUM_27 ); gpio_set_direction(GPIO_NUM_27, GPIO_MODE_OUTPUT); EPCN_DC0; \
	gpio_reset_pin( GPIO_NUM_15 ); gpio_set_direction(GPIO_NUM_15, GPIO_MODE_OUTPUT); EPCN_CS1 \
	gpio_reset_pin( GPIO_NUM_13 ); gpio_set_direction(GPIO_NUM_13, GPIO_MODE_OUTPUT); EPCN_CLK0; \
	gpio_reset_pin( GPIO_NUM_14 ); gpio_set_direction(GPIO_NUM_14, GPIO_MODE_OUTPUT); EPCN_DIN0; \
	}

#else

#error no platform defined

#endif


#define EPCN_WIDTHD (((EPCN_WIDTH % 8 == 0)? (EPCN_WIDTH / 8 ): (EPCN_WIDTH / 8 + 1))*8)

#define EPCN_CLEAR_COLOR 1
#define EPCN_BLACK_COLOR 0

void EPCN_Setup();
void EPCN_BeginData();
void EPCN_BeginData2(); //For other color if using weird red display, like SKU13379
void EPCN_SendArray( uint8_t * data, int len );
void EPCN_FinishData();
void EPCN_SendData( uint8_t data );
void EPCN_Clear(uint8_t color);
void EPCN_TestPattern();

void EPCN_Sleep();

#ifdef SKU18295
void EPD_5IN65F_Show7Block(void);
#endif

#endif

