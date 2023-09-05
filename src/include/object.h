#ifndef _CLOX_OBJECT_H_
#define _CLOX_OBJECT_H_


#include "common.h"
#include "value.h"
#include "typedefs.h"
#include "chunk.h"



#define OBJ_TYPE(value)     (AS_OBJ(value)->type)

#define IS_STRING(value)    is_objtype(value, OBJ_STRING)
#define IS_FUNCTION(value)  is_objtype(value, OBJ_FUNCTION)
#define IS_NATIVE(value)    is_objtype(value, OBJ_NATUVE)

#define AS_STR(value)       ((ObjString_t*)AS_OBJ(value))
#define AS_CSTR(value)      (AS_STR(value)->cstr)
#define AS_FUNCTION(value)  ((ObjFunction_t*)AS_OBJ(value))
#define AS_NATIVE(value)    (((ObjNativeFn_t*)AS_OBJ(value)))


typedef enum ObjType_t
{
    OBJ_STRING,
    OBJ_FUNCTION,
    OBJ_NATIVE,
} ObjType_t;

struct Obj_t
{
    ObjType_t type;
    Obj_t* next;
};





typedef Value_t (*NativeFn_t)(int argc, Value_t* argv);
typedef struct ObjNativeFn_t
{
    Obj_t obj;

    int arity;
    NativeFn_t fn; /* this ain't fun */
} ObjNativeFn_t;

typedef struct ObjFunction_t
{
    Obj_t obj;

    int arity;
    Chunk_t chunk;
    ObjString_t* name;
} ObjFunction_t;


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
void Obj_Free(Allocator_t* alloc, Obj_t* obj);





/*
 *  Creates a new ObjNativeFn_t from a C function pointer and its arity (arg count) 
 */
ObjNativeFn_t* ObjNFn_Create(VMData_t* vmdata, NativeFn_t fn, uint8_t arity);

/*
 *  Creates a new ObjFunction_t, cleanup using Obj_Free()
 */
ObjFunction_t* ObjFun_Create(VMData_t* vmdata, line_t line);



/*
 *  Creates a new ObjString_t object by copying the cstr given
 *  \returns a pointer to the newly created ObjString_t object
 */
ObjString_t* ObjStr_Copy(VMData_t* vmdata, const char* cstr, int len);

/*
 *  \returns the hash of multiple strings, the given strings are treated as one
 */
uint32_t ObjStr_HashStrs(int count, const ObjString_t* strings[static count]);


#ifdef OBJSTR_FLEXIBLE_ARR
    /* 
     *  Creates a new ObjString_t object and reserve len + 1 bytes of memory
     *  \returns a pointer to the newly created ObjString_t object
     */
    ObjString_t* ObjStr_Reserve(VMData_t* vmdata, int len);

    /* 
     *  Hashes the given string, and set it in the vmdata's string table if
     *  the string did not exist before
     *  \returns true if the string already exist in the string table
     *  \returns false otherwise
     */
    bool ObjStr_Intern(VMData_t* vmdata, ObjString_t* string);
#else

    /*
     *  steals the pointer to the heapstr, 
     *  if the heapstr already existed in vmdata's string table,
     *      the heapstr is freed and the string in vmdata's string table is returned
     *  else 
     *      allocate a new ObjString_t object, set it in vmdata's string table and then return it
     *  \returns the string in vmdata's string table if it already exist
     *  \returns the newly allocated string otherwise 
     */
    ObjString_t* ObjStr_Steal(VMData_t* vmdata, char* heapstr, int len);
#endif /* OBJSTR_FLEXIBLE_ARR */


/* prints a val to fout stream */
void Obj_Print(FILE* fout, const Value_t val);



static inline bool is_objtype(const Value_t val, ObjType_t type)
{
    return IS_OBJ(val) && (OBJ_TYPE(val) == type);
}



#endif /* _CLOX_OBJECT_H_ */

