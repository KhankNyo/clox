

#include <stdio.h>
#include <string.h>

#include "include/memory.h"
#include "include/object.h"
#include "include/value.h"
#include "include/vm.h"
#include "include/memory.h"




#define ALLOCATE_OBJ(pp_head, p_allocator, type, objType)\
    (type *)allocate_obj(pp_head, p_allocator, sizeof(type), objType)


#ifndef OBJSTR_FLEXIBLE_ARR
static ObjString_t* allocate_string(Obj_t** head, Allocator_t* alloc, char* cstr, int len);
#endif /* OBJSTR_FLEXIBLE_ARR */

static Obj_t* allocate_obj(Obj_t** head, Allocator_t* alloc, size_t nbytes, ObjType_t type);











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






ObjString_t* ObjStr_Copy(Obj_t** head, Allocator_t* alloc, const char* cstr, int len)
{
    ObjString_t* string = NULL;
    char* buf = NULL;


#ifdef OBJSTR_FLEXIBLE_ARR
    string = ObjStr_Reserve(head, alloc, len);
    buf = string->cstr;
#else
    buf = ALLOCATE(alloc, char, len + 1);
    string = allocate_string(head, alloc, buf, len);
#endif /* OBJSTR_FLEXIBLE_ARR */


    memcpy(buf, cstr, len);
    buf[len] = '\0';
    return string;
}




#ifdef OBJSTR_FLEXIBLE_ARR

ObjString_t* ObjStr_Reserve(Obj_t** head, Allocator_t* alloc, int len)
{
    ObjString_t* string = (ObjString_t*)allocate_obj(head, alloc, sizeof(*string) + len + 1, OBJ_STRING);
    string->len = len;
    return string;
}

#else

ObjString_t* ObjStr_Steal(Obj_t** head, Allocator_t* alloc, char* heapstr, int len)
{
    return allocate_string(head, alloc, heapstr, len);
}


#endif /* OBJSTR_FLEXIBLE_ARR */


void Obj_Print(const Value_t val)
{
    switch (OBJ_TYPE(val))
    {
    case OBJ_STRING:
        printf("%s", AS_CSTR(val));
        break;
    default: CLOX_ASSERT(false && "Unhandled Obj_Print() case"); break;
    }
}











#if !defined(OBJSTR_FLEXIBLE_ARR)

static ObjString_t* allocate_string(Obj_t** head, Allocator_t* alloc, char* cstr, int len)
{
    ObjString_t* string = ALLOCATE_OBJ(head, alloc, ObjString_t, OBJ_STRING);
    string->cstr = cstr;
    string->len = len;
    return string;
}

#endif /* OBJSTR_FLEXIBLE_ARR */


static Obj_t* allocate_obj(Obj_t** head, Allocator_t* alloc, size_t nbytes, ObjType_t type)
{
    Obj_t* obj = Allocator_Alloc(alloc, nbytes);
    obj->type = type;

    obj->next = *head;
    *head = obj;
    
    return obj;
}

