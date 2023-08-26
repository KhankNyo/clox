#ifndef _CLOX_VM_H_
#define _CLOX_VM_H_


#include "common.h"
#include "chunk.h"
#include "value.h"
#include "memory.h"


#define VM_STACK_MAX 256


typedef struct VM_t
{
    Chunk_t* chunk;
    uint8_t* ip;
    Value_t stack[VM_STACK_MAX];
    Value_t* sp;
} VM_t;

typedef enum InterpretResult_t
{
    INTERPRET_OK = 0,
    INTERPRET_COMPILE_ERROR,
    INTERPRET_RUNTIME_ERROR,
} InterpretResult_t;


void VM_Init(VM_t* vm);
void VM_Free(VM_t* vm);

void VM_Push(VM_t* vm, Value_t val);
Value_t VM_Pop(VM_t* vm);

InterpretResult_t VM_Interpret(VM_t* vm, Allocator_t* alloc, const char* src);


#endif /* _CLOX_VM_H_ */

