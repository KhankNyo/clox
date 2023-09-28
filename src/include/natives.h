#ifndef _CLOX_NATIVES_H_
#define _CLOX_NATIVES_H_




#include "value.h"
#include "typedefs.h"

Value_t Native_Clock(VM_t* vm, int argc, Value_t* argv);
Value_t Native_ToStr(VM_t* vm, int argc, Value_t* argv);
Value_t Native_Array(VM_t* vm, int argc, Value_t* argv);


#endif /* _CLOX_NATIVES_H_ */


