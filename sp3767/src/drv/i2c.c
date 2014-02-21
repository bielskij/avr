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
 * Software implementation of I2C (only master mode) bus. It directly uses gpio.
 *
 */

#include <avr/io.h>
#include <util/delay.h>

#include "common/utils.h"
#include "common/debug.h"

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


static _U8 _byteRead(_BOOL ack) {
	_U8 readByte = 0;

	_sdaIn();
	_sdaHigh();

	{
		_U8 i;

		for (i = 0; i < 8; i++) {
			readByte <<= 1;

			I2C_WAIT;

			_sclHigh();

			I2C_WAIT;

			if (PIO_IS_HIGH(DECLARE_PIN(SDA_BANK), DECLARE_PIN(SDA_PIO))) {
				readByte |= 0x01;
			}

			I2C_WAIT;

			_sclLow();

			I2C_WAIT;
		}
	}

	{
		_sdaOut();

		if (ack) {
			_sdaLow();

		} else {
			_sdaHigh();
		}

		I2C_WAIT;
	}
	_sclHigh();

	I2C_WAIT;
	I2C_WAIT;

	_sclLow();

	I2C_WAIT;

	return readByte;
}


static _BOOL _byteSend(_U8 byte) {
	_BOOL ack;

	{
		_U8 i;

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
		_BOOL ackR;
		_BOOL ackW;

		_start();
		ackR = _byteSend(i << 1 | 0x01);
		_stop();

		_start();
		ackW = _byteSend(i << 1);
		_stop();

		if (ackR || ackW) {
			callback(i, ackR, ackW);
		}
	}
}


_U8 drv_i2c_transfer(DrvI2cMessage *msgs, _U8 msgsCount) {
	_U8 ret = msgsCount;

	while (msgsCount > 0) {
		_BOOL stop        = FALSE;
		_BOOL lastMessage = FALSE;
		_BOOL ack;
		_U16 j;

		if (msgsCount == 1) {
			lastMessage = TRUE;
		}

		// If all data sent, send STOP
		if (lastMessage && (msgs->dataSize == 0)) {
			stop = TRUE;
		}

		// Write address
		{
			_U8 address = msgs->slaveAddress << 1;

			if (msgs->flags & DRV_I2C_MESSAGE_FLAG_READ) {
				address |= 0x01;
			}

			_start();
			ack = _byteSend(address);
			if (! ack) {
				break;
			}

			if (stop) {
				_stop();
			}
		}

		for (j = 0; j < msgs->dataSize; j++) {
			_BOOL lastByte = FALSE;

			if (j + 1 == msgs->dataSize) {
				lastByte = TRUE;
			}

			// If all data sent, send STOP
			if (lastMessage && lastByte) {
				stop = TRUE;
			}

			if (msgs->flags & DRV_I2C_MESSAGE_FLAG_READ) {
				msgs->data[j] = _byteRead(! stop);

			} else {
				ack = _byteSend(msgs->data[j]);
				if (! ack) {
					break;
				}
			}

			if (stop) {
				_stop();
			}
		}

		if (! ack) {
			break;
		}

		msgs++;
		msgsCount--;
	}

	ret -= msgsCount;

	return ret;
}
