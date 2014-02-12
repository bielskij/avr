#ifndef COMMON_UTILS_H_
#define COMMON_UTILS_H_

#define ONE_LEFT_SHIFTED(a) (1 << a)

#define SET_BIT_AT(word, bitNum)   { word |= (1 << bitNum); }
#define CLEAR_BIT_AT(word, bitNum) { word &= ~(1 << bitNum); }
#define CHECK_BIT_AT(word, bitNum) (word & ONE_LEFT_SHIFTED(bitNum))

#define SET_BIT_MASK(word, mask)   { word |= mask; }
#define CLEAR_BIT_MASK(word, mask) { word &= ~(mask); }

#define SET_PIO_HIGH(port, pin) { port |= ONE_LEFT_SHIFTED(pin); }
#define SET_PIO_LOW(port, pin)  { port &= ~ONE_LEFT_SHIFTED(pin); }

#define PIO_TOGGLE(port, pin) { port ^= ONE_LEFT_SHIFTED(pin); }

#define PIO_IS_HIGH(pin, pinno) (CHECK_BIT_AT(pin, pinno))
#define PIO_IS_LOW(pin, pinno) (! CHECK_BIT_AT(pin, pinno))

#define SET_PIO_AS_OUTPUT(ddr, pin) { ddr |= ONE_LEFT_SHIFTED(pin); }
#define SET_PIO_AS_INPUT(ddr, pin)  { ddr &= ~(ONE_LEFT_SHIFTED(pin)); }

#define CONCAT_MACRO(port, letter) port ## letter
#define DECLARE_PORT(port) CONCAT_MACRO(PORT, port)
#define DECLARE_DDR(port)  CONCAT_MACRO(DDR,  port)
#define DECLARE_PIN(port)  CONCAT_MACRO(PIN,  port)

#define TABLE_SIZE(x) (sizeof(x) / sizeof(*x))

#endif /* COMMON_UTILS_H_ */
