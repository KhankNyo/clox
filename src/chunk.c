
#include <stdlib.h>
#include "include/value.h"
#include "include/chunk.h"
#include "include/memory.h"




void Chunk_Init(Chunk_t* chunk, uint32_t line_start)
{
	chunk->code = NULL;
	chunk->size = 0;
	chunk->capacity = 0;

	LineInfo_Init(&chunk->line_info, line_start);
	ValArr_Init(&chunk->consts);
}


void Chunk_Write(Chunk_t* chunk, uint8_t byte, uint32_t line)
{
	static uint32_t prevline = 0;
	if (line != prevline)
	{
		prevline = line;
		LineInfo_Write(&chunk->line_info, chunk->size);
	}


	if (chunk->size + 1 > chunk->capacity)
	{
		const size_t oldcap = chunk->capacity;
		chunk->capacity = GROW_CAPACITY(chunk->capacity);
		chunk->code = GROW_ARRAY(uint8_t, chunk->code, oldcap, chunk->capacity);
	}

	chunk->code[chunk->size] = byte;
	chunk->size++;
}


size_t Chunk_AddConstant(Chunk_t* chunk, Value_t constant)
{
	ValArr_Write(&chunk->consts, constant);
	return chunk->consts.size - 1;
}



void Chunk_WriteConstant(Chunk_t* chunk, Value_t constant, uint32_t line)
{
	ValArr_Write(&chunk->consts, constant);
	const size_t addr = chunk->consts.size - 1;

	if (addr > UINT8_MAX)
	{
		Chunk_Write(chunk, OP_CONSTANT_LONG, line);
		Chunk_Write(chunk, addr >> 0, line);
		Chunk_Write(chunk, addr >> 8, line);
		Chunk_Write(chunk, addr >> 16, line);
	}
	else
	{
		Chunk_Write(chunk, OP_CONSTANT, line);
		Chunk_Write(chunk, addr, line);
	}
}




void Chunk_Free(Chunk_t* chunk)
{
	uint32_t line_start = chunk->line_info.start;
	FREE_ARRAY(uint8_t, chunk->code, chunk->capacity);
	LineInfo_Free(&chunk->line_info);
	ValArr_Free(&chunk->consts);

	Chunk_Init(chunk, line_start);
}

