

#include <stdarg.h>
#include <string.h>

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




typedef struct Local_t
{
    Token_t name;
    int depth;
} Local_t;

typedef struct Compiler_t
{
    Chunk_t* chunk;
    VMData_t* vmdata;
    Scanner_t scanner;
    Parser_t parser;
    
    int local_count;
    int scope_depth;
    Local_t locals[UINT8_COUNT];
} Compiler_t;




typedef void (*ParseFn_t)(Compiler_t*, bool);

typedef struct ParseRule_t
{
    ParseFn_t prefix;
    ParseFn_t infix;
    Precedence_t precedence;
} ParseRule_t;


#define VAR_UNDEFINED -1
#define LOCAL_DEFINED(p_local) ((p_local)->depth != VAR_UNDEFINED)










static void compiler_init(Compiler_t* compiler, VMData_t* data, const char* src, Chunk_t* chunk);
static void compiler_end(Compiler_t* compiler);


/* Pratt parser */
static void parse_precedence(Compiler_t* compiler, Precedence_t prec);

/* \returns &s_rules[operator] */
static const ParseRule_t* get_parse_rule(TokenType_t operator);

static size_t parse_variable(Compiler_t* compiler, const char* errmsg);

/* emit a string in the constant table with the given identifier */
static size_t identifier_constant(Compiler_t* compiler, const Token_t identifier);
static bool identifiers_equal(const Token_t a, const Token_t b);

/* define a global variable */
static void define_variable(Compiler_t* compiler, size_t global_index);

/* ensures that a local variable name is unique and calls add_local() */
static void declare_local(Compiler_t* compiler);

/* adds the previous token into the scope */
static void add_local(Compiler_t* compiler, const Token_t name);





static void declaration(Compiler_t* compiler);
static void decl_var(Compiler_t* compiler);




static void statement(Compiler_t* compiler);
static void stmt_expr(Compiler_t* compiler);
static void stmt_print(Compiler_t* compiler);
static void stmt_if(Compiler_t* compiler);
    static void scope_begin(Compiler_t* compiler);
static void stmt_block(Compiler_t* compiler);
    static void scope_end(Compiler_t* compiler);
static void stmt_while(Compiler_t* compiler);
static void stmt_for(Compiler_t* compiler);



/* parses an expression */
static void expression(Compiler_t* compiler);

static void string(Compiler_t* compiler, bool can_assign);
static void literal(Compiler_t* compiler, bool can_assign);
static void number(Compiler_t* compiler, bool can_assign);
static void grouping(Compiler_t* compiler, bool can_assign);
static void unary(Compiler_t* compiler, bool can_assign);
static void binary(Compiler_t* compiler, bool can_assign);
static void variable(Compiler_t* compiler, bool can_assign);
/* get or set a variable with the given name */
static void named_variable(Compiler_t* compiler, const Token_t name, bool can_assign);
static void and_(Compiler_t* compiler, bool can_assign);
static void or_(Compiler_t* compiler, bool can_assign);




/* emits a byte into the current chunk */
static void emit_byte(Compiler_t* compiler, uint8_t byte);

/* emits 2 byte into the chunk */
static void emit_2_bytes(Compiler_t* compiler, uint8_t byte1, uint8_t byte2);

/* emits num_bytes bytes into the chunk */
static void emit_bytes(Compiler_t* compile, size_t num_bytes, ...);

/* emits an opcode to access an object in the constant table */
static void emit_global(Compiler_t* compiler, Opc_t opcode, size_t addr);
static void emit_return(Compiler_t* compiler);

/* pushes a constant into the constant table */
static size_t emit_constant(Compiler_t* compiler, Value_t val);

/* emits a jump instruction, return the starting location of its operand */
static size_t emit_jump(Compiler_t* compiler, Opc_t jump_op);
static void patch_jump(Compiler_t* compiler, size_t start);

static void emit_loop(Compiler_t* compiler, size_t loop_head);




/* \returns the current compiling chunk */
static Chunk_t* current_chunk(Compiler_t* compiler);




/* query the scanner for the next token and put it into the parser */
static void advance(Compiler_t* compiler);
static bool match(Compiler_t* compiler, TokenType_t type);
static bool current_token_type(const Compiler_t* compiler, TokenType_t type);



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

