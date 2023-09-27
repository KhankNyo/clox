

#include <stdio.h>
#include <string.h>

#include "include/memory.h"
#include "include/object.h"
#include "include/value.h"
#include "include/vm.h"
#include "include/memory.h"




#define ALLOCATE_OBJ(p_vm, type, objType)\
    (type *)allocate_obj(p_vm, sizeof(type), objType)

#define HASH_BYTE(prev_hash, ch) (((prev_hash) ^ (uint8_t)(ch)) * 16777619)


static void print_function(FILE* fout, const ObjFunction_t* fun);

static ObjString_t* allocate_string(VM_t* vm, char* cstr, int len, uint32_t hash);

static Obj_t* allocate_obj(VM_t* vm, size_t nbytes, ObjType_t type);
static uint32_t hash_str(const char* str, int len);









void Obj_Free(VM_t* vm, Obj_t* obj)
{
    DEBUG_GC_PRINT("%p free object type %d\n", (void*)obj, obj->type);
    switch (obj->type)
    {
    case OBJ_BOUND_METHOD:
    {
        FREE(vm, ObjBoundMethod_t, obj);
    }
    break;

    case OBJ_INSTANCE:
    {
        ObjInstance_t* inst = (ObjInstance_t*)obj;
        Table_Free(&inst->fields);
        FREE(vm, ObjInstance_t, inst);
    }
    break;

    case OBJ_CLASS:
    {
        ObjClass_t* klass = (ObjClass_t*)obj;
        Table_Free(&klass->methods);
        FREE(vm, ObjClass_t, klass);
    }
    break;

    case OBJ_NATIVE:
        FREE(vm, ObjNativeFn_t, obj);
        break;

    case OBJ_FUNCTION:
    {
        ObjFunction_t* fun = (ObjFunction_t*)obj;
        Chunk_Free(&fun->chunk);
        FREE(vm, ObjFunction_t, fun);
    }
    break;

    case OBJ_CLOSURE:
    {
        ObjClosure_t* closure = (ObjClosure_t*)obj;
        FREE_ARRAY(vm, ObjUpval_t*, 
            closure->upvals, closure->upval_count
        );
        FREE(vm, ObjClosure_t, closure);
    }
    break;

    case OBJ_STRING:
    {
        ObjString_t* str = (ObjString_t*)obj;
#ifdef OBJSTR_FLEXIBLE_ARR
        FREE_ARRAY(vm, char, str, sizeof(ObjString_t) + str->len + 1);
#else
        FREE_ARRAY(vm, char, str->cstr, str->len + 1);
        FREE(vm, *str, str);
#endif 
    }
    break;

    case OBJ_UPVAL:
        FREE(vm, ObjUpval_t, obj);
        break;
    }
}








ObjNativeFn_t* ObjNFn_Create(VM_t* vm, NativeFn_t fn, uint8_t arity)
{
    ObjNativeFn_t* native = ALLOCATE_OBJ(vm, ObjNativeFn_t, OBJ_NATIVE);

    native->arity = arity;
    native->fn = fn;
    return native;
}




ObjUpval_t* ObjUpv_Create(VM_t* vm, Value_t* value)
{
    ObjUpval_t* upval = ALLOCATE_OBJ(vm, ObjUpval_t, OBJ_UPVAL);

    upval->closed = NIL_VAL();
    upval->location = value;
    upval->next = NULL;
    return upval;
}



ObjFunction_t* ObjFun_Create(VM_t* vm)
{
    ObjFunction_t* fun = ALLOCATE_OBJ(vm, ObjFunction_t, OBJ_FUNCTION);

    fun->arity = 0;
    fun->upval_count = 0;
    fun->name = NULL;
    Chunk_Init(&fun->chunk, vm);
    return fun;
}



ObjClosure_t* ObjClo_Create(VM_t* vm, ObjFunction_t* fun)
{
    ObjUpval_t** upvals = ALLOCATE(vm, ObjUpval_t*, fun->upval_count);
    VM_Push(vm, OBJ_VAL(upvals));
    for (int i = 0; i < fun->upval_count; i++)
    {
        upvals[i] = NULL; // again fuck the gc
    }

    ObjClosure_t* closure = ALLOCATE_OBJ(vm, ObjClosure_t, OBJ_CLOSURE);
    VM_Pop(vm);

    closure->upvals = upvals;
    closure->upval_count = fun->upval_count;
    closure->fun = fun;
    return closure;
}


ObjClass_t* ObjCla_Create(VM_t* vm, ObjString_t* name)
{
    ObjClass_t* klass = ALLOCATE_OBJ(vm, ObjClass_t, OBJ_CLASS);

    klass->name = name;
    Table_Init(&klass->methods, vm);
    return klass;
}


