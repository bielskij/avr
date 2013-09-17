#include <avr/io.h>

#include "bootloader/common/types.h"
#include "bootloader/common/utils.h"

#define UART_BAUD 38400

#define UART_BAUD_REG (((F_CPU / 16) / UART_BAUD) - 1)


#define UART_PUT_CHAR(c) { \
	while (! (UCSR0A & ONE_LEFT_SHIFTED(UDRE0))); \
	UDR0 = c; \
}


void debug_initialize(void) {
	UBRR0H = ((UART_BAUD_REG) >> 8);
	UBRR0L = ((UART_BAUD_REG) & 0x00FF);

	// Only transmitter
	UCSR0B = ONE_LEFT_SHIFTED(TXEN0);
	// 8bit, 1bit stop, no parity
	UCSR0C = ONE_LEFT_SHIFTED(UCSZ00) | ONE_LEFT_SHIFTED(UCSZ01);
}


void debug_terminate(void) {
	UCSR0B = 0x00;
	UCSR0C = 0x06;
	UBRR0L = 0;
	UBRR0H = 0;
}


void debug_print(char *buffer) {
	while (*buffer != 0) {
		UART_PUT_CHAR(*buffer);

		buffer++;
	}
}


void debug_dumpHex(_U16 data, _U8 lengthInBytes) {
	_S8 i;

	for (i = (lengthInBytes * 2) - 1; i >= 0; i--) {
		_U8 nibble = ((data >> (i * 4)) & 0x0F);

		if (nibble > 9) {
			UART_PUT_CHAR(nibble + 55);

		} else {
			UART_PUT_CHAR(nibble + 48);
		}
	}
}
