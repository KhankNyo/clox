#ifndef _CLOX_COMPILER_H_
#define _CLOX_COMPILER_H_



#include "common.h"
#include "object.h"
#include "typedefs.h"



ObjFunction_t* Compile(VMData_t* data, const char* src);

/* TODO: not use a static compiler? */
void Compiler_MarkObj(void);


#endif /* _CLOX_COMPILER_H_ */

