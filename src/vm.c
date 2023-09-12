

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
#include "include/natives.h"






static InterpretResult_t run(VM_t* vm);
static void init_state(VM_t* vm, Allocator_t* alloc);
static void stack_reset(VM_t* vm);
static Value_t peek(const VM_t* vm, int offset);
static bool is_falsey(const Value_t val);
static void str_concatenate(VM_t* vm);

static void runtime_error(VM_t* vm, const char* fmt, ...);
static void debug_trace_execution(const VM_t* vm);

static CallFrame_t* peek_cf(VM_t* vm, int offset);
static void trace_cf(const VM_t* vm);


static bool call_value(VM_t* vm, Value_t callee, int argc);

/* pushes a closure onto the call frame */
static bool call(VM_t* vm, ObjClosure_t* closure, int argc);
static bool call_native(VM_t* vm, ObjNativeFn_t* native, int argc);


static ObjUpval_t* capture_upval(VMData_t* data, Value_t* base);
static void close_upval(VMData_t* vmdata, Value_t* sp);








void VM_Init(VM_t* vm, Allocator_t* alloc)
{
    init_state(vm, alloc);
    CLOX_ASSERT(VM_DefineNative(vm, "clock", Native_Clock, 0));
}

void VM_Reset(VM_t* vm)
{
    VM_Free(vm);
    Allocator_Defrag(VM_GET_DATA(vm).alloc, ALLOCATOR_DEFRAG_DEFAULT);
    VM_Init(vm, VM_GET_DATA(vm).alloc);
}



void VM_Free(VM_t* vm)
{
    VM_FreeObjects(&VM_GET_DATA(vm));
    Table_Free(&VM_GET_DATA(vm).strings);
    Table_Free(&VM_GET_DATA(vm).globals);
    init_state(vm, VM_GET_DATA(vm).alloc);
}

void VM_FreeObjects(VMData_t* data)
{
    Obj_t* node = data->head;
    while (NULL != node)
    {
        Obj_t* next = node->next;
        Obj_Free(data, node);
        node = next;
    }
}


bool VM_Push(VM_t* vm, Value_t val)
{
    if (VM_GET_DATA(vm).sp >= &VM_GET_DATA(vm).stack[VM_STACK_MAX])
    {
        runtime_error(vm, "Stack overflow.");
        return false;
    }
    *VM_GET_DATA(vm).sp = val;
    VM_GET_DATA(vm).sp++;
    return true;
}


Value_t VM_Pop(VM_t* vm)
{
    CLOX_ASSERT(VM_GET_DATA(vm).sp >= VM_GET_DATA(vm).stack);
    VM_GET_DATA(vm).sp--;
    return *VM_GET_DATA(vm).sp;
}



bool VM_DefineNative(VM_t* vm, const char* name, NativeFn_t fn, uint8_t argc)
{
    if (!VM_Push(vm, OBJ_VAL(ObjStr_Copy(&VM_GET_DATA(vm), name, strlen(name)))))
        return false;
    if (!VM_Push(vm, OBJ_VAL(ObjNFn_Create(&VM_GET_DATA(vm), fn, argc))))
    {
        VM_Pop(vm);
        return false;
    }

    Table_Set(&VM_GET_DATA(vm).globals, 
        AS_STR(peek(vm, 1)), peek(vm, 0)
    );

    VM_GET_DATA(vm).sp -= 2;
    return true;
}



InterpretResult_t VM_Interpret(VM_t* vm, const char* src)
{
    CLOX_ASSERT(NULL != VM_GET_DATA(vm).sp && "call VM_Init before passing it to VM_Interpret.");

    ObjFunction_t* fun = Compile(&VM_GET_DATA(vm), src);
    if (NULL == fun)
        return INTERPRET_COMPILE_ERROR;

    VM_Push(vm, OBJ_VAL(fun));
    ObjClosure_t* script = ObjCls_Create(&VM_GET_DATA(vm), fun);
    VM_Pop(vm);
    VM_Push(vm, OBJ_VAL(script));

    call(vm, script, 0);
    return run(vm);
}

















