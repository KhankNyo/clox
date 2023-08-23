

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
*	\returns a string literal token
*/
static Token_t string_token(Scanner_t* scanner);


/*
*   \returns a numnber literal token
*/
static Token_t number_token(Scanner_t* scanner);


/*  
*   \returns true if ch is a digit, false otherwise
*/
static bool is_digit(char ch);


/*
*   \returns true if ch is a alphabetical character or '_', false otherwise
*/
static bool is_alpha(char ch);


/*
*   \returns a token that is either some kind of keyword or an identifier
*/
static Token_t identifier_token(Scanner_t* scanner);



/*
*   \returns the type of the token we're at
*/
static TokenType_t chk_iden_type(Scanner_t* scanner);


/*
*   \returns the given type param if the current lexeme matches rest
*   \returns else return TOKEN_IDENTIFIER
*/
static TokenType_t chk_keyword(const Scanner_t* scanner, 
    int head_offset, int len, 
    const char* rest, TokenType_t type
);









void Scanner_Init(Scanner_t* scanner, const char* src)
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
    if (is_alpha(ch))
    {
        return identifier_token(scanner);
    }
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

    case '"': return string_token(scanner);
    }
    
    return error_token(scanner, "Unexpected token.");
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
    {
        return error_token(scanner, "Unterminated string.");
    }
    
    /* consumes '"' */
    advance(scanner);
    return make_token(scanner, TOKEN_STRING);
}


static Token_t number_token(Scanner_t* scanner)
{
    while (is_digit(PEEK(scanner)))
    {
        advance(scanner);
    }
    if ('.' != PEEK(scanner))
        return make_token(scanner, TOKEN_NUMBER);

    /* skips decimal separator */
    advance(scanner);

    /* eat the rest of the number */
    while (is_digit(PEEK(scanner)))
    {
        advance(scanner);
    }
    return make_token(scanner, TOKEN_NUMBER);
}


static bool is_digit(char ch)
{
    return ('0' <= ch) && (ch <= '9');
}



static bool is_alpha(char ch)
{
    return (('a' <= ch) && (ch <= 'z'))
        || (('A' <= ch) && (ch <= 'Z'))
        || ('_' == ch);
}




static Token_t identifier_token(Scanner_t* scanner)
{
    /* consumes the identifier */
    while (is_alpha(PEEK(scanner)) || is_digit(PEEK(scanner)))
    {
        advance(scanner);
    }
    return make_token(scanner, chk_iden_type(scanner));
}




static TokenType_t chk_iden_type(Scanner_t* scanner)
{
    switch (scanner->start[0]) 
    {
        case 'a': return chk_keyword(scanner, 1, 2, "nd", TOKEN_AND);
        case 'c': return chk_keyword(scanner, 1, 4, "lass", TOKEN_CLASS);
        case 'e': return chk_keyword(scanner, 1, 3, "lse", TOKEN_ELSE);
        case 'i': return chk_keyword(scanner, 1, 1, "f", TOKEN_IF);
        case 'n': return chk_keyword(scanner, 1, 2, "il", TOKEN_NIL);
        case 'o': return chk_keyword(scanner, 1, 1, "r", TOKEN_OR);
        case 'p': return chk_keyword(scanner, 1, 4, "rint", TOKEN_PRINT);
        case 'r': return chk_keyword(scanner, 1, 5, "eturn", TOKEN_RETURN);
        case 's': return chk_keyword(scanner, 1, 4, "uper", TOKEN_SUPER);
        case 'v': return chk_keyword(scanner, 1, 2, "ar", TOKEN_VAR);
        case 'w': return chk_keyword(scanner, 1, 4, "hile", TOKEN_WHILE);
        case 'f':
            if (scanner->curr - scanner->start > 1) /* not just 'f' */
            {
                switch (scanner->start[1]) 
                {
                    case 'a': return chk_keyword(scanner, 2, 3, "lse", TOKEN_FALSE);
                    case 'o': return chk_keyword(scanner, 2, 1, "r", TOKEN_FOR);
                    case 'u': return chk_keyword(scanner, 2, 1, "n", TOKEN_FUN);
                }
            }
            break;
        case 't':
            if (scanner->curr - scanner->start > 1) /* not just 't' */
            {
                switch (scanner->start[1]) 
                {
                    case 'h': return chk_keyword(scanner, 2, 2, "is", TOKEN_THIS);
                    case 'r': return chk_keyword(scanner, 2, 2, "ue", TOKEN_TRUE);
                }
            }
            break;
    }
    return TOKEN_IDENTIFIER;
}




static TokenType_t chk_keyword(const Scanner_t* scanner,
    int start, int len,
    const char* rest, TokenType_t type
)
{
    const size_t lexeme_len = scanner->curr - scanner->start;
    const size_t keyword_len = start + strlen(rest);
    if ((lexeme_len == keyword_len)
    && memcmp(scanner->start + start, rest, len) == 0)
    {
        return type;
    }
    return TOKEN_IDENTIFIER;
}

