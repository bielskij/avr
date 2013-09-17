#ifndef BURNER_COMMON_DEBUG_H_
#define BURNER_COMMON_DEBUG_H_

#include <stdio.h>
#include <stdlib.h>

#define DEBUG_LEVEL_FATAL 0
#define DEBUG_LEVEL_ERR   1
#define DEBUG_LEVEL_WARN  2
#define DEBUG_LEVEL_LOG   3
#define DEBUG_LEVEL_DBG   4

#if !defined(DEBUG_LEVEL)
	#define DEBUG_LEVEL DEBUG_LEVEL_FATAL
#endif

#ifdef ENABLE_DEBUG
	#define FATAL(x)  { printf("[FATAL <%s:%d>]: ", __FILE__, __LINE__); printf x; printf("\n"); }
#else
	#define FATAL(x)  {}
#endif

#if defined(ENABLE_DEBUG) && (DEBUG_LEVEL >= DEBUG_LEVEL_ERR)
	#define ERR(x)    { printf("[ERR <%s:%d>]: ",   __FILE__, __LINE__); printf x; printf("\n"); }
#else
	#define ERR(x)    {}
#endif

#if defined(ENABLE_DEBUG) && (DEBUG_LEVEL >= DEBUG_LEVEL_WARN)
	#define WARN(x)   { printf("[WARN <%s:%d>]: ",  __FILE__, __LINE__); printf x; printf("\n"); }
#else
	#define WARN(x)   {}
#endif

#if defined(ENABLE_DEBUG) && (DEBUG_LEVEL >= DEBUG_LEVEL_LOG)
	#define LOG(x)    { printf("[LOG <%s:%d>]: ",   __FILE__, __LINE__); printf x; printf("\n"); }
#else
	#define LOG(x)    {}
#endif

#if defined(ENABLE_DEBUG) && (DEBUG_LEVEL >= DEBUG_LEVEL_DBG)
	#define DBG(x)    { printf("[DBG <%s:%d>]: ",   __FILE__, __LINE__); printf x; printf("\n"); }
#else
	#define DBG(x)    {}
#endif

#define REPORT(x) { printf x; printf("\n"); }
#define REPORT_ERR(x) { printf("!!!! ERROR !!!!"); printf x; printf(" !!!!\n"); }

#if defined(ENABLE_DEBUG)
#define ASSERT(x) { if ((x) != TRUE) { FATAL(("ASSERT!!! ("#x") != TRUE")); abort(); } }
#else
#define ASSERT(x) {}
#endif

#endif /* BURNER_COMMON_DEBUG_H_ */
