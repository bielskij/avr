#ifndef COMMON_TYPES_H_
#define COMMON_TYPES_H_

#include <stdint.h>

typedef int8_t   _S8;
typedef uint8_t  _U8;
typedef signed short int   _S16;
typedef unsigned short int _U16;
typedef unsigned long int  _U32;
typedef signed long int    _S32;

typedef _U8 _BOOL;

#define TRUE  (1 == 1)
#define FALSE (!TRUE)

#endif /* COMMON_TYPES_H_ */
