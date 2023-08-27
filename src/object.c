

#include <stdio.h>
#include <string.h>

#include "include/memory.h"
#include "include/object.h"
#include "include/value.h"
#include "include/vm.h"
#include "include/memory.h"




#define ALLOCATE_OBJ(pp_head, p_allocator, type, objType)\
    (type *)allocate_obj(pp_head, p_allocator, sizeof(type), objType)


static ObjString_t* allocate_string(Obj_t** head, Allocator_t* alloc, char* cstr, int len);
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
        FREE_ARRAY(alloc, *str->cstr, str->cstr, str->len + 1);
        FREE(alloc, *str, str);
    }
    break;

    default: CLOX_ASSERT(false && "Unhandled Obj_Free() case"); return;
    }
}






ObjString_t* ObjStr_Copy(Obj_t** head, Allocator_t* alloc, const char* cstr, int len)
{
    char* buf = ALLOCATE(alloc, char, len + 1);

    memcpy(buf, cstr, len);
    buf[len] = '\0';
    return allocate_string(head, alloc, buf, len);
}


ObjString_t* ObjStr_Steal(Obj_t** head, Allocator_t* alloc, char* heapstr, int len)
{
    return allocate_string(head, alloc, heapstr, len);
}


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












static ObjString_t* allocate_string(Obj_t** head, Allocator_t* alloc, char* cstr, int len)
{
    ObjString_t* string = ALLOCATE_OBJ(head, alloc, ObjString_t, OBJ_STRING);
    string->cstr = cstr;
    string->len = len;
    return string;
}



static Obj_t* allocate_obj(Obj_t** head, Allocator_t* alloc, size_t nbytes, ObjType_t type)
{
    Obj_t* obj = Allocator_Alloc(alloc, nbytes);
    obj->type = type;

    obj->next = *head;
    *head = obj;
    
    return obj;
}

