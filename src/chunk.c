
#include <stdlib.h>
#include "include/chunk.h"
#include "include/memory.h"




void Chunk_Init(Chunk_t* cnk)
{
	cnk->chunk = NULL;
	cnk->size = 0;
	cnk->capacity = 0;
}


void Chunk_Write(Chunk_t* cnk, uint8_t byte)
{
	if (cnk->size + 1 < cnk->capacity)
	{
		size_t oldcap = cnk->capacity;
		cnk->capacity = GROW_CAPACITY(cnk->capacity);
		cnk->chunk = GROW_ARRAY(uint8_t, cnk->chunk, oldcap, cnk->capacity);
	}
}


void Chunk_Free(Chunk_t* cnk)
{
	FREE_ARRAY(uint8_t, cnk->chunk, cnk->capacity);
	Chunk_Init(cnk);
}

