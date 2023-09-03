

#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "include/common.h"
#include "include/table.h"
#include "include/vm.h"
#include "include/compiler.h"
#include "include/debug.h"
#include "include/object.h"
#include "include/memory.h"





static InterpretResult_t run(VM_t* vm);
static void stack_reset(VM_t* vm);
static Value_t peek(const VM_t* vm, int offset);
static bool is_falsey(const Value_t val);
static void str_concatenate(VM_t* vm);

static void runtime_error(VM_t* vm, const char* fmt, ...);
static void debug_trace_execution(const VM_t* vm);








void VM_Init(VM_t* vm, Allocator_t* alloc)
{
    vm->chunk = NULL;
    vm->data.head = NULL;
    vm->data.alloc = alloc;
    stack_reset(vm);
    Table_Init(&vm->data.strings, alloc);
    Table_Init(&vm->data.globals, alloc);
}



void VM_Free(VM_t* vm)
{
    VM_FreeObjects(&vm->data);
    Table_Free(&vm->data.strings);
    Table_Free(&vm->data.globals);
    VM_Init(vm, vm->data.alloc);
}

void VM_FreeObjects(VMData_t* data)
{
    Obj_t* node = data->head;
    while (NULL != node)
    {
        Obj_t* next = node->next;
        Obj_Free(data->alloc, node);
        node = next;
    }
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

    if (!Compile(&vm->data, src, &chunk))
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


void VM_FreeObjs(VM_t* vm)
{
    Obj_t* node = vm->data.head;
    while (NULL != node)
    {
        Obj_t* next = node->next;
        Obj_Free(vm->chunk->alloc, node);
        node = next;
    }
}
















static InterpretResult_t run(VM_t* vm)
{
/* fuck macros */

#define READ_BYTE() (*vm->ip++)
#define READ_CONSTANT() (vm->chunk->consts.vals[READ_BYTE()])


#define READ_SHORT() \
    (vm->ip += 2, (((uint16_t)vm->ip[-1] << 8) | vm->ip[-2]))

    // my dear god 
#define READ_LONG() \
    (vm->ip += 3, (((uint32_t)vm->ip[-1] << 16) \
                   | ((uint32_t)vm->ip[-2] << 8) \
                   | vm->ip[-3]))
#define READ_CONSTANT_LONG() (vm->chunk->consts.vals[READ_LONG()])


#define READ_STR() AS_STR(READ_CONSTANT())
#define READ_STR_LONG() AS_STR(READ_CONSTANT_LONG())


#define GET_GLOBAL(macro_read_str) \
    do {\
        ObjString_t* name = macro_read_str();\
        Value_t val;\
        if (!Table_Get(&vm->data.globals, name, &val)) {\
            runtime_error(vm, "Undefined variable: '%s'.", name->cstr);\
            return INTERPRET_RUNTIME_ERROR;\
        }\
        VM_Push(vm, val);\
    } while (0)

#define SET_GLOBAL(macro_read_str) \
    do {\
        ObjString_t* name = macro_read_str();\
        if (Table_Set(&vm->data.globals, name, peek(vm, 0))) {\
            Table_Delete(&vm->data.globals, name);\
            runtime_error(vm, "Undefined variable: '%s'.", name->cstr);\
            return INTERPRET_RUNTIME_ERROR;\
        }\
    } while (0)


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
        debug_trace_execution(vm);
        CLOX_ASSERT(vm->sp >= &vm->stack[0]);
        uint8_t ins = READ_BYTE();

        switch (ins)
        {

        case OP_CONSTANT_LONG:  VM_Push(vm, READ_CONSTANT_LONG()); break;
        case OP_CONSTANT:       VM_Push(vm, READ_CONSTANT()); break;

        case OP_NEGATE:
            if (!IS_NUMBER(peek(vm, 0)))
            {
                runtime_error(vm, "Operand must be a number.");
                return INTERPRET_RUNTIME_ERROR;
            }
            VM_Push(vm, NUMBER_VAL(-AS_NUMBER(VM_Pop(vm)))); 
            break;

        case OP_NOT:        VM_Push(vm, BOOL_VAL(is_falsey(VM_Pop(vm)))); break;


        case OP_ADD:
            if (IS_STRING(peek(vm, 0)) && IS_STRING(peek(vm, 1)))
            {
                str_concatenate(vm);
            }
            else if (IS_NUMBER(peek(vm, 0)) && IS_NUMBER(peek(vm, 1)))
            {
                double b = AS_NUMBER(VM_Pop(vm));
                double a = AS_NUMBER(VM_Pop(vm));
                VM_Push(vm, NUMBER_VAL(a + b));
            }
            else
            {
                runtime_error(vm, "Operands must be numbers or strings.");
                return INTERPRET_RUNTIME_ERROR;
            }
            break;
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

        case OP_RETURN:     return INTERPRET_OK;

        case OP_PRINT:
        {
            Value_Print(stdout, VM_Pop(vm));
            printf("\n");
        }
        break;

        case OP_POP:        VM_Pop(vm); break;
        case OP_POPN:       vm->sp -= READ_BYTE(); break;

        case OP_DEFINE_GLOBAL_LONG:
        {
            ObjString_t* name = READ_STR_LONG();
            Table_Set(&vm->data.globals, name, peek(vm, 0));
            VM_Pop(vm);
        }
        break;
        case OP_DEFINE_GLOBAL:
        {
            ObjString_t* name = READ_STR();
            Table_Set(&vm->data.globals, name, peek(vm, 0));
            VM_Pop(vm);
        }
        break;


        case OP_SET_LOCAL: vm->stack[READ_BYTE()] = peek(vm, 0); break;
        case OP_GET_LOCAL: VM_Push(vm, vm->stack[READ_BYTE()]); break;

        case OP_GET_GLOBAL_LONG:    GET_GLOBAL(READ_STR_LONG); break;
        case OP_GET_GLOBAL:         GET_GLOBAL(READ_STR); break;
        case OP_SET_GLOBAL_LONG:    SET_GLOBAL(READ_STR_LONG); break;
        case OP_SET_GLOBAL:         SET_GLOBAL(READ_STR); break;

        case OP_JUMP:
        {
            uint16_t offset = READ_SHORT();
            vm->ip += offset;
        }
        break;
        case OP_JUMP_IF_FALSE:
        {
            uint16_t offset = READ_SHORT();
            if (is_falsey(peek(vm, 0)))
            {
                vm->ip += offset;
            }
        }
        break;


        
        default: break;
        }
    }


#undef BINARY_OP
#undef READ_CONSTANT
#undef READ_CONSTANT_LONG
#undef READ_STR
#undef READ_STR_LONG
#undef READ_BYTE
#undef READ_LONG
#undef READ_SHORT
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



static void str_concatenate(VM_t* vm)
{
    ObjString_t* str_b = AS_STR(VM_Pop(vm));
    ObjString_t* str_a = AS_STR(VM_Pop(vm));
    ObjString_t* result = NULL;
    int len = str_b->len + str_a->len;
    char* buf = NULL;
    
#ifdef OBJSTR_FLEXIBLE_ARR
    const ObjString_t* strs[] = {str_a, str_b};

    uint32_t hash = ObjStr_HashStrs(2, strs); 
    result = Table_FindStrs(&vm->data.strings, 2, strs, hash, len);
    if (NULL == result)
    {
        result = ObjStr_Reserve(&vm->data, len);
        buf = result->cstr;

        memcpy(buf, str_a->cstr, str_a->len);
        memcpy(buf + str_a->len, str_b->cstr, str_b->len);
        buf[len] = '\0';

        ObjStr_Intern(&vm->data, result);
    }
#else
    buf = ALLOCATE(vm->chunk->alloc, char, len + 1);
    
    memcpy(buf, str_a->cstr, str_a->len);
    memcpy(buf + str_a->len, str_b->cstr, str_b->len);
    buf[len] = '\0';
    
    result = ObjStr_Steal(&vm->data, buf, len);
#endif /* OBJSTR_FLEXIBLE_ARR */

    VM_Push(vm, OBJ_VAL(result));
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



static void debug_trace_execution(const VM_t* vm)
{
#ifdef DEBUG_TRACE_EXECUTION 
    fprintf(stderr, "          ");
    for (const Value_t* slot = vm->stack; slot < vm->sp; slot++) 
    {
        fprintf(stderr, "[ ");
        Value_Print(stderr, *slot);
        fprintf(stderr, " ]");
    }
    fprintf(stderr, "\n");
    Disasm_Instruction(stderr, vm->chunk, vm->ip - vm->chunk->code);
#else
    (void)vm;
#endif /* DEBUG_TRACE_EXECUTION */
}



