#ifndef _CLOX_VM_H_
#define _CLOX_VM_H_


#include "common.h"
#include "value.h"
#include "memory.h"
#include "table.h"
#include "object.h"
#include "compiler.h"


typedef enum InterpretResult_t
{
    INTERPRET_OK = 0,
    INTERPRET_COMPILE_ERROR,
    INTERPRET_RUNTIME_ERROR,
} InterpretResult_t;


#define VM_STACK_MAX (UINT8_COUNT * VM_FRAMES_MAX)

typedef struct CallFrame_t
{
    ObjClosure_t* closure;
    uint8_t* ip;
    Value_t* bp;
} CallFrame_t;


struct VM_t
{
    Allocator_t* alloc;
    Table_t strings;
    Table_t globals;
    ObjUpval_t* open_upvals;
    Obj_t* head;
    ObjString_t* init_str;

    int gray_count;
    int gray_capacity;
    Obj_t** gray_stack;

    size_t bytes_allocated;
    size_t next_gc;

    Value_t* sp;
    int frame_count;

    Value_t stack[VM_STACK_MAX];
    CallFrame_t frames[VM_FRAMES_MAX];
    Compiler_t* compiler;
};






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


/* print all elems in the vm's stack */
void VM_PrintStack(FILE* fout, const VM_t* vm);


/*
 *  defines a C function that can interface with clox
 *  \returns true on success, 
 *  \returns false on failure
 */
bool VM_DefineNative(VM_t* vm, const char* name, NativeFn_t fn, uint8_t argc);



/* concatenate a with b and intern the result 
 *  \returns the concatenation of a and b
 *
 *  NOTE: a and b will be pushed onto the stack as a precaution against the gc
 */
ObjString_t* VM_StrConcat(VM_t* vm, const ObjString_t* a, const ObjString_t* b);


/* 
 *  interprets clox src code 
 *  \returns INTERPRET_RUNTIME_ERROR if a runtime error was encountered
 *  \returns INTERPRET_COMPILE_ERROR if a compilation error was encountered
 *  \returns INTERPRET_OK if no errors were encountered
 */
InterpretResult_t VM_Interpret(VM_t* vm, const char* src);



/* free VMData_t, automatically called by VM_Free */
void VM_FreeObjects(VM_t* data);


#endif /* _CLOX_VM_H_ */

