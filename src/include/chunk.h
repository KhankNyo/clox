#ifndef _CLOX_CHUNK_H_
#define _CLOX_CHUNK_H_


#include "line.h"
#include "common.h"
#include "value.h"
#include "memory.h"

typedef enum Opc_t
{
	/* standard */
	OP_CONSTANT,
	OP_ADD,
	OP_SUBTRACT,
	OP_MULTIPLY,
	OP_DIVIDE,
	OP_NEGATE,
	OP_RETURN,


	/* challenges */
	OP_CONSTANT_LONG,
} Opc_t;


typedef struct Chunk_t
{
	Allocator_t* alloc;
	uint8_t* code;
	size_t size;
	size_t capacity;

	ValueArr_t consts;
	LineInfo_t line_info;
	line_t prevline;
} Chunk_t;


/* set all members to 0 */
void Chunk_Init(Chunk_t* chunk, Allocator_t* alloc, line_t line_start);

/* writes an op byte to the chunk's code */
void Chunk_Write(Chunk_t* chunk, uint8_t byte, line_t line);

/* adds a constant to the consts array */
size_t Chunk_AddConstant(Chunk_t* chunk, Value_t constant);


/* adds a constant to the consts array and add an instruction loading that constant */
void Chunk_WriteConstant(Chunk_t* chunk, Value_t constant, line_t line);


/* free and set all members to 0 */
void Chunk_Free(Chunk_t* chunk);


#endif /* _CLOX_CHUNK_H_ */

