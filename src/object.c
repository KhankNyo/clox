

#include <stdio.h>
#include <string.h>

#include "include/memory.h"
#include "include/object.h"
#include "include/value.h"
#include "include/vm.h"
#include "include/memory.h"




#define ALLOCATE_OBJ(p_vmdata, type, objType)\
    (type *)allocate_obj(p_vmdata, sizeof(type), objType)

#define HASH_BYTE(prev_hash, ch) (((prev_hash) ^ (uint8_t)(ch)) * 16777619)


static void print_function(FILE* fout, const ObjFunction_t* fun);

#ifndef OBJSTR_FLEXIBLE_ARR
static ObjString_t* allocate_string(VMData_t* vmdata, char* cstr, int len, uint32_t hash);
#endif /* OBJSTR_FLEXIBLE_ARR */

static Obj_t* allocate_obj(VMData_t* vmdata, size_t nbytes, ObjType_t type);
static uint32_t hash_str(const char* str, int len);









void Obj_Free(Allocator_t* alloc, Obj_t* obj)
{
    if (NULL == obj)
    {
        return;
    }


    switch (obj->type)
    {
    case OBJ_NATIVE:
        FREE(alloc, ObjNativeFn_t, obj);
        break;

    case OBJ_FUNCTION:
    {
        ObjFunction_t* fun = (ObjFunction_t*)obj;
        Chunk_Free(&fun->chunk);
        FREE(alloc, ObjFunction_t, fun);
    }
    break;

    case OBJ_CLOSURE:
        FREE(alloc, ObjClosure_t, obj);
        break;

    case OBJ_STRING:
    {
        ObjString_t* str = (ObjString_t*)obj;
#ifndef OBJSTR_FLEXIBLE_ARR
        FREE_ARRAY(alloc, *str->cstr, str->cstr, str->len + 1);
#endif 
        FREE(alloc, *str, str);
    }
    break;

    default: CLOX_ASSERT(false && "Unhandled Obj_Free() case"); return;
    }
}








ObjNativeFn_t* ObjNFn_Create(VMData_t* vmdata, NativeFn_t fn, uint8_t arity)
{
    ObjNativeFn_t* native = ALLOCATE_OBJ(vmdata, ObjNativeFn_t, OBJ_NATIVE);

    native->arity = arity;
    native->fn = fn;
    return native;
}



ObjFunction_t* ObjFun_Create(VMData_t* vmdata, line_t line)
{
    ObjFunction_t* fun = ALLOCATE_OBJ(vmdata, ObjFunction_t, OBJ_FUNCTION);
    fun->arity = 0;
    fun->name = NULL;
    Chunk_Init(&fun->chunk, vmdata->alloc, line);
    return fun;
}


ObjClosure_t* ObjCls_Create(VMData_t* vmdata, ObjFunction_t* fun)
{
    ObjClosure_t* closure = ALLOCATE_OBJ(vmdata, ObjClosure_t, OBJ_CLOSURE);
    closure->fun = fun;
    return closure;
}





ObjString_t* ObjStr_Copy(VMData_t* vmdata, const char* cstr, int len)
{
    uint32_t hash = hash_str(cstr, len);
    ObjString_t* string = Table_FindStr(&vmdata->strings, cstr, len, hash);
    if (NULL != string)
    {
        return string;
    }
    char* buf = NULL;


#ifdef OBJSTR_FLEXIBLE_ARR
    string = ObjStr_Reserve(vmdata, len);
    buf = string->cstr;
    
    memcpy(buf, cstr, len);
    buf[len] = '\0';
  
    ObjStr_Intern(vmdata, string);
#else
    buf = ALLOCATE(vmdata->alloc, char, len + 1);
    string = allocate_string(vmdata, buf, len, hash);
    
    memcpy(buf, cstr, len);
    buf[len] = '\0';
#endif /* OBJSTR_FLEXIBLE_ARR */

    return string;
}


uint32_t ObjStr_HashStrs(int count, const ObjString_t* strings[static count])
{
    uint32_t hash = 2166136261u;
    for (int i = 0; i < count; i++)
    {
        for (int j = 0; j < strings[i]->len; j++)
        {
            hash = HASH_BYTE(hash, strings[i]->cstr[j]);
        }
    }
    return hash;
}









#ifdef OBJSTR_FLEXIBLE_ARR

ObjString_t* ObjStr_Reserve(VMData_t* vmdata, int len)
{
    ObjString_t* string = (ObjString_t*)allocate_obj(
            vmdata, sizeof(*string) + len + 1, OBJ_STRING
    );
    string->len = len;
    return string;
}

bool ObjStr_Intern(VMData_t* vmdata, ObjString_t* string)
{
    string->hash = hash_str(string->cstr, string->len);
    return Table_Set(&vmdata->strings, string, NIL_VAL());
}


#else

ObjString_t* ObjStr_Steal(VMData_t* vmdata, char* heapstr, int len)
{
    uint32_t hash = hash_str(heapstr, len);
    ObjString_t* interned = Table_FindStr(&vmdata->strings, heapstr, len, hash);
    if (NULL != interned)
    {
        FREE_ARRAY(vmdata->alloc, *heapstr, heapstr, len + 1);
        return interned;
    }

    ObjString_t* string = allocate_string(vmdata, heapstr, len, hash);
    Table_Set(&vmdata->strings, string, NIL_VAL());
    return string;
}


#endif /* OBJSTR_FLEXIBLE_ARR */


void Obj_Print(FILE* fout, const Value_t val)
{
    switch (OBJ_TYPE(val))
    {
    case OBJ_NATIVE:
        fprintf(fout, "<native fn>");
        break;

    case OBJ_FUNCTION:
        print_function(fout, AS_FUNCTION(val));
        break;
    
    case OBJ_CLOSURE:
        print_function(fout, AS_CLOSURE(val)->fun);
        break;

    case OBJ_STRING:
        fprintf(fout, "%s", AS_CSTR(val));
        break;
    default: CLOX_ASSERT(false && "Unhandled Obj_Print() case"); break;
    }
}











static void print_function(FILE* fout, const ObjFunction_t* fun)
{
    if (fun->name == NULL)
        fprintf(fout, "<script>");
    else
        fprintf(fout, "<fn %s>", fun->name->cstr);
}



#if !defined(OBJSTR_FLEXIBLE_ARR)

static ObjString_t* allocate_string(VMData_t* vmdata, char* cstr, int len, uint32_t hash)
{
    ObjString_t* string = ALLOCATE_OBJ(vmdata, ObjString_t, OBJ_STRING);
    string->cstr = cstr;
    string->len = len;
    string->hash = hash;

    Table_Set(&vmdata->strings, string, NIL_VAL());
    return string;
}

#endif /* OBJSTR_FLEXIBLE_ARR */


static Obj_t* allocate_obj(VMData_t* vmdata, size_t nbytes, ObjType_t type)
{
    Obj_t* obj = Allocator_Alloc(vmdata->alloc, nbytes);
    obj->type = type;

    obj->next = vmdata->head;
    vmdata->head = obj;
    
    return obj;
}


/* FNV-1a hashing */
static uint32_t hash_str(const char* str, int len)
{
    uint32_t hash = 2166136261u;
    for (int i = 0; i < len; i++)
    {
        hash = HASH_BYTE(hash, str[i]);
    }
    return hash;
}




