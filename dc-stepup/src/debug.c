/*
 * debug.c
 *
 *  Created on: 09-12-2014
 *      Author: jarko
 */


#include <avr/io.h>

#include "debug.h"


#define SDA_BANK B
#define SDA_PIO  2

#define SCL_BANK B
#define SCL_PIO  0


#define SET_PIO_HIGH(port, pin) { port |= _BV(pin); }
#define SET_PIO_LOW(port, pin)  { port &= ~_BV(pin); }

#define PIO_IS_HIGH(pin, pinno) (bit_is_set(pin, pinno))
#define PIO_IS_LOW(pin, pinno) (! bit_is_set(pin, pinno))

#define SET_PIO_AS_OUTPUT(ddr, pin) { ddr |= _BV(pin); }
#define SET_PIO_AS_INPUT(ddr, pin)  { ddr &= ~(_BV(pin)); }

#define CONCAT_MACRO(port, letter) port ## letter
#define DECLARE_PORT(port) CONCAT_MACRO(PORT, port)
#define DECLARE_DDR(port)  CONCAT_MACRO(DDR,  port)
#define DECLARE_PIN(port)  CONCAT_MACRO(PIN,  port)


#define SDA_DDR  DECLARE_DDR(SDA_BANK)
#define SDA_PORT DECLARE_PORT(SDA_BANK)
#define SDA_PIN  DECLARE_PIN(SDA_PIO)

#define SCL_DDR  DECLARE_DDR(SCL_BANK)
#define SCL_PORT DECLARE_PORT(SCL_BANK)
#define SCL_PIN  DECLARE_PIN(SCL_PIO)


#define I2C_WAIT __asm__ __volatile__("nop");


static void _sdaIn(void) {
	SET_PIO_AS_INPUT(SDA_DDR, SDA_PIN);
}


static void _sdaOut(void) {
	SET_PIO_AS_OUTPUT(SDA_DDR, SDA_PIN);
}


static void _sdaHigh(void) {
	SET_PIO_HIGH(SDA_PORT, SDA_PIN);
}


static void _sdaLow(void) {
	SET_PIO_LOW(SDA_PORT, SDA_PIN);
}


static void _sclOut(void) {
	SET_PIO_AS_OUTPUT(SCL_DDR, SCL_PIN);
}


static void _sclIn(void) {
	SET_PIO_AS_INPUT(SCL_DDR, SCL_PIN);
}


static void _sclHigh(void) {
	SET_PIO_HIGH(SCL_PORT, SCL_PIN);
}


static void _sclLow(void) {
	SET_PIO_LOW(SCL_PORT, SCL_PIN);
}


static void _stop(void) {
	_sdaOut();
	_sdaLow();

	I2C_WAIT;

	_sclHigh();

	I2C_WAIT;

	_sdaHigh();

	I2C_WAIT;
}


static void _start(void) {
	_sdaOut();

	_sdaHigh();

	I2C_WAIT;

	_sclHigh();

	I2C_WAIT;

	_sdaLow();

	I2C_WAIT;

	_sclLow();

	I2C_WAIT;
}


static uint8_t _byteSend(uint8_t byte) {
	uint8_t ack;

	{
		uint8_t i;

		for (i = 0; i < 8; i++) {
			_sdaOut();

			if (byte & 0x80) {
				_sdaHigh();

			} else {
				_sdaLow();
			}

			I2C_WAIT;

			_sclHigh();

			I2C_WAIT;
			I2C_WAIT;

			_sclLow();

			I2C_WAIT;

			byte <<= 1;
		}
	}

	_sdaIn();
	_sdaHigh();

	I2C_WAIT;

	_sclHigh();

	I2C_WAIT;

	ack = PIO_IS_LOW(DECLARE_PIN(SDA_BANK), DECLARE_PIN(SDA_PIO));

	I2C_WAIT;

	_sclLow();

	I2C_WAIT;

	return ack;
}


void debug_sendByte(uint8_t b) {
	_start();
	_byteSend(b);
	_stop();
}


void debug_initialize(void) {
	_sdaHigh();
	_sdaOut();

	_sclHigh();
	_sclOut();
}
