

#include <stdio.h>
#include <string.h>
#include <assert.h>

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
		const size_t const_addr = Chunk_AddConstant(&chunk, 1.2);
		Chunk_Write(&chunk, OP_CONSTANT, 123);
		Chunk_Write(&chunk, const_addr, 123);
		Chunk_Write(&chunk, OP_RETURN, 123);
	}
	Disasm_Chunk(stdout, &chunk, "test chunk");
	Chunk_Free(&chunk);
	VM_Free(&vm);

	Allocator_KillEmAll(&alloc);
	return 0;
}

