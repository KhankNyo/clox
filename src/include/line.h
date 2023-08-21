#ifndef _CLOX_LINE_H_
#define _CLOX_LINE_H_




#include "common.h"
#include "memory.h"


typedef struct LineInfo_t
{
	Allocator_t* alloc;
	size_t* changes;	/* stores the offset of the isntruction that contains line changes */
	uint32_t count;
	uint32_t capacity;
	uint32_t start;

} LineInfo_t;



void LineInfo_Init(LineInfo_t* li, Allocator_t* alloc, uint32_t line_start);

/* writes the instruction offset where the line changes */
/* 
	NOTE: the instruction offsets must be written sequentially in ascending order 
*/
void LineInfo_Write(LineInfo_t* li, size_t instruction_offset);


/* 
*	\returns the line number of the given instruction offset (must be valid)
*/
uint32_t LineInfo_GetLine(const LineInfo_t li, size_t instruction_offset);

void LineInfo_Free(LineInfo_t* li);


#endif /* _CLOX_LINE_H_ */