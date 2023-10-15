

#include <time.h>
#include <math.h>

#include "include/common.h"
#include "include/object.h"
#include "include/value.h"
#include "include/object.h"
#include "include/vm.h"




static ObjString_t* str_from_array(VM_t* vm, const ValueArr_t* arr, bool recurse);
static ObjString_t* str_from_val(VM_t* vm, Value_t val, bool recurse);
static ObjString_t* str_from_number(VM_t* vm, double number);
static ObjString_t* str_from_obj(VM_t* vm, Value_t val, bool recurse);
static ObjString_t* str_from_fun(VM_t* vm, const ObjFunction_t* fun);
static ObjString_t* str_from_table(VM_t* vm, const Table_t table, bool recurse);



Value_t Native_Clock(VM_t* vm, int argc, Value_t* argv)
{
    (void)argc, (void)argv, (void)vm;
    return NUMBER_VAL((double)clock() / CLOCKS_PER_SEC);
}



Value_t Native_ToStr(VM_t* vm, int argc, Value_t* argv)
{
    (void)argc;
    return OBJ_VAL(str_from_val(vm, argv[0], true));
}


Value_t Native_Array(VM_t* vm, int argc, Value_t* argv)
{
    (void)argc, (void)argv;
    return OBJ_VAL(ObjArr_Create(vm));
}








static ObjString_t* str_from_val(VM_t* vm, Value_t val, bool recurse)
{
    ObjString_t* str = NULL;
    switch (VALTYPE(val))
    {
    case VAL_NUMBER:
        str = str_from_number(vm, AS_NUMBER(val));
        break;

    case VAL_NIL:
        str = vm->native.str.nil;
        break;

    case VAL_BOOL:
        if (AS_BOOL(val))
            str = vm->native.str.true_;
        else 
            str = vm->native.str.false_;
        break;

    case VAL_OBJ:
        str = str_from_obj(vm, val, recurse);
        break;
    }
    return str;
}


static ObjString_t* str_from_array(VM_t* vm, const ValueArr_t* array, bool recurse)
{
    if (!recurse)
        return vm->native.str.array;

    ObjString_t* str = vm->native.array.open_bracket;
    for (size_t i = 0; i < array->size; i++)
    {
        str = VM_StrConcat(vm, str, str_from_val(vm, array->vals[i], false));
        if (i != array->size - 1)
            str = VM_StrConcat(vm, str, vm->native.array.comma);
    }
    return VM_StrConcat(vm, str, vm->native.array.close_bracket);
}


static ObjString_t* str_from_number(VM_t* vm, double number)
{
    char tmp[64];
    int len = snprintf(tmp, sizeof tmp, "%g", number);
    return ObjStr_Copy(vm, tmp, len);
}


static ObjString_t* str_from_obj(VM_t* vm, Value_t val, bool recurse)
{
    switch (OBJ_TYPE(val))
    {
    case OBJ_STRING:
        return AS_STR(val);

    case OBJ_UPVAL:
        return ObjStr_Copy(vm, "upvalue", 7);

    case OBJ_CLASS:
    {
        const ObjClass_t* klass = AS_CLASS(val);
        return VM_StrConcat(vm, 
            klass->name, 
            ObjStr_Copy(vm, " class", 6)
        );
    }
    break;


    case OBJ_INSTANCE:
    {
        const ObjInstance_t* instance = AS_INSTANCE(val);
        ObjString_t* str = ObjStr_Copy(vm, 
            instance->klass->name->cstr, 
            instance->klass->name->len
        );
        VM_Push(vm, OBJ_VAL(str));
        {
            ObjString_t* tmp = ObjStr_Copy(vm, " instance:\n  ", 13);
            str = VM_StrConcat(vm, str, tmp);
            str = VM_StrConcat(vm, 
                str, 
                str_from_table(vm, instance->fields, recurse)
            );
        }
        VM_Pop(vm);
        return str;
    }
    break;

    case OBJ_BOUND_METHOD:
        return str_from_fun(vm, AS_BOUND_METHOD(val)->method->fun);
    case OBJ_CLOSURE:
        return str_from_fun(vm, AS_CLOSURE(val)->fun);
    case OBJ_FUNCTION:
    {
        const ObjFunction_t* fun = AS_FUNCTION(val);
        if (NULL == fun)
            return vm->native.str.script;
        else
            return str_from_fun(vm, fun);
    }
    break;

    case OBJ_NATIVE:
        return vm->native.str.nativefn;

    case OBJ_ARRAY:
        return str_from_array(vm, &AS_ARRAY(val)->array, recurse);
    }

    return vm->native.str.empty;
}




static ObjString_t* str_from_fun(VM_t* vm, const ObjFunction_t* fun)
{
    ObjString_t* str = ObjStr_Copy(vm, "<fn ", 4);
    str = VM_StrConcat(vm, str, fun->name);
    str = VM_StrConcat(vm, str, ObjStr_Copy(vm, ">", 1));
    return str;
}



static ObjString_t* str_from_table(VM_t* vm, const Table_t table, bool recurse)
{
    if (!recurse)
        return vm->native.str.table;

    ObjString_t* str = NULL;
    for (size_t i = 0; i < table.capacity; i++)
    {
        const Entry_t* entry = &table.entries[i];
        if (entry->key == NULL) continue;

        if (NULL == str)
            str = ObjStr_Copy(vm, entry->key->cstr, entry->key->len);
        else 
            str = VM_StrConcat(vm, str, entry->key);
        str = VM_StrConcat(vm, str, ObjStr_Copy(vm, ": ", 2));
        str = VM_StrConcat(vm, str, str_from_val(vm, entry->val, false));
        str = VM_StrConcat(vm, str, ObjStr_Copy(vm, ",\n  ", 4));
    }
    if (NULL == str)
        return vm->native.str.empty;
    return str;
}

