#ifndef _CLOX_OBJECT_H_
#define _CLOX_OBJECT_H_


#include "common.h"
#include "value.h"
#include "typedefs.h"
#include "chunk.h"
#include "table.h"


#define OBJ_TYPE(value)     (AS_OBJ(value)->type)

#define IS_STRING(value)    is_objtype(value, OBJ_STRING)
#define IS_FUNCTION(value)  is_objtype(value, OBJ_FUNCTION)
#define IS_CLOSURE(value)   is_objtype(value, OBJ_CLOSURE)
#define IS_NATIVE(value)    is_objtype(value, OBJ_NATUVE)
#define IS_CLASS(value)     is_objtype(value, OBJ_CLASS)
#define IS_INSTANCE(value)  is_objtype(value, OBJ_INSTANCE)

#define AS_STR(value)       ((ObjString_t*)AS_OBJ(value))
#define AS_CSTR(value)      (AS_STR(value)->cstr)
#define AS_FUNCTION(value)  ((ObjFunction_t*)AS_OBJ(value))
#define AS_CLOSURE(value)   ((ObjClosure_t*)AS_OBJ(value))
#define AS_NATIVE(value)    ((ObjNativeFn_t*)AS_OBJ(value))
#define AS_CLASS(value)     ((ObjClass_t*)AS_OBJ(value))
#define AS_INSTANCE(value)  ((ObjInstance_t*)AS_OBJ(value))


typedef enum ObjType_t
{
    OBJ_UPVAL,
    OBJ_STRING,
    OBJ_FUNCTION,
    OBJ_CLOSURE,
    OBJ_NATIVE,
    OBJ_CLASS,
    OBJ_INSTANCE,
} ObjType_t;

struct Obj_t
{
    ObjType_t type;
    bool is_marked;
    Obj_t* next;
};






typedef struct ObjClass_t
{
    Obj_t obj;

    ObjString_t* name;
} ObjClass_t;


typedef struct ObjInstance_t
{
    Obj_t obj;

    ObjClass_t* klass;
    Table_t fields;
} ObjInstance_t;



typedef Value_t (*NativeFn_t)(int argc, Value_t* argv);
typedef struct ObjNativeFn_t
{
    Obj_t obj;

    int arity;
    NativeFn_t fn; /* this ain't fun */
} ObjNativeFn_t;



typedef struct ObjUpval_t
{
    Obj_t obj;

    Value_t* location;
    Value_t closed;
    struct ObjUpval_t* next;
} ObjUpval_t;

typedef struct ObjFunction_t
{
    Obj_t obj;

    int arity;
    int upval_count;
    Chunk_t chunk;
    ObjString_t* name;
} ObjFunction_t;

typedef struct ObjClosure_t
{
    Obj_t obj;

    ObjFunction_t* fun;
    ObjUpval_t** upvals;
    int upval_count; // fuck gc
} ObjClosure_t;



struct ObjString_t
{
    Obj_t obj;
    int len;
    uint32_t hash;
#ifdef OBJSTR_FLEXIBLE_ARR
    char cstr[];
#else
    char* cstr;
#endif /* OBJSTR_FLEXIBLE_ARR */
};



/* 
 *  Free the obj pointer and the underlying object iself
 */
void Obj_Free(VM_t* vm, Obj_t* obj);





/*
 *  Creates a new ObjNativeFn_t from a C function pointer and its arity (arg count),
 *  cleanup using Obj_Free()
 */
ObjNativeFn_t* ObjNFn_Create(VM_t* vm, NativeFn_t fn, uint8_t arity);


/*
 *  Creates a new ObjUpval_t from a value on the vm's stack
 *  cleanup using Obj_Free()
 */
ObjUpval_t* ObjUpv_Create(VM_t* vm, Value_t* value);

/*
 *  Creates a new ObjFunction_t, cleanup using Obj_Free()
 */
ObjFunction_t* ObjFun_Create(VM_t* vm);

/*
 *  wraps a function around a closure object, cleanup using Obj_Free()
 */
ObjClosure_t* ObjClo_Create(VM_t* vm, ObjFunction_t* fun);

/*
 *  Creates a new class object
 */
ObjClass_t* ObjCla_Create(VM_t* vm, ObjString_t* name);


/*
 *  Creates a new instance of a class
 */
ObjInstance_t* ObjIns_Create(VM_t* vm, ObjClass_t* klass);




/*
 *  Creates a new ObjString_t object by copying the cstr given
 *  \returns a pointer to the newly created ObjString_t object
 */
ObjString_t* ObjStr_Copy(VM_t* vm, const char* cstr, int len);

/*
 *  \returns the hash of multiple strings, the given strings are treated as one
 */
uint32_t ObjStr_HashStrs(int count, const ObjString_t* strings[static count]);


#ifdef OBJSTR_FLEXIBLE_ARR
    /* 
     *  Creates a new ObjString_t object and reserve len + 1 bytes of memory
     *  \returns a pointer to the newly created ObjString_t object
     */
    ObjString_t* ObjStr_Reserve(VM_t* vm, int len);

    /* 
     *  Hashes the given string, and set it in the vm's string table if
     *  the string did not exist before
     *  \returns true if the string already exist in the string table
     *  \returns false otherwise
     */
    bool ObjStr_Intern(VM_t* vm, ObjString_t* string);
#else

    /*
     *  steals the pointer to the heapstr, 
     *  if the heapstr already existed in vm's string table,
     *      the heapstr is freed and the string in vm's string table is returned
     *  else 
     *      allocate a new ObjString_t object, set it in vm's string table and then return it
     *  \returns the string in vm's string table if it already exist
     *  \returns the newly allocated string otherwise 
     */
    ObjString_t* ObjStr_Steal(VM_t* vm, char* heapstr, int len);
#endif /* OBJSTR_FLEXIBLE_ARR */


/* prints a val to fout stream */
void Obj_Print(FILE* fout, const Value_t val);



static inline bool is_objtype(const Value_t val, ObjType_t type)
{
    return IS_OBJ(val) && (OBJ_TYPE(val) == type);
}



#endif /* _CLOX_OBJECT_H_ */

