

#include <stdio.h>

#include "include/line.h"
#include "include/chunk.h"
#include "include/debug.h"



/* \returns next offset */
static size_t single_byte(FILE* fout, const char* menmonic, size_t offset);
/* \returns next offset */
static size_t const_instruction(FILE* fout, 
	const char* mnemonic, const Chunk_t* chunk, size_t offset, unsigned addr_size
);

static size_t byte_instruction(FILE* fout, const char* mnemonic, const Chunk_t* chunk, size_t offset);


void Disasm_Chunk(FILE* fout, const Chunk_t* chunk, const char* name)
{
	fprintf(fout, "== %s ==\n", name);

	size_t offset = 0;
	while (offset < chunk->size)
	{
		offset = Disasm_Instruction(fout, chunk, offset);
	}
}




size_t Disasm_Instruction(FILE* fout, const Chunk_t* chunk, size_t offset)
{
	/* ins offset num */
	fprintf(fout, "%04zu ", offset);

	
	/* fancy line formatting */
	static line_t last_line = 0;
	const line_t curr_line = LineInfo_GetLine(chunk->line_info, offset);
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
		offset = single_byte(fout, "OP_RETURN", offset);
		break;

	case OP_CONSTANT:
		offset = const_instruction(fout, "OP_CONSTANT", chunk, offset, 1);
		break;
	case OP_CONSTANT_LONG:
		offset = const_instruction(fout, "OP_CNST_LONG", chunk, offset, 3);
		break;

	case OP_NEGATE:
		offset = single_byte(fout, "OP_NEGATE", offset);
		break;
	case OP_NOT:
		offset = single_byte(fout, "OP_NOT", offset);
		break;


	case OP_ADD:
		offset = single_byte(fout, "OP_ADD", offset);
		break;
	case OP_SUBTRACT:
		offset = single_byte(fout, "OP_SUBTRACT", offset);
		break;
	case OP_MULTIPLY:
		offset = single_byte(fout, "OP_MULTIPLY", offset);
		break;
	case OP_DIVIDE:
		offset = single_byte(fout, "OP_DIVIDE", offset);
		break;


	case OP_EQUAL:
		offset = single_byte(fout, "OP_EQUAL", offset);
		break;
	case OP_LESS:
		offset = single_byte(fout, "OP_LESS", offset);
		break;
	case OP_GREATER:
		offset = single_byte(fout, "OP_GREATER", offset);
		break;


	case OP_TRUE:
		offset = single_byte(fout, "OP_TRUE", offset);
		break;
	case OP_FALSE:
		offset = single_byte(fout, "OP_FALSE", offset);
		break;
	case OP_NIL:
		offset = single_byte(fout, "OP_NIL", offset);
		break;


    case OP_POP:
        offset = single_byte(fout, "OP_POP", offset);
        break;
    case OP_POPN:
        offset = byte_instruction(fout, "OP_POPN", chunk, offset);
        break;


    case OP_PRINT:
        offset = single_byte(fout, "OP_PRINT", offset);
        break;

    case OP_DEFINE_GLOBAL:
        offset = const_instruction(fout, "OP_DEFINE_GLOBAL", chunk, offset, 1);
        break;
    case OP_DEFINE_GLOBAL_LONG:
        offset = const_instruction(fout, "OP_DEFINE_GLOBAL_LONG", chunk, offset, 3);
        break;


    case OP_GET_LOCAL:
        offset = byte_instruction(fout, "OP_GET_LOCAL", chunk, offset);
        break;
    case OP_SET_LOCAL:
        offset = byte_instruction(fout, "OP_SET_LOCAL", chunk, offset);
        break;

    case OP_GET_GLOBAL:
        offset = const_instruction(fout, "OP_GET_GLOBAL", chunk, offset, 1);
        break;

    case OP_GET_GLOBAL_LONG:
        offset = const_instruction(fout, "OP_GET_GLOBAL", chunk, offset, 3);
        break;

    case OP_SET_GLOBAL:
        offset = const_instruction(fout, "OP_SET_GLOBAL", chunk, offset, 1);
        break;

    case OP_SET_GLOBAL_LONG:
        offset = const_instruction(fout, "OP_SET_GLOBAL_LONG", chunk, offset, 3);
        break;



	default:
		fprintf(fout, "Unknown opcode %d", ins);
		offset += 1;
		break;

	}
	putc('\n', fout);
	return offset;
}






static size_t single_byte(FILE* fout, const char* mnemonic, size_t offset)
{
	fprintf(fout, "%s", mnemonic);
	return offset + 1;
}


static size_t const_instruction(FILE* fout, 
	const char* mnemonic, const Chunk_t* chunk, 
	size_t offset, unsigned addr_size)
{
	size_t const_addr = 0;
	for (unsigned i = 0; i < addr_size; i++)
	{
		const_addr |= (size_t)chunk->code[offset + 1 + i] << 8*i;
	}

	fprintf(fout, "%-16s %4zu ", mnemonic, const_addr);
	Value_Print(fout, chunk->consts.vals[const_addr]);
	return offset + 1 + addr_size;
}



static size_t byte_instruction(FILE* fout, const char* mnemonic, const Chunk_t* chunk, size_t offset)
{
    uint8_t operand = chunk->code[offset + 1];
    fprintf(fout, "%-16s %4d", mnemonic, operand);
    return offset + 2;
}
