#ifndef _CLOX_CHUNK_H_
#define _CLOX_CHUNK_H_


#include "line.h"
#include "common.h"
#include "value.h"
#include "memory.h"
#include "typedefs.h"


/* maximum number of constant a chunk can have */
#define MAX_CONST_IN_CHUNK 0xffffffu

typedef enum Opc_t
{
	/* standard */
	OP_CONSTANT,
	OP_NIL,
	OP_TRUE,
	OP_FALSE,
    OP_POP,
    OP_GET_LOCAL,
    OP_SET_LOCAL,
    OP_GET_GLOBAL,
    OP_DEFINE_GLOBAL,
    OP_SET_GLOBAL,
    OP_GET_UPVALUE,
    OP_SET_UPVALUE,
    OP_GET_PROPERTY,
    OP_SET_PROPERTY,
	OP_EQUAL,
	OP_GREATER,
	OP_LESS,
	OP_ADD,
	OP_SUBTRACT,
	OP_MULTIPLY,
	OP_DIVIDE,
	OP_NOT,
	OP_NEGATE,
    OP_PRINT,
    OP_JUMP,
    OP_JUMP_IF_FALSE,
    OP_LOOP,
    OP_CALL,
    OP_CLOSURE,
    OP_CLOSE_UPVALUE,
	OP_RETURN,
    OP_CLASS,
    OP_METHOD,


	/* challenges/extensions */
    OP_DUP,
    OP_POPN                 = OP_POP | 0x80,
	OP_CONSTANT_LONG        = OP_CONSTANT | 0x80,
    OP_DEFINE_GLOBAL_LONG   = OP_DEFINE_GLOBAL | 0x80,
    OP_GET_GLOBAL_LONG      = OP_GET_GLOBAL | 0x80,
    OP_SET_GLOBAL_LONG      = OP_SET_GLOBAL | 0x80,
    OP_SET_PROPERTY_LONG    = OP_SET_PROPERTY | 0x80,
    OP_GET_PROPERTY_LONG    = OP_GET_PROPERTY | 0x80,
} Opc_t;


typedef struct Chunk_t
{
    VM_t* vm;
	uint8_t* code;
	size_t size;
	size_t capacity;

	ValueArr_t consts;
	LineInfo_t line_info;
} Chunk_t;


/* set all members to 0 */
void Chunk_Init(Chunk_t* chunk, VM_t* vm);

/* writes an op byte to the chunk's code */
void Chunk_Write(Chunk_t* chunk, uint8_t byte, line_t line);

/* adds a constant to the consts array,
*	\returns the offset of the constant in the value array
*/
size_t Chunk_AddConstant(Chunk_t* chunk, Value_t constant);

/*
 * adds a constant if it's not already in the table, or 
 * \return the index of the constant that's already in the table 
 */
size_t Chunk_AddUniqueConstant(Chunk_t* chunk, Value_t constant);



/* adds a constant to the consts array and add an appropriate instruction for loading that constant,
*	\returns the offset of the constant in the value array
*/
size_t Chunk_WriteConstant(Chunk_t* chunk, Value_t constant, line_t line);


/* free and set all members to 0 */
void Chunk_Free(Chunk_t* chunk);


#endif /* _CLOX_CHUNK_H_ */

