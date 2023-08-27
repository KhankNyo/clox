#ifndef _CLOX_OBJECT_H_
#define _CLOX_OBJECT_H_


#include "common.h"
#include "value.h"



#define OBJ_TYPE(value)     (AS_OBJ(value)->type)
#define IS_STRING(value)    is_objtype(value, OBJ_STRING)

#define AS_STR(value)       ((ObjString_t*)AS_OBJ(value))
#define AS_CSTR(value)      (AS_STR(value)->cstr)

typedef enum ObjType_t
{
    OBJ_STRING,
} ObjType_t;

struct Obj_t
{
    ObjType_t type;
    Obj_t* next;
};





struct ObjString_t
{
    Obj_t obj;
    int len;
#ifdef OBJSTR_FLEXIBLE_ARR
    char cstr[];
#else
    char* cstr;
#endif /* OBJSTR_FLEXIBLE_ARR */
};




void Obj_Free(Allocator_t* alloc, Obj_t* obj);


ObjString_t* ObjStr_Copy(Obj_t** head, Allocator_t* alloc, const char* cstr, int len);
#ifdef OBJSTR_FLEXIBLE_ARR
ObjString_t* ObjStr_Reserve(Obj_t** head, Allocator_t* alloc, int len);
#else
ObjString_t* ObjStr_Steal(Obj_t** head, Allocator_t* alloc, char* heapstr, int len);
#endif /* OBJSTR_FLEXIBLE_ARR */

void Obj_Print(const Value_t val);



static inline bool is_objtype(const Value_t val, ObjType_t type)
{
    return IS_OBJ(val) && (OBJ_TYPE(val) == type);
}



#endif /* _CLOX_OBJECT_H_ */

