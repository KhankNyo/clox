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

#ifdef DEBUG
#  define DEBUG_TRACE_EXECUTION
#  define CLOX_ASSERT(debug_expression)                 \
    do{                                                 \
        if (!(debug_expression))                        \
        {                                               \
            fprintf(stderr, "ASSERTION FAILED: file: %s, on line %d: %s", \
                __FILE__, __LINE__, STR_EXPR(debug_expression));\
            abort();                                    \
        }                                               \
    }while(0)

#define DEBUG_PRINT_CODE

#else
/* the expression is still there because it may contain code that have effect */
#  define CLOX_ASSERT(x) ((void)(x)) 
#endif /* DEBUG */


#define UINT8_COUNT (UINT8_MAX + 1)

#define STATIC_ARRSZ(comptime_array) (sizeof(comptime_array) / sizeof(comptime_array[0])) 



#endif /* _CLOX_COMMON_H_ */