static void synchronize(Compiler_t* compiler);







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
  [TOKEN_IDENTIFIER]    = {variable, NULL,   PREC_NONE},
  [TOKEN_STRING]        = {string,   NULL,   PREC_NONE},
  [TOKEN_NUMBER]        = {number,   NULL,   PREC_NONE},
  [TOKEN_AND]           = {NULL,     and_,   PREC_AND},
  [TOKEN_CLASS]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_ELSE]          = {NULL,     NULL,   PREC_NONE},
  [TOKEN_FALSE]         = {literal,  NULL,   PREC_NONE},
  [TOKEN_FOR]           = {NULL,     NULL,   PREC_NONE},
  [TOKEN_FUN]           = {NULL,     NULL,   PREC_NONE},
  [TOKEN_IF]            = {NULL,     NULL,   PREC_NONE},
  [TOKEN_NIL]           = {literal,  NULL,   PREC_NONE},
  [TOKEN_OR]            = {NULL,     or_,    PREC_OR},
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
    while (!match(&compiler, TOKEN_EOF))
    {
        declaration(&compiler);
    }
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

    compiler->local_count = 0;
    compiler->scope_depth = 0;
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
    bool can_assign = prec <= PREC_ASSIGNMENT;
    prefix_parser(compiler, can_assign);


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
        infix_parser(compiler, can_assign);
    }

    if (can_assign && match(compiler, TOKEN_EQUAL))
    {
        error(&compiler->parser, "Invalid assignment target.");
    }
}









static const ParseRule_t* get_parse_rule(TokenType_t type)
{
    return &s_rules[type];
}


static size_t parse_variable(Compiler_t* compiler, const char* errmsg)
{
    consume(compiler, TOKEN_IDENTIFIER, errmsg);

    if (compiler->scope_depth > 0)
    {
        declare_local(compiler);
        return 0; /* dummy value */
    }

    return identifier_constant(compiler, compiler->parser.prev);
}


static void mark_initialized(Compiler_t* compiler)
{
    compiler->locals[compiler->local_count - 1].depth = 
        compiler->scope_depth;
}


static size_t identifier_constant(Compiler_t* compiler, const Token_t token)
{
    return Chunk_AddUniqueConstant(current_chunk(compiler), 
        OBJ_VAL(ObjStr_Copy(compiler->vmdata, token.start, token.len))
    ); 
}


static bool identifiers_equal(const Token_t a, const Token_t b)
{
    if (a.len != b.len)
    {
        return false;
    }

    return memcmp(a.start, b.start, a.len) == 0;
}


static int resolve_local(Compiler_t* compiler, const Token_t name)
{
    for (int i = compiler->local_count - 1; i >= 0; i--)
    {
        Local_t* local = &compiler->locals[i];
        if (identifiers_equal(local->name, name))
        {
            if (!LOCAL_DEFINED(local))
            {
                error(&compiler->parser, "Can't read local variable in its own initializer.");
            }
            return i;
        }
    }
    return VAR_UNDEFINED;
}


static void define_variable(Compiler_t* compiler, size_t global_index)
{
    if (compiler->scope_depth > 0)
    {
        mark_initialized(compiler);
        return;
    }
    emit_global(compiler, OP_DEFINE_GLOBAL, global_index);
}


static void declare_local(Compiler_t* compiler)
{
    Token_t* name = &compiler->parser.prev;

    for (int i = compiler->local_count - 1; i >= 0; i++)
    {
        Local_t* local = &compiler->locals[i];
        if (LOCAL_DEFINED(local) && local->depth < compiler->scope_depth)
        {
            break;
        }


        if (identifiers_equal(local->name, *name))
        {
            error(&compiler->parser, "Already a varibale with this name in this scope.");
        }
    }

    add_local(compiler, *name);
}


static void add_local(Compiler_t* compiler, const Token_t name)
{
    if (compiler->local_count >= UINT8_COUNT)
    {
        error(&compiler->parser, "Too many local variables in scope.");
        return;
    }

    Local_t *local = &compiler->locals[compiler->local_count];

    local->name = name;
    local->depth = VAR_UNDEFINED;
    compiler->local_count += 1;
}






static void declaration(Compiler_t* compiler)
{
    if (match(compiler, TOKEN_VAR))
    {
        decl_var(compiler);
    }
    else 
    {
        statement(compiler);
    }


    if (compiler->parser.panic_mode)
    {
        synchronize(compiler);
    }
}


static void decl_var(Compiler_t* compiler)
{
    size_t global = parse_variable(compiler, "Expect variable name.");

    if (match(compiler, TOKEN_EQUAL))
    {
        expression(compiler);
    }
    else 
    {
        emit_byte(compiler, OP_NIL);
    }

    consume(compiler, TOKEN_SEMICOLON, "Expected ';' after expression.");
    define_variable(compiler, global);
}







