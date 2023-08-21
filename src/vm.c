

#include <stdio.h>
#include "include/common.h"
#include "include/vm.h"
#include "include/debug.h"




static InterpretResult_t run(VM_t* vm);
static void stack_reset(VM_t* vm);
static void push(VM_t* vm, Value_t val);
static Value_t pop(VM_t* vm);




void VM_Init(VM_t* vm)
{
    vm->chunk = NULL;
    stack_reset(vm);
}



void VM_Free(VM_t* vm)
{
    VM_Init(vm);
}



InterpretResult_t VM_Interpret(VM_t* vm, Chunk_t* chunk)
{
    vm->chunk = chunk;
    vm->ip = chunk->code;
    return run(vm);
}




static InterpretResult_t run(VM_t* vm)
{
#define READ_BYTE() (*vm->ip++)
#define READ_CONSTANT() (vm->chunk->consts.vals[READ_BYTE()])
#define BINARY_OP(op) \
do{\
    Value_t b = pop(vm);\
    Value_t a = pop(vm);\
    push(vm, a op b);\
}while(0)


    while (1)
    {

#ifdef DEBUG_TRACE_EXECUTION
        printf("          ");
        for (Value_t* slot = vm->stack; slot < vm->sp; slot++) {
            printf("[ ");
            printVal(stderr, *slot);
            printf(" ]");
        }
        printf("\n");
        Disasm_Instruction(stderr, vm->chunk, vm->ip - vm->chunk->code);
#endif /* DEBUG_TRACE_EXECUTION */

        uint8_t ins = READ_BYTE();
        switch (ins)
        {

        case OP_CONSTANT:
        {
            const Value_t constant = READ_CONSTANT();
            push(vm, constant);
        }
        break;

        case OP_NEGATE:    *(vm->sp - 1) = -*(vm->sp - 1); break;
        case OP_ADD:       BINARY_OP(+); break;
        case OP_SUBTRACT:  BINARY_OP(-); break;
        case OP_MULTIPLY:  BINARY_OP(*); break;
        case OP_DIVIDE:    BINARY_OP(/); break;


        case OP_RETURN:
        {
            pop(vm);
            //printVal(stderr, pop(vm));
            //puts("");
            return INTERPRET_OK;
        }


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


static void push(VM_t* vm, Value_t val)
{
    *vm->sp = val;
    vm->sp++;
}


static Value_t pop(VM_t* vm)
{
    vm->sp--;
    return *vm->sp;
}





