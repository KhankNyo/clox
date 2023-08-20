

#include <stdio.h>
#include "include/chunk.h"
#include "include/debug.h"




static size_t simpleton(FILE* fout, const char* menmonic, size_t offset);



void Disasm_Chunk(FILE* fout, Chunk_t* cnk, const char* name)
{
	fprintf(fout, "== %s ==\n", name);

	size_t offset = 0;
	while (offset < cnk->size)
	{
		offset = Disasm_Instruction(fout, cnk, offset);
	}
}




size_t Disasm_Instruction(FILE* fout, Chunk_t* cnk, size_t offset)
{
	fprintf(fout, "%04zu ", offset);



	const uint8_t ins = cnk->chunk[offset];
	switch (ins)
	{
	case OP_RETURN:
		return simpleton(fout, "OP_RETURN", offset);

	default:
		fprintf(fout, "Unknown opcode %d", ins);
		return offset + 1;
	}
}






static size_t simpleton(FILE* fout, const char* mnemonic, size_t offset)
{
	fprintf(fout, "%s\n", mnemonic);
	return offset + 1;
}



