#ifndef _CLOX_VALUE_H_
#define _CLOX_VALUE_H_


#include "common.h"
#include "memory.h"

typedef enum ValType_t
{
	VAL_NIL,
	VAL_BOOL,
	VAL_NUMBER,
} ValType_t;

typedef struct Value_t
{
	ValType_t type;
	union {
		bool boolean;
		double number;
	} as;
} Value_t;

typedef struct
{
	Allocator_t* alloc;
	size_t size;
	size_t capacity;
	Value_t* vals;
} ValueArr_t;



#define BOOL_VAL(val)	((Value_t){.type = VAL_BOOL,	.as.boolean = val,})
#define NIL_VAL()		((Value_t){.type = VAL_NIL,		.as.number = 0,})
#define NUMBER_VAL(val) ((Value_t){.type = VAL_NUMBER,	.as.number = val,})

#define AS_BOOL(value)		((value).as.boolean)
#define AS_NUMBER(value)	((value).as.number)

#define IS_BOOL(value)		((value).type == VAL_BOOL)
#define IS_NIL(value)		((value).type == VAL_NIL)
#define IS_NUMBER(value)	((value).type == VAL_NUMBER)


void ValArr_Init(ValueArr_t* valarr, Allocator_t* alloc);
void ValArr_Write(ValueArr_t* valarr, Value_t val);
void ValArr_Free(ValueArr_t* valarr);

void Value_Print(FILE* fout, const Value_t val);
bool Value_Equal(const Value_t a, const Value_t b);


#endif /* _CLOX_VALUE_H_ */
