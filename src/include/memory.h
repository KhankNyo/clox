#ifndef _CLOX_MEMORY_H_
#define _CLOX_MEMORY_H_




#include "common.h"

#define GROW_CAPACITY(cap)\
	((cap) < 8 ? 8 : (cap) * 2)

#define GROW_ARRAY(type, ptr, oldsize, newsize)\
	reallocate(ptr, (oldsize) * (sizeof(type)), newsize * sizeof(type))

#define FREE_ARRAY(type, ptr, oldsize)\
	reallocate(ptr, sizeof(type) * (oldsize), 0)

/* a wrapper around realloc and free
	oldsize	| newsize	| action
	0		| > 0		| allocate new buffer with newsize
	> 0		| 0			| free ptr
	> 0		| < oldsize	| shrinks buffer
	> 0		| > oldsize | reallocate ptr, return new buffer, always valid
	> 0		| ==oldsize | nop
*/
void* reallocate(void* ptr, size_t oldsize, size_t newsize);

#endif /* _CLOX_MEMORY_H_ */

