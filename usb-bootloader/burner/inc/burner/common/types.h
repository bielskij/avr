#ifndef BURNER_COMMON_TYPES_H_
#define BURNER_COMMON_TYPES_H_

#include <stdint.h>
#include <sys/types.h>

typedef int8_t   _S8;
typedef uint8_t  _U8;
typedef int16_t  _S16;
typedef uint16_t _U16;
typedef int32_t  _S32;
typedef uint32_t _U32;

typedef _U8 _BOOL;

#ifndef TRUE
	#define TRUE (1 == 1)
#endif

#ifndef FALSE
	#define FALSE (!TRUE)
#endif

typedef enum _CommonError {
	COMMON_NO_ERROR,
	COMMON_ERROR,
	COMMON_ERROR_TIMEOUT,
	COMMON_ERROR_BAD_PARAMETER,
	COMMON_ERROR_NO_FREE_RESOURCES,
	COMMON_ERROR_NO_DEVICE
} CommonError;

#endif /* BURNER_COMMON_TYPES_H_ */
