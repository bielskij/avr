/*
 * main.c
 *
 *  Created on: 25-11-2014
 *      Author: jarko
 */

#include <stdint.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <util/delay.h>

#if defined(DEBUG) && DEBUG == 1
#include "debug.h"
#endif

#define TIMER_TOP                     64

#define PWM_OCR_MAX                   46 // ~0.71
#define PWM_ADC_CURRENT_MAX          186 // 0,73V
#define PWM_ADC_VOLTAGE_MAX          254 //
#define PWM_ADC_CURRENT_TRESHOLD_MAX  15 // 86mA
#define PWM_ADC_CURRENT_TRESHOLD_MIN  10 // 76mA

#define ADC_PIN_CURRENT (_BV(MUX0) | _BV(MUX1)) // ADC3 (PB3)
#define ADC_PIN_VOLTAGE (_BV(MUX1))             // ADC2 (PB4)

#define ADC_BUFFER_SIZE 7

#define ADC_MEDIAN_INSTEAD_OF_AVERAGE


static volatile uint8_t adcBuffer[ADC_BUFFER_SIZE] = { 0 };
static volatile uint8_t adcBufferWritten           = 0;


ISR(ADC_vect) {
	adcBuffer[adcBufferWritten] = ADCH;
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

	for (i = 0; i < adcBufferWritten; i++) {
		sum += adcBuffer[i];
	}

	return (sum / adcBufferWritten);
}
#endif


static uint8_t _getAdcValue(uint8_t pin) {
	uint8_t  ret = 0;

	ADMUX &= ~(_BV(MUX0) | _BV(MUX1));

	ADMUX |= pin;

	adcBufferWritten = 0;

	ADCSRA |= _BV(ADIE);

	set_sleep_mode(SLEEP_MODE_IDLE);
	sleep_enable();

	while (adcBufferWritten != ADC_BUFFER_SIZE) {
		while (TCNT0 != TIMER_TOP) {}

		sleep_cpu();

		adcBufferWritten++;
	};

	sleep_disable();

#if defined(ADC_MEDIAN_INSTEAD_OF_AVERAGE)
	ret = _getMedian(adcBuffer, adcBufferWritten);
#else
	ret = _average(adcBuffer, adcBufferWritten);
#endif

	return ret;
}


void main() {
	// Output direction for PWM OC0B. It is not
	// automatically set when timer starts
	DDRB |= _BV(PIN1);

#if defined(DEBUG) && DEBUG == 1
	debug_initialize();

	debug_sendByte(0xff);
#endif

	{
		// Prescaler 64, ~5,7 kSps (F_CPU / 128 / 13)
		ADCSRA = _BV(ADPS0) | _BV(ADPS1) | _BV(ADPS2);

		// Only the highest 8 bits are used (PB3)
		// 1.1V internal voltage reference
		ADMUX = _BV(ADLAR) | _BV(REFS0);

		// Disable Digital Input circuit for analog pins
		DIDR0 |= (_BV(ADC0D) | _BV(ADC2D) | _BV(ADC3D));

		// Eable ADC
		ADCSRA |= _BV(ADEN);
	}

	_delay_ms(200);

	// Disable analog comparator
	ACSR |= _BV(ACD);

	{
		// Fast PWM - mode 7
		// Clear OC0B on Compare Match, set OC0B at TOP
		TCCR0A = _BV(WGM00) | _BV(WGM01) | _BV(COM0B1);
		TCCR0B = _BV(WGM02);

		OCR0A = TIMER_TOP;
		OCR0B = 0;

		// Start timer - no prescaling
		// In fast pwm mode frequency is ~147kHz
		TCCR0B |= _BV(CS00);
	}

	sei();

	// Dummy read - first conversion is much longer
	_getAdcValue(0);

	while (1) {
		uint8_t currentValue = _getAdcValue(ADC_PIN_CURRENT);
		uint8_t voltageValue = _getAdcValue(ADC_PIN_VOLTAGE);

#if defined(DEBUG) && DEBUG == 1
		debug_sendByte(currentValue);
		debug_sendByte(voltageValue);
		debug_sendByte(OCR0B);
#endif

		if (voltageValue >= PWM_ADC_VOLTAGE_MAX) {
			if (OCR0B >= 2) {
				OCR0B -= 2;

			} else {
				OCR0B = 0;
			}

			continue;
		}

		if (currentValue < PWM_ADC_CURRENT_MAX - PWM_ADC_CURRENT_TRESHOLD_MIN) {
			if (OCR0B < PWM_OCR_MAX) {
				OCR0B++;
			}

		} else if (currentValue > PWM_ADC_CURRENT_MAX + PWM_ADC_CURRENT_TRESHOLD_MAX) {
			if (OCR0B > 0) {
				OCR0B--;
			}
		}
	}

	return;
}
