#ifndef _CLOX_COMMON_H_
#define _CLOX_COMMON_H_



#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <inttypes.h>

#include <stdio.h>
#include <stdlib.h>



#define _STR_EXPR(expr) #expr
#define STR_EXPR(expr) _STR_EXPR(expr)

#ifdef BLAZINGLY_FAST
#  define NAN_BOXING
#endif /* BLAZINGLY_FAST */


#if defined(DEBUG)
#  define DEBUG_TRACE_EXECUTION
#  define CLOX_ASSERT(debug_expression)                 \
    do{                                                 \
        if (!(debug_expression))                        \
        {                                               \
            fprintf(stderr, "ASSERTION FAILED in "__FILE__", on line %d: %s", \
                __LINE__, STR_EXPR(debug_expression));\
            abort();                                    \
        }                                               \
    }while(0)

#  define DEBUG_ALLOCATION_CHK
#  define DEBUG_STRESS_GC

#  define DEBUG_PRINT_CODE
#  define DEBUG_LOG_GC
#  define DEBUG_LOG_ALLOC 

#  define GC_LOG_FILE stderr
#  define ALLOC_LOG_FILE stderr

#else
/* the expression is still there because it may contain code that have effect */
#  define CLOX_ASSERT(x) ((void)(x)) 
#endif /* DEBUG */



#define UINT8_COUNT (UINT8_MAX + 1)
#define STATIC_ARRSZ(comptime_array) (sizeof(comptime_array) / sizeof(comptime_array[0])) 


static inline void UNUSED2(int dummy, ...)
{
    (void)dummy;
}
#define UNUSED(...) UNUSED2(0, __VA_ARGS__)


#ifdef DEBUG_LOG_GC
#  define DEBUG_GC_PRINT(...) fprintf(GC_LOG_FILE, __VA_ARGS__)
#else
#  define DEBUG_GC_PRINT(...) UNUSED(__VA_ARGS__)
#endif /* DEBUG_LOG_GC */


#ifdef DEBUG_LOG_ALLOC
#  define DEBUG_ALLOC_PRINT(...) fprintf(ALLOC_LOG_FILE, __VA_ARGS__)
#else
#  define DEBUG_ALLOC_PRINT(...) UNUSED(__VA_ARGS__)
#endif /* DEBUG_LOG_ALLOC */



/* number of call frames */
#define VM_FRAMES_MAX 128



#endif /* _CLOX_COMMON_H_ */


