#ifndef _CLOX_VALUE_H_
#define _CLOX_VALUE_H_


#include "common.h"
#include "typedefs.h"
#include <string.h>




typedef enum ValType_t
{
	VAL_NIL,
	VAL_BOOL,
	VAL_NUMBER,
	VAL_OBJ,
} ValType_t;




#ifdef NAN_BOXING

typedef uint64_t Value_t;

#define CLOX_QNAN ((uint64_t)(0x7ffc) << 8*6)
#define SIGNBIT(type) ((uint64_t)1 << (8*sizeof(type) - 1))

#define TAG_LOCATION 0
/* enums do not work with 64 bit values */
#define TAG_NIL ((uint64_t)1 << TAG_LOCATION)   /* 0b01 */
#define TAG_FALSE ((uint64_t)2 << TAG_LOCATION) /* 0b10 */
#define TAG_TRUE ((uint64_t)3 << TAG_LOCATION)  /* 0b11 */
#define TAG_PTR (SIGNBIT(double) | CLOX_QNAN)

#define PTR_BITS (((uint64_t)1 << 48) - 1)




#define NUMBER_VAL(number)  Value_FromNumber(number)
static inline Value_t Value_FromNumber(double number)
{
    Value_t val;
    memcpy(&val, &number, sizeof number);
    return val;
}
#define NIL_VAL()           ((Value_t)(CLOX_QNAN | TAG_NIL))
    #define FALSE_VAL       ((Value_t)(CLOX_QNAN | TAG_FALSE))
    #define TRUE_VAL        ((Value_t)(CLOX_QNAN | TAG_TRUE))
#define BOOL_VAL(boolean)   ((Value_t)((boolean) ? TRUE_VAL : FALSE_VAL))
#define OBJ_VAL(obj)        ((Value_t)(SIGNBIT(double) | CLOX_QNAN | (uint64_t)(uintptr_t)(obj)))


#define AS_NUMBER(value)    Value_ToNumber(value)
static inline double Value_ToNumber(Value_t val)
{
    double number;
    memcpy(&number, &val, sizeof number);
    return number;
}
#define AS_BOOL(value)      ((value) == TRUE_VAL)
#define AS_OBJ(value)       ((Obj_t*)(uintptr_t)((value) & PTR_BITS))


#define IS_NUMBER(value)    (((value) & CLOX_QNAN) != CLOX_QNAN)
#define IS_NIL(value)       (NIL_VAL() == (value))
#define IS_BOOL(value)      (TRUE_VAL == ((value) | TAG_TRUE))
#define IS_OBJ(value)       (TAG_PTR == ((value) & TAG_PTR))

#define VALTYPE(value)      Value_TypeOf(value)
static inline ValType_t Value_TypeOf(Value_t value)
{
    if (IS_NUMBER(value))
        return VAL_NUMBER;
    if (IS_NIL(value))
        return VAL_NIL;
    if (IS_BOOL(value))
        return VAL_BOOL;
    return VAL_OBJ;
}


#else /* !NAN_BOXING */


typedef struct Value_t
{
	ValType_t type;
	union {
		bool boolean;
		double number;
		Obj_t* obj;
	} as;
} Value_t;

#define BOOL_VAL(b)         ((Value_t){.type = VAL_BOOL,	.as.boolean = b,})
#define NIL_VAL()			((Value_t){.type = VAL_NIL,		.as.number = 0,})
#define NUMBER_VAL(num)		((Value_t){.type = VAL_NUMBER,	.as.number = num,})
#define OBJ_VAL(object)		((Value_t){.type = VAL_OBJ,		.as.obj = (Obj_t*)(object),})

#define AS_BOOL(value)		((value).as.boolean)
#define AS_NUMBER(value)	((value).as.number)
#define AS_OBJ(value)		((value).as.obj)

#define VALTYPE(value)      ((value).type)
#define IS_BOOL(value)		(VALTYPE(value) == VAL_BOOL)
#define IS_NIL(value)		(VALTYPE(value) == VAL_NIL)
#define IS_NUMBER(value)	(VALTYPE(value) == VAL_NUMBER)
#define IS_OBJ(value)		(VALTYPE(value) == VAL_OBJ)

#endif /* NAN_BOXING */



typedef struct
{
    VM_t* vm;
	size_t size;
	size_t capacity;
	Value_t* vals;
} ValueArr_t;



void ValArr_Init(ValueArr_t* valarr, VM_t* vm);
void ValArr_Reserve(ValueArr_t* valarr, size_t extra);
void ValArr_Write(ValueArr_t* valarr, Value_t val);
bool ValArr_Find(const ValueArr_t* valarr, Value_t val, size_t* index_out);
void ValArr_Free(ValueArr_t* valarr);

void Value_Print(FILE* fout, const Value_t val);
bool Value_Equal(const Value_t a, const Value_t b);


#endif /* _CLOX_VALUE_H_ */
