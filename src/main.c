

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <time.h>

#include "include/memory.h"
#include "include/debug.h"
#include "include/chunk.h"
#include "include/vm.h"


int main(void)
{

	Allocator_t alloc;
	Allocator_Init(&alloc, 5 * 1024);

	VM_t vm;
	VM_Init(&vm);

	
	Chunk_t chunk;
	Chunk_Init(&chunk, &alloc, 123);
	{
		Chunk_WriteConstant(&chunk, 3.4, 123);
		Chunk_Write(&chunk, OP_NEGATE, 123);
		Chunk_Write(&chunk, OP_RETURN, 123);

		Chunk_WriteConstant(&chunk, 1.2, 123);
		Chunk_WriteConstant(&chunk, 3.4, 123);
		Chunk_Write(&chunk, OP_ADD, 123);

		Chunk_WriteConstant(&chunk, 5.6, 123);
		Chunk_Write(&chunk, OP_DIVIDE, 123);

		Chunk_Write(&chunk, OP_NEGATE, 123);
		Chunk_Write(&chunk, OP_RETURN, 123);
	}

	for (int j = 0; j < 10; j++)
	{
		double start = clock();
		for (size_t i = 0; i < 100000000; i++)
			VM_Interpret(&vm, &chunk);
		double t = clock() - start;
		printf("Run #%d: %.03fms\n", j, t * 1000 / CLOCKS_PER_SEC);
	}

	Chunk_Free(&chunk);
	VM_Free(&vm);

	Allocator_KillEmAll(&alloc);
	return 0;
}

