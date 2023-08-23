

#include <stdio.h>
#include "include/common.h"
#include "include/vm.h"
#include "include/debug.h"




static InterpretResult_t run(VM_t* vm);
static void stack_reset(VM_t* vm);




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



InterpretResult_t VM_Interpret(VM_t* vm, const char* src, size_t src_size)
{
    Compile(src, src_size);
    return INTERPRET_OK;
}














static InterpretResult_t run(VM_t* vm)
{
#define READ_BYTE() (*vm->ip++)
#define READ_CONSTANT() (vm->chunk->consts.vals[READ_BYTE()])
#define BINARY_OP(op) \
do{\
    Value_t b = VM_Pop(vm);\
    Value_t a = VM_Pop(vm);\
    VM_Push(vm, a op b);\
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
            VM_Push(vm, constant);
        }
        break;

        case OP_NEGATE:    VM_Push(vm, -VM_Pop(vm)); break;
		case OP_ADD:       BINARY_OP(+); break;
        case OP_SUBTRACT:  BINARY_OP(-); break;
        case OP_MULTIPLY:  BINARY_OP(*); break;
        case OP_DIVIDE:    BINARY_OP(/); break;


        case OP_RETURN:
        {
            VM_Pop(vm);
            //printVal(stderr, VM_Pop(vm));
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








