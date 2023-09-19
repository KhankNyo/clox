#ifndef _CLOX_COMPILER_H_
#define _CLOX_COMPILER_H_



#include "common.h"
#include "object.h"
#include "typedefs.h"


typedef struct Compiler_t Compiler_t;

ObjFunction_t* Compile(VM_t* vmdata, const char* src);
void Compiler_MarkObj(Compiler_t* compiler);


#endif /* _CLOX_COMPILER_H_ */

