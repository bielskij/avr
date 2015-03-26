/*
 * main.c
 *
 *  Created on: 25-11-2014
 *      Author: jarko
 */
#include "common/avr.h"
#include "common/types.h"
#include "common/utils.h"

#define TIMER_TOP                    128

#define PWM_OCR_MAX                   90 // ~0.71
#define PWM_ADC_VOLTAGE_MIN           15 //

#define ADC_BUFFER_SIZE 7

//#define ADC_MEDIAN_INSTEAD_OF_AVERAGE

#define ADC_V_BANK B
#define ADC_V_PIN  3

#define STBY_BANK  B
#define STBY_PIN   4

#define OK_BANK    B
#define OK_PIN     0

#define TIMER_BANK B
#define TIMER_PIN  1


static void timerStart(_BOOL start);


ISR(PCINT0_vect) {
	timerStart(PIO_IS_LOW(STBY_BANK, STBY_PIN));
}


void timerStart(_BOOL start) {
	if (start) {
		// Start timer - no prescaling
		// In fast pwm mode frequency is ~147kHz
		TCCR0B |= _BV(CS00);
		TCCR0A |= (_BV(COM0B1) | _BV(COM0B0));

	} else {
		TCCR0A &= ~(_BV(COM0B1) | _BV(COM0B0));
		TCCR0B &= ~_BV(CS00);
	}
}


#if defined(ADC_MEDIAN_INSTEAD_OF_AVERAGE)
static void _bubbleSortInPlace(uint8_t *buffer, uint8_t  bufferSize) {
	uint8_t n = bufferSize;

	do {
		uint8_t i;

		for (i = 0; i < n - 1; i++) {
			if (buffer[i] > buffer[i + 1]) {
				uint8_t tmp = buffer[i];

				buffer[i]     = buffer[i + 1];
				buffer[i + 1] = tmp;
			}
		}

		n--;
	} while (n > 1);
}


static uint8_t _getMedian(uint8_t *buffer, uint8_t bufferSize) {
	uint8_t ret = 0;

	do {
		if (bufferSize < 1) {
			break;
		}

		_bubbleSortInPlace(buffer, bufferSize);

		if (bufferSize % 2 == 0) {
			ret = (buffer[bufferSize / 2] + buffer[(bufferSize / 2) + 1]) / 2;

		} else {
			ret = buffer[bufferSize / 2];
		}

	} while (0);

	return ret;
}

#else
static uint8_t _average(uint8_t *buffer, uint8_t bufferSize) {
	uint16_t sum = 0;
	uint8_t i;

	for (i = 0; i < bufferSize; i++) {
		sum += buffer[i];
	}

	return (sum / bufferSize);
}
#endif


static uint8_t _getAdcValue(void) {
	uint8_t  ret = 0;

	_U8 buffer[ADC_BUFFER_SIZE];
	_U8 bufferWritten = 0;

	while (bufferWritten != ADC_BUFFER_SIZE) {
		ADCSRA |= _BV(ADSC);

		while (!(ADCSRA & _BV(ADIF)));

		buffer[bufferWritten++] = ADCH;

		ADCSRA |= _BV(ADIF);
	};

#if defined(ADC_MEDIAN_INSTEAD_OF_AVERAGE)
	ret = _getMedian(buffer, bufferWritten);
#else
	ret = _average(buffer, bufferWritten);
#endif

	return ret;
}


int main(void) {
	// Output direction for PWM OC0B. It is not
	// automatically set when timer starts
	PIO_SET_HIGH(  TIMER_BANK, TIMER_PIN);
	PIO_SET_OUTPUT(TIMER_BANK, TIMER_PIN);

	PIO_SET_OUTPUT(OK_BANK, OK_PIN);

	PIO_SET_HIGH(STBY_BANK, STBY_PIN);

#if defined(DEBUG) && DEBUG == 1
	debug_initialize();
#endif

	{
		// Prescaler 32, ~23 kSps (F_CPU / 32 / 13)
		ADCSRA = _BV(ADPS0) | _BV(ADPS2);

		// Only the highest 8 bits are used
		// VCC voltage reference
		ADMUX = _BV(ADLAR) | _BV(MUX1) | _BV(MUX0);      // ADC3 (PB3)

		// Disable Digital Input circuit for analog pins
		DIDR0 |= (_BV(ADC0D) | _BV(ADC1D) | _BV(ADC3D));

		// Eable ADC
		ADCSRA |= _BV(ADEN);
	}

	// Disable analog comparator
	ACSR |= _BV(ACD);

	{
		// Fast PWM - mode 7
		// set OC0B on Compare Match, clear OC0B at TOP
		TCCR0A = _BV(WGM00) | _BV(WGM01);
		TCCR0B = _BV(WGM02);

		OCR0A = TIMER_TOP;
		OCR0B = 0;
	}

	// Any logical change on STBY pin should trigger interrupt
	GIMSK |= _BV(PCIE);
	PCMSK |= _BV(PCINT4);

	timerStart(PIO_IS_LOW(STBY_BANK, STBY_PIN));

	sei();

	// Dummy read - first conversion is much longer
	_getAdcValue();

	while (1) {
		uint8_t voltageValue = 0;

		if (PIO_IS_HIGH(STBY_BANK, STBY_PIN)) {
#if !defined(DEBUG) || (defined(DEBUG) && DEBUG == 0)
			PIO_SET_LOW(OK_BANK, OK_PIN);
#endif

			continue;
		}

#if !defined(DEBUG) || (defined(DEBUG) && DEBUG == 0)
		PIO_SET_HIGH(OK_BANK, OK_PIN);
#endif

		voltageValue = _getAdcValue();

#if defined(DEBUG) && DEBUG == 1
		debug_sendByte(voltageValue);
		debug_sendByte(OCR0B);
#endif

		if (voltageValue > PWM_ADC_VOLTAGE_MIN - 3) {
			if (OCR0B < PWM_OCR_MAX) {
				if (OCR0B == 0) {
					timerStart(TRUE);
				}

				OCR0B++;
			}

		} else if (voltageValue < PWM_ADC_VOLTAGE_MIN + 3) {
			if (OCR0B > 0) {
				OCR0B--;

			} else {
				timerStart(FALSE);
			}
		}
	}

	return 0;
}
