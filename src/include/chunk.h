#ifndef _CLOX_CHUNK_H_
#define _CLOX_CHUNK_H_

#include "common.h"

typedef enum Opc_t
{
	OP_RETURN,
} Opc_t;


typedef struct Chunk_t
{
	uint8_t* chunk;
	size_t size;
	size_t capacity;
} Chunk_t;



void Chunk_Init(Chunk_t* cnk);
void Chunk_Write(Chunk_t* cnk, uint8_t byte);
void Chunk_Free(Chunk_t* cnk);


#endif /* _CLOX_CHUNK_H_ */

