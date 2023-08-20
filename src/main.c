

#include <stdio.h>
#include "include/debug.h"
#include "include/chunk.h"


int main(void)
{
	Chunk_t chunk;
	Chunk_Init(&chunk);
	{
		const size_t const_addr = Chunk_AddConstant(&chunk, 1.2);
		Chunk_Write(&chunk, OP_CONSTANT, 123);
		Chunk_Write(&chunk, const_addr, 123);
		Chunk_Write(&chunk, OP_RETURN, 123);
	}
	Disasm_Chunk(stdout, &chunk, "test chunk");
	Chunk_Free(&chunk);
	return 0;
}

