#ifndef _CLOX_VM_H_
#define _CLOX_VM_H_


#include "common.h"
#include "value.h"
#include "memory.h"
#include "table.h"
#include "object.h"

#define VM_FRAMES_MAX 128
#define VM_STACK_MAX (VM_FRAMES_MAX*UINT8_COUNT)

struct VMData_t
{
    Allocator_t* alloc;
    Table_t strings;
    Table_t globals;
    Obj_t* head;
};



typedef struct CallFrame_t
{
    ObjClosure_t* closure;
    uint8_t* ip;
    Value_t* base;
} CallFrame_t;

typedef struct VM_t
{
    int frame_count;
    Value_t* sp;

    CallFrame_t frames[VM_FRAMES_MAX];
    Value_t stack[VM_STACK_MAX];

    VMData_t data;
} VM_t;

typedef enum InterpretResult_t
{
    INTERPRET_OK = 0,
    INTERPRET_COMPILE_ERROR,
    INTERPRET_RUNTIME_ERROR,
} InterpretResult_t;





/* initializes vm state and runtime objects like native functions */
void VM_Init(VM_t* vm, Allocator_t* alloc);
/* intializes vm's states only */
void VM_Reset(VM_t* vm);
/* free vm's data */
void VM_Free(VM_t* vm);


/* pushes a value onto the vm's stack 
 *  \returns true on success, 
 *  \returns false if the stack is full
 */
bool VM_Push(VM_t* vm, Value_t val);

/* pops a value off the vm's stack */
Value_t VM_Pop(VM_t* vm);


/*
 *  defines a C function that can interface with clox
 *  \returns true on success, 
 *  \returns false on failure
 */
bool VM_DefineNative(VM_t* vm, const char* name, NativeFn_t fn, uint8_t argc);



/* 
 *  interprets clox src code 
 *  \returns INTERPRET_RUNTIME_ERROR if a runtime error was encountered
 *  \returns INTERPRET_COMPILE_ERROR if a compilation error was encountered
 *  \returns INTERPRET_OK if no errors were encountered
 */
InterpretResult_t VM_Interpret(VM_t* vm, const char* src);



/* free VMData_t, automatically called by VM_Free */
void VM_FreeObjects(VMData_t* data);


#endif /* _CLOX_VM_H_ */

