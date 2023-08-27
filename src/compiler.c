

#include <stdarg.h>

#include "include/common.h"
#include "include/memory.h"
#include "include/scanner.h"
#include "include/chunk.h"
#include "include/compiler.h"
#include "include/debug.h"
#include "include/object.h"


typedef struct Parser_t
{
    Token_t prev;
    Token_t curr;
    bool had_error;
    bool panic_mode;
} Parser_t;


typedef enum Precedence_t {
    PREC_NONE = 0,
    PREC_ASSIGNMENT,  // =
    PREC_OR,          // or
    PREC_AND,         // and
    PREC_EQUALITY,    // == !=
    PREC_COMPARISON,  // < > <= >=
    PREC_TERM,        // + -
    PREC_FACTOR,      // * /
    PREC_UNARY,       // ! -
    PREC_CALL,        // . ()
    PREC_PRIMARY,
} Precedence_t;




typedef struct Compiler_t
{
    Scanner_t scanner;
    Parser_t parser;
    Chunk_t* chunk;
    VMData_t* vmdata;
} Compiler_t;


typedef void (*ParseFn_t)(Compiler_t*);
typedef struct ParseRule_t
{
    ParseFn_t prefix;
    ParseFn_t infix;
    Precedence_t precedence;
} ParseRule_t;









static void compiler_init(Compiler_t* compiler, VMData_t* data, const char* src, Chunk_t* chunk);
static void compiler_end(Compiler_t* compiler);


/* Pratt parser */
static void parse_precedence(Compiler_t* compiler, Precedence_t prec);

/* \returns &s_rules[operator] */
static const ParseRule_t* get_parse_rule(TokenType_t operator);

/* parses an expression */
static void expression(Compiler_t* compiler);


static void string(Compiler_t* copmiler);
static void literal(Compiler_t* compiler);
static void number(Compiler_t* compiler);
static void grouping(Compiler_t* compiler);
static void unary(Compiler_t* compiler);
static void binary(Compiler_t* compiler);






/* emits a byte into the current chunk */
static void emit_byte(Compiler_t* compiler, uint8_t byte);

/* emits 2 byte into the chunk */
static void emit_2_bytes(Compiler_t* compiler, uint8_t byte1, uint8_t byte2);

/* emits num_bytes bytes into the chunk */
static void emit_bytes(Compiler_t* compile, size_t num_bytes, ...);





static void emit_return(Compiler_t* compiler);
static void emit_constant(Compiler_t* compiler, Value_t val);



/* \returns the current compiling chunk */
static Chunk_t* current_chunk(Compiler_t* compiler);

/* query the scanner for the next token and put it into the parser */
static void advance(Compiler_t* compiler);


/* consumes the next token, 
*   if the next token is not the expected type,
*       report an error with the given errmsg
*   else errmsg is unused
*/
static void consume(Compiler_t* compiler, TokenType_t expected_type, const char* errmsg);



/* report an error at the parser's curr token */
static void error_at_current(Parser_t* parser, const char* msg);

/* report an error at the parser's prev token */
static void error(Parser_t* parser, const char* msg);

/* helper function used by error_at_current and error */
static void error_at(Parser_t* parser, const Token_t* token, const char* msg);








static const ParseRule_t s_rules[] = 
{
  [TOKEN_LEFT_PAREN]    = {grouping, NULL,   PREC_NONE},
  [TOKEN_RIGHT_PAREN]   = {NULL,     NULL,   PREC_NONE},
  [TOKEN_LEFT_BRACE]    = {NULL,     NULL,   PREC_NONE}, 
  [TOKEN_RIGHT_BRACE]   = {NULL,     NULL,   PREC_NONE},
  [TOKEN_COMMA]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_DOT]           = {NULL,     NULL,   PREC_NONE},
  [TOKEN_MINUS]         = {unary,    binary, PREC_TERM},
  [TOKEN_PLUS]          = {unary,    binary, PREC_TERM},
  [TOKEN_SEMICOLON]     = {NULL,     NULL,   PREC_NONE},
  [TOKEN_SLASH]         = {NULL,     binary, PREC_FACTOR},
  [TOKEN_STAR]          = {NULL,     binary, PREC_FACTOR},
  [TOKEN_BANG]          = {unary,    NULL,   PREC_NONE},
  [TOKEN_EQUAL_EQUAL]   = {NULL,     binary, PREC_EQUALITY},
  [TOKEN_EQUAL]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_BANG_EQUAL]    = {NULL,     binary, PREC_EQUALITY},
  [TOKEN_GREATER]       = {NULL,     binary, PREC_COMPARISON},
  [TOKEN_GREATER_EQUAL] = {NULL,     binary, PREC_COMPARISON},
  [TOKEN_LESS]          = {NULL,     binary, PREC_COMPARISON},
  [TOKEN_LESS_EQUAL]    = {NULL,     binary, PREC_COMPARISON},
  [TOKEN_IDENTIFIER]    = {NULL,     NULL,   PREC_NONE},
  [TOKEN_STRING]        = {string,   NULL,   PREC_NONE},
  [TOKEN_NUMBER]        = {number,   NULL,   PREC_NONE},
  [TOKEN_AND]           = {NULL,     NULL,   PREC_NONE},
  [TOKEN_CLASS]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_ELSE]          = {NULL,     NULL,   PREC_NONE},
  [TOKEN_FALSE]         = {literal,  NULL,   PREC_NONE},
  [TOKEN_FOR]           = {NULL,     NULL,   PREC_NONE},
  [TOKEN_FUN]           = {NULL,     NULL,   PREC_NONE},
  [TOKEN_IF]            = {NULL,     NULL,   PREC_NONE},
  [TOKEN_NIL]           = {literal,  NULL,   PREC_NONE},
  [TOKEN_OR]            = {NULL,     NULL,   PREC_NONE},
  [TOKEN_PRINT]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_RETURN]        = {NULL,     NULL,   PREC_NONE},
  [TOKEN_SUPER]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_THIS]          = {NULL,     NULL,   PREC_NONE},
  [TOKEN_TRUE]          = {literal,  NULL,   PREC_NONE},
  [TOKEN_VAR]           = {NULL,     NULL,   PREC_NONE},
  [TOKEN_WHILE]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_ERROR]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_EOF]           = {NULL,     NULL,   PREC_NONE},
};




