ObjInstance_t* ObjIns_Create(VM_t* vm, ObjClass_t* klass)
{
    ObjInstance_t* inst = ALLOCATE_OBJ(vm, ObjInstance_t, OBJ_INSTANCE);

    inst->klass = klass;
    Table_Init(&inst->fields, vm);
    return inst;
}


ObjBoundMethod_t* ObjBmd_Create(VM_t* vm, Value_t receiver, ObjClosure_t* closure)
{
    ObjBoundMethod_t* bmd = ALLOCATE_OBJ(vm, ObjBoundMethod_t, OBJ_BOUND_METHOD);

    bmd->receiver = receiver;
    bmd->method = closure;
    return bmd;
}








ObjString_t* ObjStr_Copy(VM_t* vm, const char* cstr, int len)
{
    uint32_t hash = hash_str(cstr, len);
    ObjString_t* string = Table_FindStr(&vm->strings, cstr, len, hash);
    if (NULL != string)
    {
        return string;
    }
    char* buf = NULL;


#ifdef OBJSTR_FLEXIBLE_ARR
    string = ObjStr_Reserve(vm, len);
    buf = string->cstr;
    
    memcpy(buf, cstr, len);
    buf[len] = '\0';
  
    ObjStr_Intern(vm, string);
#else
    buf = ALLOCATE(vm, char, len + 1);
    string = allocate_string(vm, buf, len, hash);
    
    memcpy(buf, cstr, len);
    buf[len] = '\0';
#endif /* OBJSTR_FLEXIBLE_ARR */

    return string;
}


uint32_t ObjStr_HashStrs(int count, const ObjString_t* strings[])
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










ObjString_t* ObjStr_Reserve(VM_t* vm, int len)
{
    ObjString_t* string;
#ifdef OBJSTR_FLEXIBLE_ARR
    string = (ObjString_t*)allocate_obj(
        vm, sizeof(*string) + len + 1, OBJ_STRING
    );
#else
    string = ALLOCATE_OBJ(vm, ObjString_t, OBJ_STRING);
    string->cstr = ALLOCATE(vm, char, len + 1);
#endif /* OBJSTR_FLEXIBLE_ARR */

    string->len = len;
    return string;
}

bool ObjStr_Intern(VM_t* vm, ObjString_t* string)
{
    string->hash = hash_str(string->cstr, string->len);
    bool already_there = Table_Set(&vm->strings, string, NIL_VAL());
    return already_there;
}



ObjString_t* ObjStr_Steal(VM_t* vm, char* heapstr, int len)
{
    uint32_t hash = hash_str(heapstr, len);
    ObjString_t* interned = Table_FindStr(&vm->strings, heapstr, len, hash);
    if (NULL != interned)
    {
        FREE_ARRAY(vm, *heapstr, heapstr, len + 1);
        return interned;
    }

    ObjString_t* string = allocate_string(vm, heapstr, len, hash);
    Table_Set(&vm->strings, string, NIL_VAL());
    return string;
}




void Obj_Print(FILE* fout, const Value_t val)
{
    switch (OBJ_TYPE(val))
    {
    case OBJ_BOUND_METHOD:
        print_function(fout, AS_BOUND_METHOD(val)->method->fun);
        break;
        
    case OBJ_INSTANCE:
        fprintf(fout, "%s instance", AS_INSTANCE(val)->klass->name->cstr);
        break;

    case OBJ_CLASS:
        fprintf(fout, "%s", AS_CLASS(val)->name->cstr);
        break;

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

    case OBJ_UPVAL:
        fprintf(fout, "upvalue");
        break;
    }
}











static void print_function(FILE* fout, const ObjFunction_t* fun)
{
    if (fun->name == NULL)
        fprintf(fout, "<script>");
    else
        fprintf(fout, "<fn %s>", fun->name->cstr);
}




static ObjString_t* allocate_string(VM_t* vm, char* cstr, int len, uint32_t hash)
{
    ObjString_t* string;

#ifdef OBJSTR_FLEXIBLE_ARR
    string = ObjStr_Reserve(vm, len);
    memcpy(string->cstr, cstr, len);
    string->cstr[len] = '\0';
    FREE_ARRAY(vm, char, cstr, len + 1);
#else
    string = ALLOCATE_OBJ(vm, ObjString_t, OBJ_STRING);
    string->cstr = cstr;
#endif /* OBJSTR_FLEXIBLE_ARR */

    string->len = len;
    string->hash = hash;

    Table_Set(&vm->strings, string, NIL_VAL());
    return string;
}



static Obj_t* allocate_obj(VM_t* vm, size_t nbytes, ObjType_t type)
{
    Obj_t* obj = ALLOCATE(vm, uint8_t, nbytes);
    obj->type = type;
    obj->is_marked = false;

    obj->next = vm->head;
    vm->head = obj;
    DEBUG_GC_PRINT("%p allocate %zu for %d\n", (void*)obj, nbytes, type);
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




