/**
 * @file:   drv/i2c.c
 * @date:   2014-02-19
 * @Author: Jaroslaw Bielski (bielski.j@gmail.com)
 *
 **********************************************
 * Copyright (c) 2013 Jaroslaw Bielski.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the GNU Public License v3.0
 * which accompanies this distribution, and is available at
 * http://www.gnu.org/licenses/gpl.html
 *
 * Contributors:
 *     Jaroslaw Bielski (bielski.j@gmail.com)
 *********************************************
 *
 *
 * @brief
 *
 * Software implementation of I2C bus. It directly uses gpio.
 *
 * !! WARNING !!
 * It is only proof of concept. These piece of code was written to check radio tuner driver!
 */

#include <avr/io.h>
#include <util/delay.h>

#include "common/utils.h"

#include "drv/i2c.h"


#define SDA_BANK C
#define SDA_PIO  4

#define SCL_BANK C
#define SCL_PIO  5

#define SDA_DDR  DECLARE_DDR(SDA_BANK)
#define SDA_PORT DECLARE_PORT(SDA_BANK)
#define SDA_PIN  DECLARE_PIN(SDA_PIO)

#define SCL_DDR  DECLARE_DDR(SCL_BANK)
#define SCL_PORT DECLARE_PORT(SCL_BANK)
#define SCL_PIN  DECLARE_PIN(SCL_PIO)

// 100 kHz
#define I2C_WAIT _delay_us(2)


#define BIT_READ  1
#define BIT_WRITE 0


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
	_sclLow();
	{
		I2C_WAIT;

		_sdaOut();
		_sdaLow();

		I2C_WAIT;
	}
	_sclHigh();

	I2C_WAIT;

	_sdaHigh();

	I2C_WAIT;
}


static _U8 _getByte(_BOOL stop) {
	_U8 i;
	_U8 readByte = 0;

	for (i = 0; i < 8; i++) {
		readByte <<= 1;

		_sclLow();
		{
			I2C_WAIT;

			_sdaIn();
			_sdaHigh();

			I2C_WAIT;
		}
		_sclHigh();

		I2C_WAIT;

		if (PIO_IS_HIGH(DECLARE_PIN(SDA_BANK), DECLARE_PIN(SDA_PIO))) {
			readByte |= 0x01;
		}

		I2C_WAIT;
	}

	_sclLow();
	{
		I2C_WAIT;

		_sdaOut();

		if (stop) {
			_sdaHigh();

		} else {
			_sdaLow();
		}

		I2C_WAIT;
	}
	_sclHigh();

	I2C_WAIT;

	if (stop) {
		_stop();
	}

	return readByte;
}


static _BOOL _putByte(_U8 byte, _BOOL start, _BOOL stop) {
	_BOOL ack;

	if (start) {
		_sdaOut();
		_sdaLow();

		I2C_WAIT;
	}

	_U8 i;

	for (i = 0; i < 8; i++) {
		_sclLow();
		{
			I2C_WAIT;

			_sdaOut();

			if (byte & 0x80) {
				_sdaHigh();

			} else {
				_sdaLow();
			}

			I2C_WAIT;
		}
		_sclHigh();

		I2C_WAIT;

		byte <<= 1;
	}

	_sclLow();
	{
		I2C_WAIT;

		_sdaIn();
		_sdaHigh();

		I2C_WAIT;
	}
	_sclHigh();

	I2C_WAIT;

	ack = PIO_IS_LOW(DECLARE_PIN(SDA_BANK), DECLARE_PIN(SDA_PIO));

	if (stop) {
		_stop();
	}

	return ack;
}


void drv_i2c_initialize(void) {
	_sdaHigh();
	_sdaOut();

	_sclHigh();
	_sclOut();
}


void drv_i2c_terminate(void) {
	_sclHigh();
	_sclIn();

	_sdaLow();
	_sdaIn();
}


void drv_i2c_detect(DrvI2cDetectCallback callback) {
	_U8 i;

	// 7bit addressing 0x08 - 0x77
	for (i = 0x08; i <= 0x77; i++) {
		_BOOL ackR = _putByte((i << 1) | 0x01, TRUE, TRUE);
		_BOOL ackW = _putByte((i << 1),        TRUE, TRUE);
		if (ackR || ackW) {
			callback(i, ackR, ackW);
		}
	}
}


_S8 drv_i2c_transfer(DrvI2cMessage *messages, _U8 messagesCount) {
	_S8 ret = 0;

	{
		_U8 i;
		_U8 j;

		for (i = 0; i < messagesCount; i++) {
			DrvI2cMessage *currentMessage = messages + i;

			_BOOL stop  = FALSE;

			// Write address
			{
				_U8 address = currentMessage->slaveAddress << 1;

				if (currentMessage->flags & DRV_I2C_MESSAGE_FLAG_READ) {
					address |= 0x01;
				}

				ret = _putByte(address, TRUE, FALSE);
			}

			for (j = 0; j < currentMessage->dataSize; j++) {
				if (i == messagesCount - 1 && j == currentMessage->dataSize -1) {
					stop = TRUE;
				}

				if (currentMessage->flags & DRV_I2C_MESSAGE_FLAG_READ) {
					currentMessage->data[j] = _getByte(stop);

				} else {
					ret = _putByte(currentMessage->data[j], FALSE, stop);
				}
			}
		}
	}

	return 0;
}
