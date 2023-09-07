#ifndef _CLOX_LINE_H_
#define _CLOX_LINE_H_




#include "common.h"
#include "memory.h"

typedef uint32_t line_t;
typedef struct LineAddr_t LineAddr_t;
typedef struct LineInfo_t
{
	Allocator_t* alloc;
	LineAddr_t* at;	/* stores the offset of the isntruction that contains line changes */

	size_t count;
	uint32_t capacity;

    line_t prevline;
} LineInfo_t;



void LineInfo_Init(LineInfo_t* li, Allocator_t* alloc);

/*
 *  Notes which instruction_offset corresponding to which line
 */
void LineInfo_Write(LineInfo_t* li, size_t instruction_offset, line_t line);


/* 
*	\returns the line number of the given instruction offset (must be valid)
*/
line_t LineInfo_GetLine(const LineInfo_t li, size_t instruction_offset);

void LineInfo_Free(LineInfo_t* li);



struct LineAddr_t
{
    line_t line;
    size_t addr;
};


#endif /* _CLOX_LINE_H_ */
