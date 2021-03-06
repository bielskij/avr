#include <stdio.h>

#include <avr/io.h>
#include <avr/interrupt.h>

#include <util/delay.h>

#include "common/utils.h"

#include "common/debug.h"

// PB1 - OC1A - timer1 output
// PC5 - ADC5 - comparator negative

// 942 nF

// supported inductance range: 4,7uH - 50 mH

#define USE_INTERNAL_COMPARATOR 0

#define LOAD_PIO_BANK C
#define LOAD_PIO_PIN  5

#define SAMPLES_COUNT 8


volatile _U8  samplesCount;
volatile _U16 samples[SAMPLES_COUNT];


static FILE mystdout = FDEV_SETUP_STREAM(debug_putc, NULL, _FDEV_SETUP_WRITE);


ISR(TIMER1_CAPT_vect) {
	if (samplesCount >= SAMPLES_COUNT) {
		return;
	}

	samples[samplesCount++] = ICR1;
}

#if USE_INTERNAL_COMPARATOR
ISR(ANALOG_COMP_vect) {
	ACSR &= ~_BV(ACIE);

	// enables the input capture function in Timer/Counter1 to be triggered
	// by the Analog Comparator.
	ACSR |= _BV(ACIC);

	// Start timer1
	TCCR1B |= _BV(CS11);
}
#endif


static void _loadPioSet(_BOOL high) {
	if (high) {
		SET_PIO_AS_OUTPUT(DECLARE_DDR(LOAD_PIO_BANK), DECLARE_PIN(LOAD_PIO_PIN));
		SET_PIO_HIGH(DECLARE_PORT(LOAD_PIO_BANK), DECLARE_PIN(LOAD_PIO_PIN));

	} else {
		SET_PIO_AS_INPUT(DECLARE_DDR(LOAD_PIO_BANK), DECLARE_PIN(LOAD_PIO_PIN));
		SET_PIO_LOW(DECLARE_PORT(LOAD_PIO_BANK), DECLARE_PIN(LOAD_PIO_PIN));
	}
}


void main(void) {
	debug_initialize();

	{
		stdout = &mystdout;

		DBG(("START"));

		sei();
#if USE_INTERNAL_COMPARATOR
		{
			// Enable ADC (without this input multiplexer is not available)
			PRR &= ~_BV(PRADC);

			// Enable multiplexer for AD comparator
			ADCSRB |= _BV(ACME);

			// ADC5 - negative input
			ADMUX |= (_BV(MUX2) | _BV(MUX0));

			// Interrupt on rising edge
			ACSR |= _BV(ACIS0);
			ACSR |= _BV(ACIS1);
		}
#endif

		while (1) {
			char c = debug_getc();

			switch (c) {
				case 'l':
					{
						// Load capacitor with coil
						_loadPioSet(TRUE);

						_delay_ms(400);

						// timer1
						{
							TCNT1 = 0;

							// Enable interrupt on capture
							TIMSK1 |= _BV(ICIE1);

							samplesCount = 0;

#if USE_INTERNAL_COMPARATOR
							// Trigger on falling edge
							TCCR1B &= ~_BV(ICES1);
#else
							// Trigger on rising edge
							TCCR1B |= _BV(ICES1);
#endif
						}
#if USE_INTERNAL_COMPARATOR
						// Enable interrupt on Analog comparator
						ACSR |= _BV(ACIE);
#else
						// Start timer1
						TCCR1B |= _BV(CS11);
#endif
						//
						_loadPioSet(FALSE);

						_delay_ms(200);
#if USE_INTERNAL_COMPARATOR
						// Disable interrupt on Analog comparator
						ACSR &= ~_BV(ACIE);
#endif
						// Stop timer
						{
							TIMSK1 &= ~_BV(ICIE1);
							TCCR1B &= ~_BV(CS11);
						}
#if USE_INTERNAL_COMPARATOR
						// Disable ICP
						ACSR &= ~_BV(ACIC);
#endif
//						printf("Samples: %d %d %d %d %d\r\n", samplesCount, samples[0], samples[1], samples[2], samples[3]);

						{
							float frequency = 0;
							float inductance = 0;

							_U8 i;

							if (samplesCount >= 2) {
								frequency = (F_CPU / 8) / (float)(samples[2] - samples[1]);

								inductance = 1000.0 / (float) (4.0 * M_PI * M_PI * frequency * frequency * 0.000000942); // w mH
							}

							printf("samples: %d, frequency: %g Hz, inductance: %g mH\r\n", samplesCount, frequency, inductance);
						}
					}
					break;
			}
		}
	}
}