bool Compile(VMData_t* data, const char* src, Chunk_t* chunk)
{
    Compiler_t compiler;
    compiler_init(&compiler, data, src, chunk);


    advance(&compiler);
    expression(&compiler);
    consume(&compiler, TOKEN_EOF, "Expected end of expression.");

    const bool error = compiler.parser.had_error;
    compiler_end(&compiler);
    return !error;
}

















static void compiler_init(Compiler_t* compiler, VMData_t* data, const char* src, Chunk_t* chunk)
{
    Scanner_Init(&compiler->scanner, src);
    compiler->parser.had_error = false;
    compiler->parser.panic_mode = false;
    compiler->chunk = chunk;
    compiler->vmdata = data;
}



static void compiler_end(Compiler_t* compiler)
{
    emit_return(compiler);
#ifdef DEBUG_PRINT_CODE
    if (!compiler->parser.had_error)
    {
        Disasm_Chunk(stderr, current_chunk(compiler), "debug");
    }
#endif /* DEBUG_PRINT_CODE */
}









static void parse_precedence(Compiler_t* compiler, Precedence_t prec)
{
    CLOX_ASSERT(prec != PREC_NONE);

    advance(compiler);
    ParseFn_t prefix_parser = get_parse_rule(compiler->parser.prev.type)->prefix;
    if (NULL == prefix_parser)
    {
        /* 
        *   if parse_precedence is called, but there are no prefix parser for it,
        *   then the token that caused the call to this function must've been a syntax error,
        *   report the error.
        */
        error(&compiler->parser, "Expected expression.");
        return;
    }

    /* parses prefix operator */
    /* NOTE: prefix operator for a binary operator is its first argument (left operand) */
    prefix_parser(compiler);


    /* 
    *   The loop will exit when it encounters a token that 
    *   has lower precedence than the current operator
    * 
    *   if a token IS an infix operator, it will have a value other than PREC_NONE,
    *       which means that infix_parser variable inside the loop will never be NULL
    *   if a token IS NOT an infix operator, its precedence WILL BE PREC_NONE,
    *       assuming prec != PREC_NONE, this loop will never be executed
    */
    while (prec <= get_parse_rule(compiler->parser.curr.type)->precedence)
    {
        advance(compiler);
        ParseFn_t infix_parser = get_parse_rule(compiler->parser.prev.type)->infix;
        CLOX_ASSERT(NULL != infix_parser);
        infix_parser(compiler);
    }
}



static const ParseRule_t* get_parse_rule(TokenType_t type)
{
    return &s_rules[type];
}




static void expression(Compiler_t* compiler)
{
    parse_precedence(compiler, PREC_ASSIGNMENT);
}








static void string(Compiler_t* compiler)
{
    emit_constant(compiler, 
        OBJ_VAL(ObjStr_Copy(compiler->vmdata, 
            compiler->parser.prev.start + 1, compiler->parser.prev.len - 2))
    );
}


static void literal(Compiler_t* compiler)
{
    switch (compiler->parser.prev.type)
    {
    case TOKEN_TRUE:    emit_byte(compiler, OP_TRUE); break;
    case TOKEN_FALSE:   emit_byte(compiler, OP_FALSE); break;
    case TOKEN_NIL:     emit_byte(compiler, OP_NIL); break;
    default: CLOX_ASSERT(false && "Unhandled literal type."); return;
    }
}


static void number(Compiler_t* compiler)
{
    const Value_t val = NUMBER_VAL(strtod(compiler->parser.prev.start, NULL));
    emit_constant(compiler, val);
}


static void grouping(Compiler_t* compiler)
{
    /* assumes that '(' is already consumed */
    expression(compiler);
    consume(compiler, TOKEN_RIGHT_PAREN, "Expected ')' after expression.");
}


