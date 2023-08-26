

#include <float.h>
#include "include/common.h"
#include "include/memory.h"
#include "include/value.h"




void ValArr_Init(ValueArr_t* valarr, Allocator_t* alloc)
{
	valarr->size = 0;
	valarr->capacity = 0;
	valarr->vals = NULL;
	valarr->alloc = alloc;
}


void ValArr_Write(ValueArr_t* valarr, Value_t val)
{
	if (valarr->size + 1 > valarr->capacity)
	{
		const size_t oldcap = valarr->capacity;
		valarr->capacity = GROW_CAPACITY(valarr->capacity);
		valarr->vals = GROW_ARRAY(valarr->alloc, Value_t, 
			valarr->vals, oldcap, valarr->capacity
		);
	}

	valarr->vals[valarr->size] = val;
	valarr->size++;
}


void ValArr_Free(ValueArr_t* valarr)
{
	FREE_ARRAY(valarr->alloc, Value_t, valarr->vals, valarr->capacity);
	ValArr_Init(valarr, valarr->alloc);
}


void Value_Print(FILE* fout, Value_t val)
{
	switch (val.type)
	{
	case VAL_BOOL:		fprintf(fout, AS_BOOL(val) ? "true" : "false"); break;
	case VAL_NIL:		fprintf(fout, "nil"); break;
	case VAL_NUMBER:	fprintf(fout, "'%g'", AS_NUMBER(val)); break;
	default: CLOX_ASSERT(false && "Unhandled Value_Print() case."); return;
	}
}


bool Value_Equal(const Value_t a, const Value_t b)
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
	default: CLOX_ASSERT(false && "Unhandled Value_Equal() case"); return false;
	}
}

