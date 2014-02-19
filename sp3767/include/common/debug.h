#ifndef COMMON_DEBUG_H_
#define COMMON_DEBUG_H_

#include "common/types.h"

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
void debug_print(char *buffer);

/**
 * @name
 * @brief
 * @ingroup
 *
 * Prints formated string
 *
 * @param[in] format  string format.
 */
void debug_printf(char *format, ...);

/**
 * @name
 * @brief
 * @ingroup
 *
 * Prints variable value in hex base.
 *
 * @param[in] data          variable to print.
 * @param[in] lengthInBytes variable length in bytes
 */
void debug_dumpHex(_U16 data, _U8 lengthInBytes);

/**
 * @name
 * @brief
 * @ingroup
 *
 * Prints variable value in decimal base.
 *
 * @param[in] data          variable to print.
 * @param[in] lengthInBytes variable length in bytes
 */
void debug_dumpDec(_U32 data);

#if defined(ENABLE_DEBUG)
	#define DBG(x) { debug_print x; debug_print("\r\n"); }
#else
	#define DBG(x) {}
#endif

void debug_putc(char c);

char debug_getc(void);

_U8 debug_readLine(char *buffer, _U16 bufferSize);

#endif /* COMMON_DEBUG_H_ */
