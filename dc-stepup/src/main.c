/*
 * main.c
 *
 *  Created on: 25-11-2014
 *      Author: jarko
 */

#include <stdint.h>
#include <avr/io.h>
#include <avr/interrupt.h>


#define PWM_OCR_MAX                   46 // ~0.71
#define PWM_ADC_CURRENT_MAX          186 // 0,73V
#define PWM_ADC_VOLTAGE_MAX          254 //
#define PWM_ADC_CURRENT_TRESHOLD_MAX  15 // 84mA
#define PWM_ADC_CURRENT_TRESHOLD_MIN   5 // 78mA

#define ADC_PIN_CURRENT (_BV(MUX0) | _BV(MUX1)) // ADC3 (PB3)
#define ADC_PIN_VOLTAGE (_BV(MUX1))             // ADC2 (PB4)


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


static uint8_t _getAdcValue(uint8_t pin) {
	uint8_t  ret = 0;
	uint8_t  i   = 16;
	uint8_t  buffer[16];

	ADMUX &= ~(_BV(MUX0) | _BV(MUX1));

	while (i--) {
		ADMUX |= pin;

		// Start first conversion
		ADCSRA |= _BV(ADSC);

		while (bit_is_set(ADCSRA, ADSC));

		buffer[i] = ADCH;

		ADCSRA &= ~_BV(ADIF);
	}

	ret = _getMedian(buffer, sizeof(buffer));

	return ret;
}


void main() {
	// Output direction for PWM OC0B. It is not
	// automatically set when timer starts
	DDRB |= _BV(PIN1);

#if defined(DEBUG) && DEBUG == 1
	DDRB |= _BV(PIN0);
#endif

	{
		// Eable ADC
		// Prescaler 64, ~11,1 kSps (F_CPU / 64 / 13)
		ADCSRA = _BV(ADEN) | _BV(ADPS1) | _BV(ADPS2);

		// Only the highest 8 bits are used (PB3)
		// 1.1V internal voltage reference
		ADMUX = _BV(ADLAR) | _BV(REFS0);
	}

	{
		// Fast PWM - mode 7
		// Clear OC0B on Compare Match, set OC0B at TOP
		TCCR0A = _BV(WGM00) | _BV(WGM01) | _BV(COM0B1);
		TCCR0B = _BV(WGM02);

		OCR0A = 64;
		OCR0B = 1;

		// Start timer - no prescaling
		// In fast pwm mode frequency is ~147kHz
		TCCR0B |= _BV(CS00);
	}

	// Dummy read - first conversion is much longer
	_getAdcValue(0);

	while (1) {
		uint8_t currentValue = _getAdcValue(ADC_PIN_CURRENT);
		uint8_t voltageValue = _getAdcValue(ADC_PIN_VOLTAGE);

		if (voltageValue >= PWM_ADC_VOLTAGE_MAX) {
			if (OCR0B >= 2) {
				OCR0B -= 2;
			}

			continue;
		}

		if (currentValue < PWM_ADC_CURRENT_MAX - PWM_ADC_CURRENT_TRESHOLD_MIN) {
			if (OCR0B < PWM_OCR_MAX) {
				OCR0B++;

#if defined(DEBUG) && DEBUG == 1
				PORTB ^= _BV(PIN0);
#endif
			}

		} else if (currentValue > PWM_ADC_CURRENT_MAX + PWM_ADC_CURRENT_TRESHOLD_MAX) {
			if (OCR0B > 0) {
				OCR0B--;

#if defined(DEBUG) && DEBUG == 1
				PORTB ^= _BV(PIN0);
#endif
			}
		}
	}

	return;
}
