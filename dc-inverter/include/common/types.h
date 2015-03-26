#ifndef COMMON_TYPES_H_
#define COMMON_TYPES_H_

#define _inline_  __inline

#define _asm_ __asm

#define _nop_ _asm_("nop"::);


#if !defined(TRUE)
	#define TRUE  (1 == 1)
#endif

#if !defined(FALSE)
	#define FALSE (! TRUE)
#endif

#if !defined(NULL)
	#define NULL ((void *) 0)
#endif

typedef signed char         _S8;
typedef unsigned char       _U8;
typedef signed short int   _S16;
typedef unsigned short int _U16;
typedef unsigned long int  _U32;
typedef signed long int    _S32;

typedef _U8 _BOOL;


#endif /* COMMON_TYPES_H_ */
