
#include "bootloader/common/crc8.h"

/*
 * polynomials: reversed
 * CRC-8-CCITT	I.432.1; ATM HEC, ISDN HEC and cell delineation 0xE0
 * CRC-8-Dallas/Maxim	1-Wire bus                              0x8C
 * CRC-8                                                        0xAB
 * CRC-8-SAE J1850	AES3                                        0xB8
 * CRC-8-WCDMA                                                  0xD9
 */


_U8 crc8_getForByte(_U8 byte, _U8 polynomial, _U8 start) {
	_U8 remainder = start;

	remainder ^= byte;

	{
		_U8 bit;

		for (bit = 0; bit < 8; bit++) {
			if (remainder & 0x01) {
				remainder = (remainder >> 1) ^ polynomial;

			} else {
				remainder = (remainder >> 1);
			}

		}
	}

    return remainder;
}


_U8 crc8_get(_U8 *buffer, _U16 bufferSize, _U8 polynomial, _U8 start) {
	_U8 remainder = start;
	_U8 byte;

	// Perform modulo-2 division, a byte at a time.
	for (byte = 0; byte < bufferSize; ++byte) {
		remainder = crc8_getForByte(buffer[byte], polynomial, remainder);
	}

    return remainder;
}