static void unary(Compiler_t* compiler)
{
    const TokenType_t operator = compiler->parser.prev.type;

    /* compiles first operand */
    parse_precedence(compiler, PREC_UNARY);


    /* emit op that operates on whatever expression() put on the vm's stack */
    switch (operator)
    {
    case TOKEN_BANG:    emit_byte(compiler, OP_NOT); break;
    case TOKEN_MINUS:   emit_byte(compiler, OP_NEGATE); break;
    case TOKEN_PLUS:    /* nop */ break;

    default: CLOX_ASSERT(false && "Unhandled unary operator"); return;
    }
}


static void binary(Compiler_t* compiler)
{
    const TokenType_t operator = compiler->parser.prev.type;
    const ParseRule_t* rule = get_parse_rule(operator);
    /* 
    *   because most binary operators are left associative, 
    *   the next operator does not have equal precedence to the current operator
    */
    /* TODO: assignment operator */
    parse_precedence(compiler, (Precedence_t)(rule->precedence + 1));


    switch (operator)
    {
    case TOKEN_PLUS:            emit_byte(compiler, OP_ADD); break;
    case TOKEN_MINUS:           emit_byte(compiler, OP_SUBTRACT); break;
    case TOKEN_STAR:            emit_byte(compiler, OP_MULTIPLY); break;
    case TOKEN_SLASH:           emit_byte(compiler, OP_DIVIDE); break;

    case TOKEN_GREATER:         emit_byte(compiler, OP_GREATER); break;
    case TOKEN_GREATER_EQUAL:   emit_2_bytes(compiler, OP_LESS, OP_NOT); break;

    case TOKEN_LESS:            emit_byte(compiler, OP_LESS); break;
    case TOKEN_LESS_EQUAL:      emit_2_bytes(compiler, OP_GREATER, OP_NOT); break;

    case TOKEN_EQUAL_EQUAL:     emit_byte(compiler, OP_EQUAL); break;
    case TOKEN_BANG_EQUAL:      emit_2_bytes(compiler, OP_EQUAL, OP_NOT); break;
    default: CLOX_ASSERT(false && "Unhandled binary operator type"); return;
    }
}








static void emit_byte(Compiler_t* compiler, uint8_t byte)
{
    Chunk_Write(current_chunk(compiler), byte, compiler->parser.prev.line);
}


static void emit_2_bytes(Compiler_t* compiler, uint8_t byte1, uint8_t byte2)
{
    emit_byte(compiler, byte1);
    emit_byte(compiler, byte2);
}

static void emit_bytes(Compiler_t* compiler, size_t num_bytes, ...)
{
    va_list args;
    va_start(args, num_bytes);

    for (size_t i = 0; i < num_bytes; i++)
    {
        const uint8_t byte = va_arg(args, uint8_t);
        emit_byte(compiler, byte);
    }

    va_end(args);
}





static void emit_return(Compiler_t* compiler)
{
    emit_byte(compiler, OP_RETURN);
}

static void emit_constant(Compiler_t* compiler, Value_t val)
{
    size_t val_addr = Chunk_WriteConstant(current_chunk(compiler), val, compiler->parser.prev.line);
    if (val_addr > MAX_CONST_IN_CHUNK)
    {
        error(&compiler->parser, "Too many constants in one chunk.");
    }
}






static Chunk_t* current_chunk(Compiler_t* compiler)
{
    return compiler->chunk;
}






static void advance(Compiler_t* compiler)
{
    Parser_t* parser = &compiler->parser; /* ease typing */
    parser->prev = parser->curr;


    do {
        parser->curr = Scanner_ScanToken(&compiler->scanner);
        error_at_current(parser, parser->curr.start);
    } while (TOKEN_ERROR == parser->curr.type);
}


static void consume(Compiler_t* compiler, TokenType_t expected_type, const char* errmsg)
{
    Parser_t* parser = &compiler->parser; /* ease typing */
    if (expected_type == parser->curr.type)
    {
        advance(compiler);
        return;
    }

    error_at_current(parser, errmsg);
}
















static void error_at_current(Parser_t* parser, const char* msg)
{
    error_at(parser, &parser->curr, msg);
}

static void error(Parser_t* parser, const char* msg)
{
    error_at(parser, &parser->prev, msg);
}


static void error_at(Parser_t* parser, const Token_t* token, const char* msg)
{
    /* error reporting is suppressed during panic mode */
    if (parser->panic_mode)
    {
        return;
    }


    parser->panic_mode = true;
    fprintf(stderr, "[Line %zu] Error", (size_t)token->line);


    if (token->type == TOKEN_EOF)
    {
        fprintf(stderr, " at end");
    }
    else if (token->type == TOKEN_ERROR)
    { /* nothing to print */ }
    else
    {
        fprintf(stderr, " at '%.*s'", (int)token->len, token->start);
    }

    fprintf(stderr, ": %s\n", msg);
    parser->had_error = true;
}




