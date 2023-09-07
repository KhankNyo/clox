

#include "include/common.h"
#include "include/memory.h"
#include "include/line.h"



void LineInfo_Init(LineInfo_t* li, Allocator_t* alloc)
{
    li->at = NULL;
	li->count = 0;
	li->capacity = 0;
    li->prevline = 0;
	li->alloc = alloc;
}
	

void LineInfo_Write(LineInfo_t* li, size_t addr, line_t line)
{
    if (line == li->prevline)
    {
        return;
    }
    else
    {
        li->prevline = line;
    }

	if (li->count + 1 > li->capacity)
	{
		const size_t oldcap = li->capacity;
		li->capacity = GROW_CAPACITY(li->capacity);
		li->at = GROW_ARRAY(li->alloc, LineAddr_t, 
			li->at, oldcap, li->capacity
		);
	}

	li->at[li->count] = (LineAddr_t){ 
        .addr = addr,
        .line = line,
    };
	li->count++;
}
	

line_t LineInfo_GetLine(const LineInfo_t li, size_t addr)
{
    if (li.count == 0)
        return 1;
    else if (addr == 0)
        return li.at[0].line;
    else if (li.at[li.count - 1].addr <= addr)
        return li.at[li.count - 1].line;

    line_t i = 0;
    while (i < li.count && li.at[i].addr < addr)
        i++;

    return li.at[i - 1].line;
}


void LineInfo_Free(LineInfo_t* li)
{
	FREE_ARRAY(li->alloc, LineAddr_t, li->at, li->capacity);
	LineInfo_Init(li, li->alloc);
}

