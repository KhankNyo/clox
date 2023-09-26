

#include <float.h>
#include <string.h>

#include "include/common.h"
#include "include/memory.h"
#include "include/value.h"
#include "include/object.h"
#include "include/vm.h"




void ValArr_Init(ValueArr_t* valarr, VM_t* vm)
{
	valarr->size = 0;
	valarr->capacity = 0;
	valarr->vals = NULL;
	valarr->vm = vm;
}


void ValArr_Write(ValueArr_t* valarr, Value_t val)
{
	if (valarr->size + 1 > valarr->capacity)
	{
        VM_Push(valarr->vm, val);

		const size_t oldcap = valarr->capacity;
		valarr->capacity = GROW_CAPACITY(valarr->capacity);
		valarr->vals = GROW_ARRAY(valarr->vm, Value_t, 
			valarr->vals, oldcap, valarr->capacity
		);

        VM_Pop(valarr->vm);
	}

	valarr->vals[valarr->size] = val;
	valarr->size++;
}




bool ValArr_Find(const ValueArr_t* valarr, Value_t val, size_t* index_out)
{
    size_t i = 0; 
    for (; i < valarr->size; i++)
    {
        if (Value_Equal(val, valarr->vals[i]))
        {
            *index_out = i;
            return true;
        }
    }
    return i != valarr->size;
}




void ValArr_Free(ValueArr_t* valarr)
{
	FREE_ARRAY(valarr->vm, Value_t, valarr->vals, valarr->capacity);
	ValArr_Init(valarr, valarr->vm);
}


void Value_Print(FILE* fout, Value_t val)
{
	switch (VALTYPE(val))
	{
	case VAL_BOOL:		fprintf(fout, AS_BOOL(val) ? "true" : "false"); break;
	case VAL_NIL:		fprintf(fout, "nil"); break;
	case VAL_NUMBER:	fprintf(fout, "%g", AS_NUMBER(val)); break;
	case VAL_OBJ:		Obj_Print(fout, val); break;
    default: fprintf(fout, "%d", VALTYPE(val));break;
	}
}


bool Value_Equal(Value_t a, Value_t b)
{
	if (a.type != b.type)
	{
		return false;
	}

	switch (a.type)
	{
	case VAL_BOOL:		return AS_BOOL(a) == AS_BOOL(b);
	case VAL_NIL:		return true;
	case VAL_NUMBER:	
		return (AS_NUMBER(a) - FLT_EPSILON <= AS_NUMBER(b))
			&& (AS_NUMBER(b) <= AS_NUMBER(a) + FLT_EPSILON);
	case VAL_OBJ:       return AS_OBJ(a) == AS_OBJ(b);
	default: CLOX_ASSERT(false && "Unhandled Value_Equal() case"); return false;
	}
}

