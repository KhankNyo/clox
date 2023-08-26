

#include <stdio.h>
#include <stdarg.h>

#include "include/common.h"
#include "include/vm.h"
#include "include/compiler.h"
#include "include/debug.h"




static InterpretResult_t run(VM_t* vm);
static void stack_reset(VM_t* vm);
static Value_t peek(const VM_t* vm, int offset);
static bool is_falsey(const Value_t val);

static void runtime_error(VM_t* vm, const char* fmt, ...);




void VM_Init(VM_t* vm)
{
    vm->chunk = NULL;
    stack_reset(vm);
}



void VM_Free(VM_t* vm)
{
    VM_Init(vm);
}




void VM_Push(VM_t* vm, Value_t val)
{
    *vm->sp = val;
    vm->sp++;
}


Value_t VM_Pop(VM_t* vm)
{
    vm->sp--;
    return *vm->sp;
}



InterpretResult_t VM_Interpret(VM_t* vm, Allocator_t* alloc, const char* src)
{
    CLOX_ASSERT(NULL != vm->sp && "call VM_Init before passing it to VM_Interpret.");
    Chunk_t chunk;
    Chunk_Init(&chunk, alloc, 1);

    if (!Compile(src, &chunk))
    {
        Chunk_Free(&chunk);
        return INTERPRET_COMPILE_ERROR;
    }

    vm->chunk = &chunk;
    vm->ip = chunk.code;

    InterpretResult_t ret = run(vm);
    Chunk_Free(&chunk);
    return ret;
}














static InterpretResult_t run(VM_t* vm)
{
#define READ_BYTE() (*vm->ip++)
#define READ_CONSTANT() (vm->chunk->consts.vals[READ_BYTE()])
#define BINARY_OP(ValueType, op) \
do{\
    if (!IS_NUMBER(peek(vm, 0)) || !IS_NUMBER(peek(vm, 1))) {\
        runtime_error(vm, "Operands must be numbers.");\
        return INTERPRET_RUNTIME_ERROR;\
    }\
    double b = AS_NUMBER(VM_Pop(vm));\
    double a = AS_NUMBER(VM_Pop(vm));\
    VM_Push(vm, ValueType(a op b));\
}while(0)


    while (true)
    {

#ifdef DEBUG_TRACE_EXECUTION
        printf("          ");
        for (Value_t* slot = vm->stack; slot < vm->sp; slot++) {
            printf("[ ");
            Value_Print(stderr, *slot);
            printf(" ]");
        }
        printf("\n");
        Disasm_Instruction(stderr, vm->chunk, vm->ip - vm->chunk->code);
#endif /* DEBUG_TRACE_EXECUTION */

        uint8_t ins = READ_BYTE();
        switch (ins)
        {

        case OP_CONSTANT:
        case OP_CONSTANT_LONG:
        {
            const Value_t constant = READ_CONSTANT();
            VM_Push(vm, constant);
        }
        break;

        case OP_NEGATE:
            if (!IS_NUMBER(peek(vm, 0)))
            {
                runtime_error(vm, "Operand must be a number.");
                return INTERPRET_RUNTIME_ERROR;
            }
            VM_Push(vm, NUMBER_VAL(-AS_NUMBER(VM_Pop(vm)))); 
            break;
        case OP_NOT:
            VM_Push(vm, BOOL_VAL(is_falsey(VM_Pop(vm))));
            break;


		case OP_ADD:        BINARY_OP(NUMBER_VAL, + ); break;
        case OP_SUBTRACT:   BINARY_OP(NUMBER_VAL, - ); break;
        case OP_MULTIPLY:   BINARY_OP(NUMBER_VAL, * ); break;
        case OP_DIVIDE:     BINARY_OP(NUMBER_VAL, / ); break;

        case OP_EQUAL:
        {
            Value_t b = VM_Pop(vm);
            Value_t a = VM_Pop(vm);
            VM_Push(vm, BOOL_VAL(Value_Equal(a, b)));
        }
        break;
        case OP_GREATER:    BINARY_OP(BOOL_VAL, > ); break;
        case OP_LESS:       BINARY_OP(BOOL_VAL, < ); break;


        case OP_TRUE:       VM_Push(vm, BOOL_VAL(true)); break;
        case OP_FALSE:      VM_Push(vm, BOOL_VAL(false)); break;
        case OP_NIL:        VM_Push(vm, NIL_VAL()); break;

        case OP_RETURN:
        {
            Value_Print(stderr, VM_Pop(vm));
            puts("");
            return INTERPRET_OK;
        }

        default: break;
        }
    }


#undef BINARY_OP
#undef READ_CONSTANT
#undef READ_BYTE
}


static void stack_reset(VM_t* vm)
{
    vm->sp = &vm->stack[0];
}


static Value_t peek(const VM_t* vm, int offset)
{
    return vm->sp[-1 - offset];
}



static bool is_falsey(const Value_t val)
{
    return IS_NIL(val) || (IS_BOOL(val) && !AS_BOOL(val));
}



static void runtime_error(VM_t* vm, const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fputc('\n', stderr);

    size_t ins_addr = vm->ip - vm->chunk->code - 1;
    int line = LineInfo_GetLine(vm->chunk->line_info, ins_addr);
    fprintf(stderr, "[line %d] in script\n", line);
    stack_reset(vm);
}






