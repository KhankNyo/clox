#ifndef _CLOX_SCANNER_H_
#define _CLOX_SCANNER_H_


#include "line.h"

typedef struct Scanner_t
{
    const char* start;
    const char* curr;
    uint32_t line;
} Scanner_t;


typedef struct Token_t
{
    TokenType_t type;
    const char* start;
    size_t len;
    uint32_t line;
} Token_t;

void Scanner_Init(Scanner_t* scanner, const char* src, size_t src_size);



#endif /* _CLOX_SCANNER_H_ */

