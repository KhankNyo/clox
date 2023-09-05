#ifndef _CLOX_COMPILER_H_
#define _CLOX_COMPILER_H_



#include "common.h"
#include "object.h"
#include "typedefs.h"


ObjFunction_t* Compile(VMData_t* data, const char* src);



#endif /* _CLOX_COMPILER_H_ */

