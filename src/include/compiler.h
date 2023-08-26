#ifndef _CLOX_COMPILER_H_
#define _CLOX_COMPILER_H_



#include "common.h"
#include "chunk.h"

bool Compile(const char* src, Chunk_t* chunk);



#endif /* _CLOX_COMPILER_H_ */
