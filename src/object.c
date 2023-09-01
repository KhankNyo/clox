

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
    case OBJ_STRING:
        fprintf(fout, "%s", AS_CSTR(val));
        break;
    default: CLOX_ASSERT(false && "Unhandled Obj_Print() case"); break;
    }
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




