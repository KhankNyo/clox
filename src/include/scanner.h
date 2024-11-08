#ifndef _CLOX_SCANNER_H_
#define _CLOX_SCANNER_H_


#include "line.h"



typedef struct Scanner_t
{
    const char* start;
    const char* curr;
    line_t line;
} Scanner_t;


typedef enum TokenType_t
{
    // Single-character tokens.
    TOKEN_LEFT_PAREN, TOKEN_RIGHT_PAREN,
    TOKEN_LEFT_BRACE, TOKEN_RIGHT_BRACE,
    TOKEN_COMMA, TOKEN_DOT, TOKEN_MINUS, TOKEN_PLUS,
    TOKEN_SEMICOLON, TOKEN_SLASH, TOKEN_STAR,

    // One or two character tokens.
    TOKEN_BANG, TOKEN_BANG_EQUAL,
    TOKEN_EQUAL, TOKEN_EQUAL_EQUAL,
    TOKEN_GREATER, TOKEN_GREATER_EQUAL,
    TOKEN_LESS, TOKEN_LESS_EQUAL,

    // Literals.
    TOKEN_IDENTIFIER, TOKEN_STRING, TOKEN_NUMBER,

    // Keywords.
    TOKEN_AND, TOKEN_CLASS, TOKEN_ELSE, TOKEN_FALSE,
    TOKEN_FOR, TOKEN_FUN, TOKEN_IF, TOKEN_NIL, TOKEN_OR,
    TOKEN_PRINT, TOKEN_RETURN, TOKEN_SUPER, TOKEN_THIS,
    TOKEN_TRUE, TOKEN_VAR, TOKEN_WHILE,

    TOKEN_ERROR, TOKEN_EOF,

    // extensions
    TOKEN_STAR_STAR,
    TOKEN_PLUS_EQUAL, TOKEN_MINUS_EQUAL,
    TOKEN_SLASH_EQUAL, TOKEN_STAR_EQUAL,
    TOKEN_LEFT_BRACKET, TOKEN_RIGHT_BRACKET,
    TOKEN_TYPE_COUNT,
} TokenType_t;



typedef struct Token_t
{
    const char* start;
    size_t len;
    TokenType_t type;
    line_t line;
} Token_t;



/*
*   initializes scanner
*/
void Scanner_Init(Scanner_t* scanner, const char* src);

/* 
*   the scanner itself has no resource that need to be destroyed
*/



/*
*   scans a single token at a time
*   \returns the scanned token
*/
Token_t Scanner_ScanToken(Scanner_t* scanner);


#endif /* _CLOX_SCANNER_H_ */

