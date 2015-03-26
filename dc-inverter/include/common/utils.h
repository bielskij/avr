#ifndef COMMON_UTILS_H_
#define COMMON_UTILS_H_

#include <avr/io.h>

#define BIT_SET_AT(word, bitNum)   { word |= (1 << bitNum); }
#define BIT_CLEAR_AT(word, bitNum) { word &= ~(1 << bitNum); }

#define BIT_SET_MASK(word, mask)   { word |= mask; }
#define BIT_CLEAR_MASK(word, mask) { word &= ~(mask); }

#define CONCAT_MACRO(port, letter) port ## letter
#define DECLARE_PORT(port) CONCAT_MACRO(PORT, port)
#define DECLARE_DDR(port)  CONCAT_MACRO(DDR,  port)
#define DECLARE_PIN(port)  CONCAT_MACRO(PIN,  port)

#define PIO_SET_HIGH(bank, pin) { DECLARE_PORT(bank) |= _BV(DECLARE_PIN(pin)); }
#define PIO_SET_LOW(bank, pin)  { DECLARE_PORT(bank) &= ~_BV(DECLARE_PIN(pin)); }

#define PIO_TOGGLE(bank, pin) { DECLARE_PORT(bank) ^= _BV(DECLARE_PIN(pin)); }

#define PIO_IS_HIGH(bank, pin) (bit_is_set(DECLARE_PIN(bank), DECLARE_PIN(pin)))
#define PIO_IS_LOW(bank, pin) (! bit_is_set(DECLARE_PIN(bank), DECLARE_PIN(pin)))

#define PIO_SET_OUTPUT(bank, pin) { DECLARE_DDR(bank) |= _BV(DECLARE_PIN(pin)); }
#define PIO_SET_INPUT(bank, pin)  { DECLARE_DDR(bank) &= ~(_BV(DECLARE_PIN(pin))); }

#define TABLE_SIZE(x) (sizeof(x) / sizeof(*x))

#endif /* COMMON_UTILS_H_ */
