/**
 * @file:   bootloader/common/debug.h
 * @date:   2013-09-17
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
 * @brief Common debug API.
 *
 * This API provide common methods used to debug the source code.
 */

#ifndef BOOTLOADER_COMMON_DEBUG_H_
#define BOOTLOADER_COMMON_DEBUG_H_

#include "bootloader/common/types.h"

/**
 * @name
 * @brief
 * @ingroup
 *
 * Initializes debug API.
 */
void debug_initialize(void);

/**
 * @name
 * @brief
 * @ingroup
 *
 * Terminates debug API.
 */
void debug_terminate(void);

/**
 * @name
 * @brief
 * @ingroup
 *
 * Prints string buffer (it must be terminated by null character '\0')
 *
 * @param[in] buffer  string to print.
 */
void drv_debug_print(char *buffer);

/**
 * @name
 * @brief
 * @ingroup
 *
 * Prints variable value with hex base.
 *
 * @param[in] data          variable to print.
 * @param[in] lengthInBytes variable length in bytes
 */
void drv_debug_dumpHex(_U16 data, _U8 lengthInBytes);

#ifdef ENABLE_DEBUG
	#define DBG(x) { debug_print x; debug_print("\n"); }
#else
	#define DBG(x) {}
#endif

#endif /* BOOTLOADER_COMMON_DEBUG_H_ */
