#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/atomic.h>

#include "common/types.h"
#include "common/utils.h"

#define UART_BAUD 38400

#define UART_BAUD_REG (((F_CPU / 16) / UART_BAUD) - 1)

//#define USE_INTERRUPTS

#if defined(USE_INTERRUPTS)

#define UART_FIFO_SIZE 8

typedef struct _Fifo {
	volatile _U8  buffer[UART_FIFO_SIZE];
	volatile _U8 *producer;
	volatile _U8 *consumer;
	volatile _U8  used;
} Fifo;

static Fifo txFifo;


static void _fifoPut(Fifo *fifo, _U8 byte) {
	while (fifo->used == UART_FIFO_SIZE) {};

	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
		fifo->producer++;

		fifo->producer = fifo->buffer + ((fifo->producer - fifo->buffer) % UART_FIFO_SIZE);

		*( fifo->producer ) = byte;

		fifo->used++;

		if (fifo->used == 1) {
			SET_BIT_AT(UCSR0B, UDRIE0);
		}
	}
}


// Data could be send interrupt
ISR(USART_UDRE_vect) {
	txFifo.consumer++;

	txFifo.consumer = txFifo.buffer + ((txFifo.consumer - txFifo.buffer) % UART_FIFO_SIZE);

	UDR0 = *txFifo.consumer;

	txFifo.used--;

	if (txFifo.used == 0) {
		CLEAR_BIT_AT(UCSR0B, UDRIE0);
	}
}
#else
#define _waitForTransmit() {                \
	while (! (UCSR0A & ONE_LEFT_SHIFTED(UDRE0))); \
}
#endif


void debug_initialize(void) {

	// Initialize IO fifo
#if defined(USE_INTERRUPTS)
	txFifo.consumer = txFifo.buffer;
	txFifo.producer = txFifo.buffer;
	txFifo.used     = 0;
#endif

	// Configure usart
	UBRR0H = ((UART_BAUD_REG) >> 8);
	UBRR0L = ((UART_BAUD_REG) & 0x00FF);

	SET_BIT_AT(UCSR0B, TXEN0);
	SET_BIT_AT(UCSR0B, RXEN0);

	// 8bit, 1bit stop, no parity
	UCSR0C = ONE_LEFT_SHIFTED(UCSZ00) | ONE_LEFT_SHIFTED(UCSZ01);
}


void debug_terminate(void) {
	// From datasheet
	UCSR0B = 0x00;
	UCSR0C = 0x06;
	UBRR0L = 0;
	UBRR0H = 0;
}


void debug_print(char *buffer) {
	while (*buffer != 0) {
#if defined(USE_INTERRUPTS)
		_fifoPut(&txFifo, *buffer);
#else
		_waitForTransmit();

		UDR0 = *buffer;
#endif
		buffer++;
	}
}


void debug_putc(char c) {
#if defined(USE_INTERRUPTS)
		_fifoPut(&txFifo, *buffer);
#else
		_waitForTransmit();

		UDR0 = c;
#endif
}


void debug_dumpHex(_U16 data, _U8 lengthInBytes) {
	_S8 i;

	for (i = (lengthInBytes * 2) - 1; i >= 0; i--) {
		_U8 nibble = ((data >> (i * 4)) & 0x0F);

		if (nibble > 9) {
#if defined(USE_INTERRUPTS)
			_fifoPut(&txFifo, nibble + 55);
#else
			_waitForTransmit();

			UDR0 = nibble + 55;
#endif
		} else {
#if defined(USE_INTERRUPTS)
			_fifoPut(&txFifo, nibble + 48);
#else
			_waitForTransmit();

			UDR0 = nibble + 48;
#endif
		}
	}
}


char debug_getc(void) {
	char ret = 0;

	while (!(UCSR0A & (1<<RXC0))) {};
	// Get and return received data from buffer

	ret = UDR0;

	return ret;
}


_U8 debug_readLine(char *buffer, _U16 bufferSize) {
	_U8 ret = 0;

	do {
		if (bufferSize < 3) {
			break;
		}

		buffer[ret++] = debug_getc();
		buffer[ret++] = debug_getc();

		while (
			(buffer[ret - 2] != '\r' && buffer[ret - 1] != '\n') &&
			(ret < bufferSize)
		) {
			buffer[ret++] = debug_getc();
		}

		buffer[ret] = '\0';
	} while (0);

	return ret;
}
