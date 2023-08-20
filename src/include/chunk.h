#ifndef _CLOX_CHUNK_H_
#define _CLOX_CHUNK_H_


#include "line.h"
#include "common.h"
#include "value.h"

typedef enum Opc_t
{
	OP_CONSTANT,
	OP_RETURN,
} Opc_t;


typedef struct Chunk_t
{
	uint8_t* code;
	size_t size;
	size_t capacity;

	ValueArr_t consts;
	LineInfo line_info;
} Chunk_t;


/* set all members to 0 */
void Chunk_Init(Chunk_t* chunk);

/* writes an op byte to the chunk's code */
void Chunk_Write(Chunk_t* chunk, uint8_t byte, uint32_t line);


size_t Chunk_AddConstant(Chunk_t* chunk, Value_t constant);

/* free and set all members to 0 */
void Chunk_Free(Chunk_t* chunk);


#endif /* _CLOX_CHUNK_H_ */

