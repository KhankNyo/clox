

#include "include/common.h"
#include "include/memory.h"
#include "include/line.h"



void LineInfo_Init(LineInfo_t* li, Allocator_t* alloc, line_t start)
{
	li->changes = NULL;
	li->count = 0;
	li->capacity = 0;
	li->start = start;
	li->alloc = alloc;
}
	

void LineInfo_Write(LineInfo_t* li, size_t ins_offset)
{
	if (li->count + 1 > li->capacity)
	{
		const size_t oldcap = li->capacity;
		li->capacity = GROW_CAPACITY(li->capacity);
		li->changes = GROW_ARRAY(li->alloc, size_t, 
			li->changes, oldcap, li->capacity
		);
	}

	li->changes[li->count] = ins_offset;
	li->count++;
}
	

uint32_t LineInfo_GetLine(const LineInfo_t li, size_t offset)
{
	
	if ((li.count != 0) && (offset > li.changes[li.count - 1]))
		return li.start + li.count - 1;


	line_t start = 0;
	line_t end = li.count;
	line_t midpoint = 0;

	/* binary search, instruction offsets are guaranteed to be pushed in ascending order */
	while (start < end)
	{
		midpoint = start + (end - start) / 2;
		if (li.changes[midpoint] < offset)
		{
			start = midpoint;
		}
		else if (li.changes[midpoint] > offset)
		{
			end = midpoint;
		}
		else break;
	}
	return li.start + midpoint;
}


void LineInfo_Free(LineInfo_t* li)
{
	const line_t start = li->start;
	FREE_ARRAY(li->alloc, size_t, li->changes, li->capacity);
	LineInfo_Init(li, li->alloc, start);
}