static InterpretResult_t run(VM_t* vm)
{
    CLOX_ASSERT(vm->frame_count > 0);
    CallFrame_t* current = peek_cf(vm, 0);

#define PUSH(val)\
do{\
    if (!VM_Push(vm, val))\
        return INTERPRET_RUNTIME_ERROR;\
}while(0)

#define POP() VM_Pop(vm)


#define GET_IP() (current->ip)
#define READ_BYTE() (*GET_IP()++)
#define READ_CONSTANT() (current->closure->fun->chunk.consts.vals[READ_BYTE()])


#define READ_SHORT() \
    (GET_IP() += 2, (((uint16_t)GET_IP()[-2] << 8) | GET_IP()[-1]))

    // my dear god 
#define READ_LONG() \
    (GET_IP() += 3, (((uint32_t)GET_IP()[-3] << 16) \
                   | ((uint32_t)GET_IP()[-2] << 8) \
                   | GET_IP()[-1]))
#define READ_CONSTANT_LONG() (current->closure->fun->chunk.consts.vals[READ_LONG()])


#define READ_STR() AS_STR(READ_CONSTANT())
#define READ_STR_LONG() AS_STR(READ_CONSTANT_LONG())


#define GET_GLOBAL(macro_read_str) \
    do {\
        ObjString_t* name = macro_read_str();\
        Value_t val;\
        if (!Table_Get(&VM_GET_DATA(vm).globals, name, &val)) {\
            runtime_error(vm, "Undefined variable: '%s'.", name->cstr);\
            return INTERPRET_RUNTIME_ERROR;\
        }\
        PUSH(val);\
    } while (0)

#define SET_GLOBAL(macro_read_str) \
    do {\
        ObjString_t* name = macro_read_str();\
        if (Table_Set(&VM_GET_DATA(vm).globals, name, peek(vm, 0))) {\
            Table_Delete(&VM_GET_DATA(vm).globals, name);\
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
    double b = AS_NUMBER(POP());\
    double a = AS_NUMBER(POP());\
    PUSH(ValueType(a op b));\
}while(0)








    while (true)
    {
        debug_trace_execution(vm);
        CLOX_ASSERT(VM_GET_DATA(vm).sp >= &VM_GET_DATA(vm).stack[0]);
        uint8_t ins = READ_BYTE();

        switch (ins)
        {

        case OP_CONSTANT_LONG:  PUSH(READ_CONSTANT_LONG()); break;
        case OP_CONSTANT:       PUSH(READ_CONSTANT()); break;

        case OP_NEGATE:
            if (!IS_NUMBER(peek(vm, 0)))
            {
                runtime_error(vm, "Operand must be a number.");
                return INTERPRET_RUNTIME_ERROR;
            }
            PUSH(NUMBER_VAL(-AS_NUMBER(POP()))); 
            break;

        case OP_NOT:        PUSH(BOOL_VAL(is_falsey(POP()))); break;


        case OP_ADD:
            if (IS_STRING(peek(vm, 0)) && IS_STRING(peek(vm, 1)))
            {
                str_concatenate(vm);
            }
            else if (IS_NUMBER(peek(vm, 0)) && IS_NUMBER(peek(vm, 1)))
            {
                double b = AS_NUMBER(POP());
                double a = AS_NUMBER(POP());
                PUSH(NUMBER_VAL(a + b));
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
            Value_t b = POP();
            Value_t a = POP();
            PUSH(BOOL_VAL(Value_Equal(a, b)));
        }
        break;
        case OP_GREATER:    BINARY_OP(BOOL_VAL, > ); break;
        case OP_LESS:       BINARY_OP(BOOL_VAL, < ); break;


        case OP_TRUE:       PUSH(BOOL_VAL(true)); break;
        case OP_FALSE:      PUSH(BOOL_VAL(false)); break;
        case OP_NIL:        PUSH(NIL_VAL()); break;


        case OP_PRINT:
        {
            Value_Print(stdout, POP());
            printf("\n");
        }
        break;

        case OP_POP:        POP(); break;
        case OP_POPN:       VM_GET_DATA(vm).sp -= READ_BYTE(); break;

        case OP_DEFINE_GLOBAL_LONG:
        {
            ObjString_t* name = READ_STR_LONG();
            Table_Set(&VM_GET_DATA(vm).globals, name, peek(vm, 0));
            POP();
        }
        break;
        case OP_DEFINE_GLOBAL:
        {
            ObjString_t* name = READ_STR();
            Table_Set(&VM_GET_DATA(vm).globals, name, peek(vm, 0));
            POP();
        }
        break;


        case OP_SET_LOCAL: 
        {
            uint8_t slot = READ_BYTE();
            current->base[slot] = peek(vm, 0);
        }
        break;
        case OP_GET_LOCAL: 
        {
            uint8_t slot = READ_BYTE();
            PUSH(current->base[slot]); 
        }
        break;

        case OP_GET_GLOBAL_LONG:    GET_GLOBAL(READ_STR_LONG); break;
        case OP_GET_GLOBAL:         GET_GLOBAL(READ_STR); break;
        case OP_SET_GLOBAL_LONG:    SET_GLOBAL(READ_STR_LONG); break;
        case OP_SET_GLOBAL:         SET_GLOBAL(READ_STR); break;

        case OP_JUMP:
        {
            uint16_t offset = READ_SHORT();
            GET_IP() += offset;
        }
        break;
        case OP_JUMP_IF_FALSE:
        {
            uint16_t offset = READ_SHORT();
            if (is_falsey(peek(vm, 0)))
            {
                GET_IP() += offset;
            }
        }
        break;
        case OP_LOOP:
        {
            uint16_t offset = READ_SHORT();
            GET_IP() -= offset;
        }
        break;
        case OP_CALL:
        {
            uint8_t argc = READ_BYTE();
            if (!call_value(vm, peek(vm, argc), argc))
            {
                return INTERPRET_RUNTIME_ERROR;
            }
            current = peek_cf(vm, 0);
            if (NULL == current)
            {
                return INTERPRET_RUNTIME_ERROR;
            }
        }
        break;

        case OP_GET_UPVALUE:
        {
            uint8_t slot = READ_BYTE();
            ObjUpval_t* upval = current->closure->upvals[slot];
            PUSH(*upval->location);
        }
        break;
        case OP_SET_UPVALUE:
        {
            uint8_t slot = READ_BYTE();
            ObjUpval_t* upval = current->closure->upvals[slot];

            /* does not pop because this instruction is only used in assignment expressions,
             * and expressions do have return value in Lox,
             * so if we pop it off, we'd need to push it back 
             */
            *upval->location = peek(vm, 0);
        }
        break;
        case OP_CLOSURE:
        {
            ObjFunction_t* fun = AS_FUNCTION(READ_CONSTANT());
            ObjClosure_t* closure = ObjCls_Create(&VM_GET_DATA(vm), fun);
            PUSH(OBJ_VAL(closure));

            for (int i = 0; i < fun->upval_count; i++)
            {
                uint8_t is_local = READ_BYTE();
                uint8_t slot = READ_BYTE();

                if (is_local)
                {
                    closure->upvals[i] = capture_upval(&VM_GET_DATA(vm), current->base + slot);
                }
                else 
                {
                    closure->upvals[i] = current->closure->upvals[slot];
                }
            }
        }
        break;
        case OP_CLOSE_UPVALUE:
            close_upval(&VM_GET_DATA(vm), VM_GET_DATA(vm).sp - 1);
            POP();
            break;
        case OP_RETURN:
        {
            Value_t val = POP();
            close_upval(&VM_GET_DATA(vm), current->base);
            vm->frame_count--;
            if (vm->frame_count == 0)
            {
                POP();
                return INTERPRET_OK;
            }

            VM_GET_DATA(vm).sp = current->base;
            PUSH(val);
            current = peek_cf(vm, 0);
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
#undef PUSH
#undef GET_IP
}








static void init_state(VM_t* vm, Allocator_t* alloc)
{
    VM_GET_DATA(vm).head = NULL;
    VM_GET_DATA(vm).open_upvals = NULL;
    VM_GET_DATA(vm).alloc = alloc;
    vm->frame_count = 0;

    stack_reset(vm);
    Table_Init(&VM_GET_DATA(vm).strings, &VM_GET_DATA(vm));
    Table_Init(&VM_GET_DATA(vm).globals, &VM_GET_DATA(vm));
}


static void stack_reset(VM_t* vm)
{
    VM_GET_DATA(vm).sp = &VM_GET_DATA(vm).stack[0];
    vm->frame_count = 0;
}


static Value_t peek(const VM_t* vm, int offset)
{
    CLOX_ASSERT(VM_GET_DATA(vm).sp >= VM_GET_DATA(vm).stack && "peek");
    return VM_GET_DATA(vm).sp[-1 - offset];
}



static bool is_falsey(const Value_t val)
{
    return IS_NIL(val) 
        || (IS_BOOL(val) && !AS_BOOL(val))
        || (IS_NUMBER(val) && Value_Equal(val, NUMBER_VAL(0.0f)));
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
    result = Table_FindStrs(&VM_GET_DATA(vm).strings, 2, strs, hash, len);
    if (NULL == result)
    {
        result = ObjStr_Reserve(&VM_GET_DATA(vm), len);
        buf = result->cstr;

        memcpy(buf, str_a->cstr, str_a->len);
        memcpy(buf + str_a->len, str_b->cstr, str_b->len);
        buf[len] = '\0';

        ObjStr_Intern(&VM_GET_DATA(vm), result);
    }
#else
    buf = ALLOCATE(VM_GET_DATA(vm).alloc, char, len + 1);
    
    memcpy(buf, str_a->cstr, str_a->len);
    memcpy(buf + str_a->len, str_b->cstr, str_b->len);
    buf[len] = '\0';
    
    result = ObjStr_Steal(&VM_GET_DATA(vm), buf, len);
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

    CLOX_ASSERT(vm->frame_count > 0);
    trace_cf(vm);

    stack_reset(vm);
}



static void debug_trace_execution(const VM_t* vm)
{
#ifdef DEBUG_TRACE_EXECUTION 
    CLOX_ASSERT(vm->frame_count > 0);
    const CallFrame_t* current = &vm->frames[vm->frame_count - 1];

    fprintf(stderr, "          ");
    for (const Value_t* slot = VM_GET_DATA(vm).stack; slot < VM_GET_DATA(vm).sp; slot++) 
    {
        fprintf(stderr, "[ ");
        Value_Print(stderr, *slot);
        fprintf(stderr, " ]");
    }
    fprintf(stderr, "\n");
    Disasm_Instruction(stderr, 
        &current->closure->fun->chunk, 
        current->ip - current->closure->fun->chunk.code
    );
#else
    (void)vm;
#endif /* DEBUG_TRACE_EXECUTION */
}



static CallFrame_t* peek_cf(VM_t* vm, int offset)
{
    CLOX_ASSERT(vm->frame_count - 1 - offset >= 0);
    return &vm->frames[vm->frame_count - 1 - offset];
}



static void trace_cf(const VM_t* vm)
{
    for (int i = 0; i < vm->frame_count; i++)
    {
        const CallFrame_t* frame = &vm->frames[i];
        const ObjFunction_t* fun = frame->closure->fun;
        size_t ins = frame->ip - fun->chunk.code - 1;
        
        fprintf(stderr, "[line %d] in ",
            LineInfo_GetLine(fun->chunk.line_info, ins)
        );

        if (NULL == fun->name)
        {
            fprintf(stderr, "script\n");
        }
        else 
        {
            fprintf(stderr, "%s()\n", fun->name->cstr);
        }
    }
}








static bool call_value(VM_t* vm, Value_t callee, int argc)
{
    if (!IS_OBJ(callee))
        goto error;


    switch (OBJ_TYPE(callee))
    {
    case OBJ_NATIVE: return call_native(vm, AS_NATIVE(callee), argc);
    case OBJ_CLOSURE: return call(vm, AS_CLOSURE(callee), argc);
    default: break;
    }


error:
    runtime_error(vm, "Can only call functions and classes.");
    return false;
}


static bool call(VM_t* vm, ObjClosure_t* closure, int argc)
{
    if (argc != closure->fun->arity)
    {
        runtime_error(vm, "Expected %d arguments, got %d instead.", 
            closure->fun->arity, argc
        );
        return false;
    }

    if (vm->frame_count >= VM_FRAMES_MAX)
    {
        runtime_error(vm, "Call Stack overflow.");
        return false;
    }

    CallFrame_t* current = &vm->frames[vm->frame_count++];
    current->closure = closure;
    current->ip = closure->fun->chunk.code;
    current->base = VM_GET_DATA(vm).sp - argc - 1;
    return true;
}


static bool call_native(VM_t* vm, ObjNativeFn_t* native, int argc)
{
    if (native->arity != argc)
    {
        runtime_error(vm, "Expected %d arguments, got %d instead.", native->arity, argc);
        return false;
    }

    Value_t* base_args = VM_GET_DATA(vm).sp - argc;
    Value_t ret = native->fn(argc, base_args);
    VM_GET_DATA(vm).sp = base_args;
    VM_GET_DATA(vm).sp[-1] = ret;
    return true;
}



static ObjUpval_t* capture_upval(VMData_t* vmdata, Value_t* base)
{
    ObjUpval_t* prev = NULL;
    ObjUpval_t* curr = vmdata->open_upvals;
    while (NULL != curr && curr->location > base)
    {
        prev = curr;
        curr = curr->next;
    }

    if (NULL != curr && base == curr->location)
    {
        return curr;
    }


    ObjUpval_t* new_upval = ObjUpv_Create(vmdata, base);
    new_upval->next = curr;
    if (NULL == prev)
    {
        vmdata->open_upvals = new_upval;
    }
    else
    {
        prev->next = new_upval;
    }
    return new_upval;
}



static void close_upval(VMData_t* vmdata, Value_t* base)
{
    while (NULL != vmdata->open_upvals 
        && vmdata->open_upvals->location >= base)
    {
        ObjUpval_t* upval = vmdata->open_upvals;
        upval->closed = *upval->location;
        upval->location = &upval->closed;
        vmdata->open_upvals = upval->next;
    }
}


