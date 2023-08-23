#ifndef _CLOX_SCANNER_H_
#define _CLOX_SCANNER_H_


#include "line.h"



typedef struct Scanner_t
{
    const char* start;
    const char* curr;
    line_t line;
} Scanner_t;


typedef struct Token_t
{
    TokenType_t type;
    const char* start;
    size_t len;
    line_t line;
} Token_t;

void Scanner_Init(Scanner_t* scanner, const char* src, size_t src_size);

Token_t Scanner_ScanToken(Scanner_t* scanner);


#endif /* _CLOX_SCANNER_H_ */

