
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


void printVal(FILE* fout, Value_t val)
{
	fprintf(fout, "'%g'", val);
}

