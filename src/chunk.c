
#include <stdlib.h>
#include "include/value.h"
#include "include/chunk.h"
#include "include/memory.h"
#include "include/vm.h"




void Chunk_Init(Chunk_t* chunk, VM_t* vm)
{
	chunk->code = NULL;
	chunk->size = 0;
	chunk->capacity = 0;
    chunk->vm = vm;

	LineInfo_Init(&chunk->line_info, vm->alloc);
	ValArr_Init(&chunk->consts, vm);
}


void Chunk_Write(Chunk_t* chunk, uint8_t byte, line_t line)
{
    LineInfo_Write(&chunk->line_info, chunk->size, line);
	if (chunk->size + 1 > chunk->capacity)
	{
		const size_t oldcap = chunk->capacity;
		chunk->capacity = GROW_CAPACITY(chunk->capacity);
		chunk->code = GROW_ARRAY(chunk->vm, uint8_t, 
			chunk->code, oldcap, chunk->capacity
        );
	}

	chunk->code[chunk->size] = byte;
	chunk->size++;
}


size_t Chunk_AddConstant(Chunk_t* chunk, Value_t constant)
{
	ValArr_Write(&chunk->consts, constant);
	return chunk->consts.size - 1;
}


size_t Chunk_AddUniqueConstant(Chunk_t* chunk, Value_t constant)
{
    size_t index = 0;
    if (ValArr_Find(&chunk->consts, constant, &index))
    {
        return index;
    }
    return Chunk_AddConstant(chunk, constant);
}



size_t Chunk_WriteConstant(Chunk_t* chunk, Value_t constant, line_t line)
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
	return addr;
}




void Chunk_Free(Chunk_t* chunk)
{
	FREE_ARRAY(chunk->vm, uint8_t, chunk->code, chunk->capacity);
	LineInfo_Free(&chunk->line_info);
	ValArr_Free(&chunk->consts);

	Chunk_Init(chunk, chunk->vm);
}

