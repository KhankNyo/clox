#ifndef _CLOX_VALUE_H_
#define _CLOX_VALUE_H_


#include "common.h"
#include "typedefs.h"



typedef struct Obj_t Obj_t;
typedef struct ObjString_t ObjString_t;


typedef enum ValType_t
{
	VAL_NIL,
	VAL_BOOL,
	VAL_NUMBER,
	VAL_OBJ,
} ValType_t;


typedef struct Value_t
{
	ValType_t type;
	union {
		bool boolean;
		double number;
		Obj_t* obj;
	} as;
} Value_t;

typedef struct
{
    VM_t* vm;
	size_t size;
	size_t capacity;
	Value_t* vals;
} ValueArr_t;



#define BOOL_VAL(val)		((Value_t){.type = VAL_BOOL,	.as.boolean = val,})
#define NIL_VAL()			((Value_t){.type = VAL_NIL,		.as.number = 0,})
#define NUMBER_VAL(val)		((Value_t){.type = VAL_NUMBER,	.as.number = val,})
#define OBJ_VAL(val)		((Value_t){.type = VAL_OBJ,		.as.obj = (Obj_t*)(val),})


#define AS_BOOL(value)		((value).as.boolean)
#define AS_NUMBER(value)	((value).as.number)
#define AS_OBJ(value)		((value).as.obj)


#define VALTYPE(value)      ((value).type)
#define IS_BOOL(value)		(VALTYPE(value) == VAL_BOOL)
#define IS_NIL(value)		(VALTYPE(value) == VAL_NIL)
#define IS_NUMBER(value)	(VALTYPE(value) == VAL_NUMBER)
#define IS_OBJ(value)		(VALTYPE(value) == VAL_OBJ)


void ValArr_Init(ValueArr_t* valarr, VM_t* vm);
void ValArr_Write(ValueArr_t* valarr, Value_t val);
bool ValArr_Find(const ValueArr_t* valarr, Value_t val, size_t* index_out);
void ValArr_Free(ValueArr_t* valarr);

void Value_Print(FILE* fout, const Value_t val);
bool Value_Equal(const Value_t a, const Value_t b);


#endif /* _CLOX_VALUE_H_ */
