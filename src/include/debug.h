#ifndef _CLOX_DEBUG_H_
#define _CLOX_DEBUG_H_


#include "common.h"
#include "chunk.h"







/* disassembles a chunk of instructions */
void Disasm_Chunk(FILE* fout, Chunk_t* cnk, const char* name);


/* disassembles a single instruction */
size_t Disasm_Instruction(FILE* fout, Chunk_t* cnk, size_t offset);


#endif /* _CLOX_DEBUG_H_ */

