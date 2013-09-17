#ifndef BOOTLOADER_COMMON_CRC8_H_
#define BOOTLOADER_COMMON_CRC8_H_

#include "bootloader/common/types.h"


_U8 crc8_getForByte(_U8 byte, _U8 polynomial, _U8 start);

_U8 crc8_get(_U8 *buffer, _U16 bufferSize, _U8 polynomial, _U8 start);


#endif /* BOOTLOADER_COMMON_CRC8_H_ */