static void statement(Compiler_t* compiler)
{
    if (match(compiler, TOKEN_PRINT))
    {
        stmt_print(compiler);
    }
    else if (match(compiler, TOKEN_FOR))
    {
        stmt_for(compiler);
    }
    else if (match(compiler, TOKEN_WHILE))
    {
        stmt_while(compiler);
    }
    else if (match(compiler, TOKEN_IF))
    {
        stmt_if(compiler);
    }
    else if (match(compiler, TOKEN_LEFT_BRACE))
    {
        scope_begin(compiler);
        stmt_block(compiler);
        scope_end(compiler);
    }
    else 
    {
        stmt_expr(compiler);
    }
}





static void stmt_expr(Compiler_t* compiler)
{
    expression(compiler);
    consume(compiler, TOKEN_SEMICOLON, "Expect ';' after expression.");
    emit_byte(compiler, OP_POP);
}


static void stmt_print(Compiler_t* compiler)
{
    expression(compiler);
    consume(compiler, TOKEN_SEMICOLON, "Expeceted ';' after expression.");
    emit_byte(compiler, OP_PRINT);
}


static void stmt_if(Compiler_t* compiler)
{
    consume(compiler, TOKEN_LEFT_PAREN, "Expected '(' after 'if'.");
    expression(compiler);
    consume(compiler, TOKEN_RIGHT_PAREN, "Expected ')' after expression.");

    /*
     * expr  
     * jif l1
     *  pop
     *  .. stmt ..
     *  jmp l3
     * l1:
     *   pop 
     *   expr  
     *   jif l3
     *   pop 
     *   .. stmt ..
     * l3:
     *   pop 
     * */
    size_t skip_if = emit_jump(compiler, OP_JUMP_IF_FALSE);
        emit_byte(compiler, OP_POP);
        statement(compiler);
    size_t done_if = emit_jump(compiler, OP_JUMP);
    patch_jump(compiler, skip_if);
        emit_byte(compiler, OP_POP);
        if (match(compiler, TOKEN_ELSE))
        {
            statement(compiler);
        }
    patch_jump(compiler, done_if);
}



static void scope_begin(Compiler_t* compiler)
{
    compiler->scope_depth += 1;
}


static void stmt_block(Compiler_t* compiler)
{
    while (!current_token_type(compiler, TOKEN_RIGHT_BRACE)
        && !current_token_type(compiler, TOKEN_EOF))
    {
        declaration(compiler);
    }

    consume(compiler, TOKEN_RIGHT_BRACE, "Expected '}' after block.");
}


static void scope_end(Compiler_t* compiler)
{
    compiler->scope_depth -= 1;
    int popped_vars = 0;

    while (compiler->local_count > 0
        && compiler->locals[compiler->local_count - 1].depth > compiler->scope_depth
    )
    {
        popped_vars += 1;
        compiler->local_count -= 1;
    }

    emit_2_bytes(compiler, OP_POPN, popped_vars);
}


static void stmt_while(Compiler_t* compiler)
{
    size_t loop_begin = current_chunk(compiler)->size;

        consume(compiler, TOKEN_LEFT_PAREN, "Expected '(' after 'while'.");
        expression(compiler);
        consume(compiler, TOKEN_RIGHT_PAREN, "Expected ')' after expression.");

    size_t skip_while = emit_jump(compiler, OP_JUMP_IF_FALSE);
            emit_byte(compiler, OP_POP);
            statement(compiler);
    emit_loop(compiler, loop_begin);

    patch_jump(compiler, skip_while);
    emit_byte(compiler, OP_POP);
}


