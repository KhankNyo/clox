#ifndef _CLOX_VMDATA_H_ 
#define _CLOX_VMDATA_H_


#include "memory.h"
#include "table.h"
#include "object.h"
#include "compiler.h"

#define VM_STACK_MAX (UINT8_COUNT * VM_FRAMES_MAX)



typedef struct CallFrame_t
{
    ObjClosure_t* closure;
    uint8_t* ip;
    Value_t* base;
} CallFrame_t;


struct VMData_t
{
    Allocator_t* alloc;
    Table_t strings;
    Table_t globals;
    ObjUpval_t* open_upvals;
    Obj_t* head;

    int gray_count;
    int gray_capacity;
    Obj_t** gray_stack;
    Compiler_t* compiler;

    Value_t* sp;
    int frame_count;

    Value_t stack[VM_STACK_MAX];
    CallFrame_t frames[VM_FRAMES_MAX];
};

#endif /* _CLOX_VMDATA_H_ */


