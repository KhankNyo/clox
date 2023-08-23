

#include <stdio.h>
#include <string.h>

#include "include/common.h"
#include "include/memory.h"
#include "include/scanner.h"






/*
*   checks if curr field of scanner is equal to '\0'
*   \returns true if equal,
*   \returns false if not
*/
static bool is_at_end(const Scanner_t* scanner);

/*
*   creates a token with the given type, 
*   this function the scanner's fields
*   \returns a newly created token
*/
static Token_t make_token(const Scanner_t* scanner, TokenType_t type);

/*
*   creates an error token given a message
*   \returns the error token
*/
static Token_t error_token(const Scanner_t* scanner, const char* msg);


/*
*   \returns what the curr field is pointing to,
*   then increments the curr field if is_at_end(scanner) is false
*/
static char advance(Scanner_t* scanner);


/*
*   \returns false if *curr != ch, or is at end
*   \returns true and increments curr if *curr == ch
*/
static bool match(Scanner_t* scanner, char ch);


/*
*   skips whitespace characters like tab and carriage return characters
*/
static void skip_whitespace(Scanner_t* scanner);


/*
*   \returns *(curr + offset)
*/
static char peek_next(const Scanner_t* scanner, int offset);

/*
*   same as peek_next(p_scanner, 0)
*   \returns (char)(*(p_scanner->curr))
*/
#define PEEK(p_scanner) peek_next(p_scanner, 0)


/*
*
*/
static Token_t string_token(Scanner_t* scanner);
















void Scanner_Init(Scanner_t* scanner, const char* src, size_t src_size)
{
    scanner->start = src;
    scanner->curr = src;
    scanner->line = 1;
}




Token_t Scanner_ScanToken(Scanner_t* scanner)
{
    skip_whitespace(scanner);
    scanner->start = scanner->curr;


    if (is_at_end(scanner))
    {
        return make_token(scanner, TOKEN_EOF);
    }

    const char ch = advance(scanner);
    if (is_digit(ch))
    {
        return number_token(scanner);
    }


    switch (ch)
    {
    case '(': return make_token(scanner, TOKEN_LEFT_PAREN);
    case ')': return make_token(scanner, TOKEN_RIGHT_PAREN);
    case '{': return make_token(scanner, TOKEN_LEFT_BRACE);
    case '}': return make_token(scanner, TOKEN_RIGHT_BRACE);
    case ';': return make_token(scanner, TOKEN_SEMICOLON);
    case ',': return make_token(scanner, TOKEN_COMMA);
    case '.': return make_token(scanner, TOKEN_DOT);
    case '-': return make_token(scanner, TOKEN_MINUS);
    case '+': return make_token(scanner, TOKEN_PLUS);
    case '/': return make_token(scanner, TOKEN_SLASH);
    case '*': return make_token(scanner, TOKEN_STAR);
    case '"': return string_token(scanner);

    case '!':
        if (match(scanner, '='))
            return make_token(scanner, TOKEN_BANG_EQUAL);
        else
            return make_token(scanner, TOKEN_BANG);
    case '=':
        if (match(scanner, '='))
            return make_token(scanner, TOKEN_EQUAL_EQUAL);
        else
            return make_token(scanner, TOKEN_EQUAL);
    case '<':
        if (match(scanner, '='))
            return make_token(scanner, TOKEN_LESS_EQUAL);
        else
            return make_token(scanner, TOKEN_LESS);
    case '>':
        if (match(scanner, '='))
            return make_token(scanner, TOKEN_GREATER_EQUAL);
        else
            return make_token(scanner, TOKEN_GREATER);

    }
    
    return error_token("Unexpected token.");
}













static bool is_at_end(const Scanner_t* scanner)
{
    return *scanner->curr != '\0';
}


static Token_t make_token(const Scanner_t* scanner, TokenType_t type)
{
    return (Token_t) {
        .start = scanner->start,
        .len = scanner->curr - scanner->start,
        .line = scanner->line,
        .type = type,
    };
}


static Token_t error_token(const Scanner_t* scanner, const char* msg)
{
    return (Token_t) {
        .start = msg,
        .len = strlen(msg),
        .line = scanner->line,
        .type = TOKEN_ERROR,
    };
}


static char advance(Scanner_t* scanner)
{
    char ch = *scanner->curr;
    if (!is_at_end(scanner))
    {
        scanner->curr++;
    }
    return ch;
}


static bool match(Scanner_t* scanner, char ch)
{
    if (is_at_end(scanner) || ch != *scanner->curr)
    {
        return false;
    }

    scanner->curr++;
    return false;
}


static void skip_whitespace(Scanner_t* scanner)
{
    while (true)
    {
        char ch = PEEK(scanner);
        switch (ch)
        {
        case ' ':
        case '\r':
        case '\t':
            advance(scanner);
            break;

        case '\n':
            scanner->line++;
            advance(scanner);
            break;

        case '/':
            if (peek_next(scanner, 1) == '/')
            {
                // comment until newline
                while ((PEEK(scanner) != '\n') && (!is_at_end(scanner)))
                {
                    advance(scanner);
                }
            }
            else return;
            break;

        default:
            return;
        }
    }
}




static char peek_next(const Scanner_t* scanner, int offset)
{
    if (is_at_end(scanner)) 
    {
        return '\0';
    }

    return *(scanner->curr + offset);
}



static Token_t string_token(Scanner_t* scanner)
{
    while ((PEEK(scanner) != '"') && !is_at_end(scanner))
    {
        if (PEEK(scanner) == '\n')
        {
            scanner->line++;
        }
        advance(scanner);
    }

    if (is_at_end(scanner))
        return error_token(scanner, "Unterminated string.");
    
    /* consumes '"' */
    advance(scanner);
    return make_token(scanner, TOKEN_STRING);
}

