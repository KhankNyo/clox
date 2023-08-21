#ifndef _CLOX_VM_H_
#define _CLOX_VM_H_


#include "common.h"
#include "chunk.h"

typedef struct VM_t
{
    Chunk_t* chunk;
} VM_t;

typedef enum InterpretResult_t
{
    INTERPRET_OK = 0,
    INTERPRET_COMPILE_ERROR,
    INTERPRET_RUNTIME_ERROR,
} InterpretResult_t;


void VM_Init(VM_t* vm);

InterpretResult_t VM_Interpret(Chunk_t* chunk);

void VM_Free(VM_t* vm);


#endif /* _CLOX_VM_H_ */