static void stmt_for(Compiler_t* compiler)
{
    /*
     *  mamma mia la spaghetti
     *  loop_head:
     *    expr 
     *    jif loop_end
     *    pop
     *    jmp loop_body
     *  loop_inc:
     *    expr 
     *    pop 
     *    jmp loop_head
     *  loop_body:
     *      .. stmt .. 
     *    jmp loop_inc  
     *  loop_end:
     */
    scope_begin(compiler);
    {
        consume(compiler, TOKEN_LEFT_PAREN, "Expected '(' after 'for'.");
        /* intialization statement */
        if (match(compiler, TOKEN_SEMICOLON))
        {
            /* no intiializer */
        }
        else if (match(compiler, TOKEN_VAR))
        {
            decl_var(compiler);
        }
        else 
        {
            stmt_expr(compiler);
        }


        /* condition statement */
        size_t loop_head = current_chunk(compiler)->size;
        size_t exit_jump = (size_t)-1;
        if (!match(compiler, TOKEN_SEMICOLON))
        {
            expression(compiler);
            consume(compiler, TOKEN_SEMICOLON, "Expected ';' after expression.");
            exit_jump = emit_jump(compiler, OP_JUMP_IF_FALSE);
            emit_byte(compiler, OP_POP);
        }


        /* inc/dec expression */
        size_t increment_start = loop_head;
        if (!match(compiler, TOKEN_RIGHT_PAREN))
        {
            size_t to_body = emit_jump(compiler, OP_JUMP);
            increment_start = current_chunk(compiler)->size;
            expression(compiler);
            emit_byte(compiler, OP_POP); 
            consume(compiler, TOKEN_RIGHT_PAREN, "Expected ')' after expression.");

            emit_loop(compiler, loop_head); /* goes to condition statement */
            loop_head = increment_start;
            patch_jump(compiler, to_body);
        }

        
        /* loop body */
        statement(compiler);
        emit_loop(compiler, increment_start);

        /* exit location */
        if (exit_jump != (size_t)-1)
        {
            patch_jump(compiler, exit_jump);
            emit_byte(compiler, OP_POP);
        }
    }
    scope_end(compiler);
}










static void expression(Compiler_t* compiler)
{
    parse_precedence(compiler, PREC_ASSIGNMENT);
}








static void string(Compiler_t* compiler, bool can_assign)
{
    (void)can_assign;
    emit_constant(compiler, 
        OBJ_VAL(ObjStr_Copy(compiler->vmdata, 
            /* not including '"' in string literals */
            compiler->parser.prev.start + 1, compiler->parser.prev.len - 2))
    );
}


static void literal(Compiler_t* compiler, bool can_assign)
{
    (void)can_assign;
    switch (compiler->parser.prev.type)
    {
    case TOKEN_TRUE:    emit_byte(compiler, OP_TRUE); break;
    case TOKEN_FALSE:   emit_byte(compiler, OP_FALSE); break;
    case TOKEN_NIL:     emit_byte(compiler, OP_NIL); break;
    default: CLOX_ASSERT(false && "Unhandled literal type."); return;
    }
}


static void number(Compiler_t* compiler, bool can_assign)
{
    (void)can_assign;
    const Value_t val = NUMBER_VAL(strtod(compiler->parser.prev.start, NULL));
    emit_constant(compiler, val);
}


static void grouping(Compiler_t* compiler, bool can_assign)
{
    (void)can_assign;
    /* assumes that '(' is already consumed */
    expression(compiler);
    consume(compiler, TOKEN_RIGHT_PAREN, "Expected ')' after expression.");
}


