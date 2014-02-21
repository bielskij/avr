/**
 * @file:   drv/i2c.h
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
 * Software implementation of I2C bus. It uses gpio directly.
 */

#ifndef DRV_I2C_H_
#define DRV_I2C_H_

#include "common/types.h"


#define DRV_I2C_MESSAGE_FLAG_WRITE 0
#define DRV_I2C_MESSAGE_FLAG_READ  1


typedef void (* DrvI2cDetectCallback)(_U8 address, _BOOL read, _BOOL write);


typedef struct _DrvI2cMessage {
	_U16 slaveAddress;
	_U8  flags;
	_U8 *data;
	_U16 dataSize;
} DrvI2cMessage;


void drv_i2c_initialize(void);

void drv_i2c_terminate(void);

void drv_i2c_detect(DrvI2cDetectCallback callback);

_U8 drv_i2c_transfer(DrvI2cMessage *messages, _U8 messagesCount);

#endif /* DRV_I2C_H_ */
