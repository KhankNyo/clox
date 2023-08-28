#ifndef _CLOX_VM_H_
#define _CLOX_VM_H_


#include "common.h"
#include "chunk.h"
#include "value.h"
#include "memory.h"
#include "table.h"

#define VM_STACK_MAX 256


struct VMData_t
{
    Allocator_t* alloc;
    Table_t strings;
    Obj_t* head;
};

typedef struct VM_t
{
    Chunk_t* chunk;
    uint8_t* ip;
    Value_t stack[VM_STACK_MAX];
    Value_t* sp;

    VMData_t data;
} VM_t;

typedef enum InterpretResult_t
{
    INTERPRET_OK = 0,
    INTERPRET_COMPILE_ERROR,
    INTERPRET_RUNTIME_ERROR,
} InterpretResult_t;


/* initializes vm */
void VM_Init(VM_t* vm, Allocator_t* alloc);
/* free vm's data */
void VM_Free(VM_t* vm);


/* pushes a value onto the vm's stack */
void VM_Push(VM_t* vm, Value_t val);

/* pops a value off the vm's stack */
Value_t VM_Pop(VM_t* vm);



/* 
 *  interprets clox src code 
 *  \returns INTERPRET_RUNTIME_ERROR if a runtime error was encountered
 *  \returns INTERPRET_COMPILE_ERROR if a compilation error was encountered
 *  \returns INTERPRET_OK if no errors were encountered
 */
InterpretResult_t VM_Interpret(VM_t* vm, Allocator_t* alloc, const char* src);



/* free VMData_t, automatically called by VM_Free */
void VM_FreeObjects(VMData_t* data);


#endif /* _CLOX_VM_H_ */

