#ifndef _CLOX_COMPILER_H_
#define _CLOX_COMPILER_H_



#include "common.h"
#include "chunk.h"
#include "typedefs.h"


bool Compile(VMData_t* data, const char* src, Chunk_t* chunk);



#endif /* _CLOX_COMPILER_H_ */

