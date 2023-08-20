

#include <stdio.h>

#include "include/line.h"
#include "include/chunk.h"
#include "include/debug.h"



/* all returns next offset */

static size_t singleByte(FILE* fout, const char* menmonic, size_t offset);
static size_t constInstruction(FILE* fout, const char* mnemonic, Chunk_t* chunk, size_t offset);


void Disasm_Chunk(FILE* fout, Chunk_t* chunk, const char* name)
{
	fprintf(fout, "== %s ==\n", name);

	size_t offset = 0;
	while (offset < chunk->size)
	{
		offset = Disasm_Instruction(fout, chunk, offset);
	}
}




size_t Disasm_Instruction(FILE* fout, Chunk_t* chunk, size_t offset)
{
	/* ins offset num */
	fprintf(fout, "%04zu ", offset);

	
	/* fancy line formatting */
	static uint32_t last_line = 0;
	const uint32_t curr_line = LineInfo_GetLine(chunk->line_info, offset);
	if (last_line != curr_line)
	{
		fprintf(fout, "%4d ", curr_line);
		last_line = curr_line;
	}
	else
	{
		fprintf(fout, "   | ");
	}



	/* mnemonic and operand(s) */
	const uint8_t ins = chunk->code[offset];
	switch (ins)
	{
	case OP_RETURN:
		offset = singleByte(fout, "OP_RETURN", offset);
		break;

	case OP_CONSTANT:
		offset = constInstruction(fout, "OP_CONSTANT", chunk, offset);
		break;

	default:
		fprintf(fout, "Unknown opcode %d", ins);
		offset += 1;
		break;
	}
	putc('\n', fout);
	return offset;
}






static size_t singleByte(FILE* fout, const char* mnemonic, size_t offset)
{
	fprintf(fout, "%s", mnemonic);
	return offset + 1;
}


static size_t constInstruction(FILE* fout, const char* mnemonic, Chunk_t* chunk, size_t offset)
{
	const uint8_t const_addr = chunk->code[offset + 1];
	fprintf(fout, "%-16s %4d ", mnemonic, const_addr);
	printVal(fout, chunk->consts.vals[const_addr]);
	return offset + 2;
}