static void unary(Compiler_t* compiler, bool can_assign)
{
    (void)can_assign;
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


static void binary(Compiler_t* compiler, bool can_assign)
{
    (void)can_assign;
    const TokenType_t operator = compiler->parser.prev.type;
    const ParseRule_t* rule = get_parse_rule(operator);
    /* 
    *   because most binary operators are left associative, 
    *   the next operator does not have equal precedence to the current operator
    */
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


static void variable(Compiler_t* compiler, bool can_assign)
{
    named_variable(compiler, compiler->parser.prev, can_assign);
}


static void named_variable(Compiler_t* compiler, const Token_t name, bool can_assign)
{
    uint8_t get_op, set_op;
    int arg = resolve_local(compiler, name);
    if (VAR_UNDEFINED == arg) /* then is assumed to be global */
    {
        arg = identifier_constant(compiler, name);
        get_op = OP_GET_GLOBAL;
        set_op = OP_SET_GLOBAL;
    }
    else 
    {
        get_op = OP_GET_LOCAL;
        set_op = OP_SET_LOCAL;
    }
    


    
    if (can_assign && match(compiler, TOKEN_EQUAL))
    {
        /* compiles initializer */
        expression(compiler);
        emit_global(compiler, set_op, (unsigned)arg);
    }
    else 
    {
        emit_global(compiler, get_op,  (unsigned)arg);
    }
    
}



static void and_(Compiler_t* compiler, bool can_assign)
{
    (void)can_assign;

    /* LHS already compiled */
    size_t skip_right = emit_jump(compiler, OP_JUMP_IF_FALSE);
        emit_byte(compiler, OP_POP);
        parse_precedence(compiler, PREC_AND);
    patch_jump(compiler, skip_right);
}


static void or_(Compiler_t* compiler, bool can_assign)
{
    (void)can_assign;

    size_t continue_or = emit_jump(compiler, OP_JUMP_IF_FALSE);
    size_t skip_or = emit_jump(compiler, OP_JUMP);
    patch_jump(compiler, continue_or);
        emit_byte(compiler, OP_POP);
        parse_precedence(compiler, PREC_OR);
    patch_jump(compiler, skip_or);
    /* result on stack */
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
        const uint8_t byte = va_arg(args, unsigned int);
        emit_byte(compiler, byte);
    }

    va_end(args);
}


static void emit_global(Compiler_t* compiler, Opc_t opcode, size_t addr)
{
    if (addr <= UINT8_MAX)
    {
        emit_2_bytes(compiler, opcode, addr);
    }
    else if (opcode == OP_SET_LOCAL || opcode == OP_GET_LOCAL)
    {
        error(&compiler->parser, "Cannot have more than 256 variables in scope.");
    }
    else 
    {
        emit_bytes(compiler, 4, 
            opcode | 0x80,
            addr >> 16,
            addr >> 8,
            addr >> 0
        );
    }
}


static void emit_return(Compiler_t* compiler)
{
    emit_byte(compiler, OP_RETURN);
}


static size_t emit_constant(Compiler_t* compiler, Value_t val)
{
    size_t val_addr = Chunk_WriteConstant(
        current_chunk(compiler), val, compiler->parser.prev.line
    );
    if (val_addr > MAX_CONST_IN_CHUNK)
    {
        error(&compiler->parser, "Too many constants in one chunk.");
    }
    return val_addr;
}


static size_t emit_jump(Compiler_t* compiler, Opc_t jump_ins)
{
    emit_bytes(compiler, 3, 
        jump_ins, 0xff, 0xff
    );
    return current_chunk(compiler)->size - 2; /* where its operand is */
}


static void patch_jump(Compiler_t* compiler, size_t start)
{
    uint32_t offset = current_chunk(compiler)->size - (start + 2);
    if (offset > UINT16_MAX)
    {
        error(&compiler->parser, "Too much code in an if statement.");
        return;
    }

    current_chunk(compiler)->code[start + 0] = offset >> 8;
    current_chunk(compiler)->code[start + 1] = offset;
}


static void emit_loop(Compiler_t* compiler, size_t loop_head)
{
    emit_byte(compiler, OP_LOOP);
    uint32_t offset = (current_chunk(compiler)->size + 2) - loop_head;

    if (offset > UINT16_MAX)
    {
        error(&compiler->parser, "Loop body too large.");
        return;
    }

    emit_2_bytes(compiler, offset >> 8, offset);
}








static Chunk_t* current_chunk(Compiler_t* compiler)
{
    return compiler->chunk;
}






static void advance(Compiler_t* compiler)
{
    Parser_t* parser = &compiler->parser; /* ease typing */
    parser->prev = parser->curr;


    parser->curr = Scanner_ScanToken(&compiler->scanner);
    while (TOKEN_ERROR == parser->curr.type) 
    {
        parser->prev = parser->curr;
        parser->curr = Scanner_ScanToken(&compiler->scanner);
        error_at_current(parser, parser->curr.start);
    }
}




static bool match(Compiler_t* compiler, TokenType_t type)
{
    if (current_token_type(compiler, type))
    {
        advance(compiler);
        return true;
    }
    return false;
}


static bool current_token_type(const Compiler_t* compiler, TokenType_t type)
{
    return compiler->parser.curr.type == type;
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



static void synchronize(Compiler_t* compiler)
{
    Parser_t *parser = &compiler->parser;
    parser->panic_mode = false;
    while (parser->curr.type != TOKEN_EOF)
    {
        if (parser->prev.type == TOKEN_SEMICOLON)
        {
            switch (parser->curr.type)
            {
            case TOKEN_CLASS:
            case TOKEN_FUN:
            case TOKEN_VAR:
            case TOKEN_FOR:
            case TOKEN_IF:
            case TOKEN_WHILE:
            case TOKEN_PRINT:
            case TOKEN_RETURN:
            case TOKEN_RIGHT_BRACE:
                return;

            default: break; // Do nothing.
            }
        }

        advance(compiler);
    }
}
