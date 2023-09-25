

#include <stdio.h>

#include "include/line.h"
#include "include/chunk.h"
#include "include/object.h"
#include "include/debug.h"



/* \returns next offset */
static size_t single_byte(FILE* fout, const char* menmonic, size_t offset);
/* \returns next offset */
static size_t const_instruction(FILE* fout, 
	const char* mnemonic, const Chunk_t* chunk, size_t offset, unsigned addr_size
);

static size_t bytes_instruction(FILE* fout, 
    const char* mnemonic, const Chunk_t* chunk, size_t offset, unsigned argsize
);
static size_t jump_instruction(FILE* fout, 
    const char* mnemonic, int sign, const Chunk_t* chunk, size_t offset
);
#define byte_instruction(f, mne, cnk, off)\
    bytes_instruction(f, mne, cnk, off, 1)
#define INS_FMTSTR "%-24s "


static size_t invoke_instruction(FILE* fout,
    const char *mnemonic, const Chunk_t *chunk, size_t offset, size_t name_size
);





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


    case OP_JUMP:
        offset = jump_instruction(fout, "OP_JUMP", 1, chunk, offset);
        break;
    case OP_JUMP_IF_FALSE:
        offset = jump_instruction(fout, "OP_JUMP_IF_FALSE", 1, chunk, offset);
        break;

    case OP_LOOP:
        offset = jump_instruction(fout, "OP_LOOP", -1, chunk, offset);
        break;

    case OP_CALL:
        offset = byte_instruction(fout, "OP_CALL", chunk, offset); 
        break;

    case OP_INVOKE:
        offset = invoke_instruction(fout, "OP_INVOKE", chunk, offset, 1);
        break;

    case OP_SUPER_INVOKE:
        offset = invoke_instruction(fout, "OP_SUPER_INVOKE", chunk, offset, 1);
        break;

    case OP_SET_UPVALUE:
        offset = byte_instruction(fout, "OP_SET_UPVALUE", chunk, offset);
        break;
    case OP_GET_UPVALUE:
        offset = byte_instruction(fout, "OP_GET_UPVALUE", chunk, offset);
        break;

    case OP_SET_PROPERTY:
        offset = const_instruction(fout, "OP_SET_PROPERTY", chunk, offset, 1);
        break;
    case OP_GET_PROPERTY:
        offset = const_instruction(fout, "OP_GET_PROPERTY", chunk, offset, 1);
        break;
    case OP_SET_PROPERTY_LONG:
        offset = const_instruction(fout, "OP_SET_PROPERTY_LONG", chunk, offset, 3);
        break;
    case OP_GET_PROPERTY_LONG:
        offset = const_instruction(fout, "OP_GET_PROPERTY_LONG", chunk, offset, 3);
        break;

    case OP_GET_SUPER:
        offset = const_instruction(fout, "OP_GET_SUPER", chunk, offset, 1);
        break;
        
    case OP_CLOSURE:
    {
        offset++; /* skip instruction */
        uint8_t constant = chunk->code[offset++];

        fprintf(fout, INS_FMTSTR"%4d ", "OP_CLOSURE", constant);
        Value_Print(fout, chunk->consts.vals[constant]);
        fputc('\n', fout);

        const ObjFunction_t* fun = AS_FUNCTION(chunk->consts.vals[constant]);
        for (int j = 0; j < fun->upval_count; j++)
        {
            uint8_t is_local = chunk->code[offset++];
            uint8_t index = chunk->code[offset++];
            printf("%04d    |                       %s %d\n",
                (int)offset - 2, is_local ? "local" : "upvalue", index
            );
        }
    }
    break;
    case OP_CLOSE_UPVALUE:
        offset = single_byte(fout, "OP_CLOSE_UPVALUE", offset);
        break;

    case OP_CLASS:
        offset = const_instruction(fout, "OP_CLASS", chunk, offset, 1);
        break;

    case OP_INHERIT:
        offset = single_byte(fout, "OP_INHERIT", offset);
        break;

    case OP_DUP:
        offset = single_byte(fout, "OP_DUP", offset);
        break;

    case OP_METHOD:
        offset = const_instruction(fout, "OP_METHOD", chunk, offset, 1);
        break;






	default:
		fprintf(fout, "Unknown opcode %d", ins);
		offset += 1;
		break;

	}
	putc('\n', fout);
	return offset;
}








static uint64_t read_arg(const Chunk_t* chunk, size_t offset, unsigned argsize)
{
    uint64_t arg = 0;
    for (unsigned i = 0; i < argsize; i++)
    {
        arg |= (uint64_t)chunk->code[offset + i + 1] << (argsize - i - 1)*8;
    }
    return arg;
}



static size_t single_byte(FILE* fout, const char* mnemonic, size_t offset)
{
	fprintf(fout, INS_FMTSTR, mnemonic);
	return offset + 1;
}


static size_t const_instruction(FILE* fout, 
	const char* mnemonic, const Chunk_t* chunk, 
	size_t offset, unsigned addr_size)
{
    CLOX_ASSERT(chunk->consts.vals != NULL);

	uint64_t const_addr = read_arg(chunk, offset + 1, addr_size);
	fprintf(fout, INS_FMTSTR"%4zu ", mnemonic, const_addr);
	Value_Print(fout, chunk->consts.vals[const_addr]);
	return offset + 1 + addr_size;
}



static size_t bytes_instruction(FILE* fout, 
    const char* mnemonic, const Chunk_t* chunk, size_t offset, unsigned argsize
)
{
    CLOX_ASSERT(argsize < sizeof(uint64_t) && "Unsupported arg size");

    uint64_t operand = read_arg(chunk, offset + 1, argsize);
    fprintf(fout, INS_FMTSTR"%4llu", mnemonic, operand);
    return offset + 1 + argsize;
}


static size_t jump_instruction(FILE* fout, 
    const char* mnemonic, int sign, const Chunk_t* chunk, size_t offset
)
{
    uint16_t jump_offset = (uint16_t)read_arg(chunk, offset + 1, 2);

    fprintf(fout, INS_FMTSTR"%4d -> %zu", mnemonic, jump_offset,
        offset + 3 + sign * jump_offset
    );
    return offset + 3;
}



static size_t invoke_instruction(FILE* fout,
    const char *mnemonic, const Chunk_t *chunk, size_t offset, size_t name_size
)
{
    unsigned name = read_arg(chunk, offset + 1, name_size);
    uint8_t argc = read_arg(chunk, offset + name_size + 1, 1);

    fprintf(fout, "%-16s (%d args) %4d", mnemonic, argc, name);
    Value_Print(fout, chunk->consts.vals[name]);
    fputc('\n', fout);
    return offset + 1 + name_size + 1;
}
