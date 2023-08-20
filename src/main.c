

#include <stdio.h>
#include "include/debug.h"
#include "include/chunk.h"


int main(void)
{
	Chunk_t chunk;
	Chunk_Init(&chunk);
	Chunk_Write(&chunk, OP_RETURN);

	Disasm_Chunk(stdout, &chunk, "test chunk");

	Chunk_Free(&chunk);
	return 0;
}

