#ifndef _CLOX_VALUE_H_
#define _CLOX_VALUE_H_


#include "common.h"

typedef double Value_t;
typedef struct
{
	size_t size;
	size_t capacity;
	Value_t* vals;
} ValueArr_t;


void ValArr_Init(ValueArr_t* valarr);
void ValArr_Write(ValueArr_t* valarr, Value_t val);
void ValArr_Free(ValueArr_t* valarr);
void printVal(FILE* fout, Value_t val);



#endif /* _CLOX_VALUE_H_ */