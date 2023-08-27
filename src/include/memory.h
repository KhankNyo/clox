#ifndef _CLOX_MEMORY_H_
#define _CLOX_MEMORY_H_

#include "common.h"




/* CLox macros */

#define ALLOCATE(p_allocator, type, nbytes)\
	Allocator_Reallocate(p_allocator, NULL, 0, sizeof(type) * nbytes)

#define FREE(p_allocator, type, ptr)\
	Allocator_Reallocate(p_allocator, ptr, sizeof(type), 0)

#define GROW_CAPACITY(cap)\
	((cap) < 8 ? 8 : (cap) * 2)

#define GROW_ARRAY(p_allocator, type, ptr, oldsize, newsize)\
	Allocator_Reallocate(p_allocator, ptr, (oldsize) * (sizeof(type)), newsize * sizeof(type))

#define FREE_ARRAY(p_allocator, type, ptr, oldsize)\
	Allocator_Reallocate(p_allocator, ptr, sizeof(type) * (oldsize), 0)









typedef struct FreeHeader_t FreeHeader_t;
#ifndef bufsize_t
	typedef size_t bufsize_t;	/* affects the header's size, is configurable */
#endif /* bufsize_t */

typedef struct Allocator_t
{
	uint8_t* head;
	bufsize_t capacity;
	bufsize_t used_size;

	FreeHeader_t* free_head;
} Allocator_t;

/* 
 *   \param 1: allocator: the allocator struct
 *   \param 2: capacity: the total amount of memory the given struct shall have
 *   initializes the allocator (free list allocator) 
 */
void Allocator_Init(Allocator_t* allocator, bufsize_t capacity);

/* 
 * free all memory allocated by the allocator 
 */
void Allocator_KillEmAll(Allocator_t* allocator);


/* 
 *   \param 1 allocator: the allocator struct, must NOT be NULL
 *   \param 2 nbytes: the size of the buffer 
 *   \returns always a valid buffer with the capacity specified by nbytes,
 *	 if out of memory, this function will call exit(1); and will log a message to stderr
*/
void* Allocator_Alloc(Allocator_t* allocator, bufsize_t nbytes);

/*
 *   frees a buffer returned by Allocator_Alloc
 */
void Allocator_Free(Allocator_t* allocator, void* ptr);



/* a wrapper around Allocator_Alloc and Allocator_Free
 *   oldsize | newsize   | action
 *   0       | > 0       | Allocator_Alloc
 *   > 0     | 0         | Allocator_Free
 *   > 0     | < oldsize | nop, returns ptr
 *   > 0     | > oldsize | reallocates ptr, return the same or a new ptr to a buf with the requested size
 *   > 0     | ==oldsize | nop, returns NULL
 */
void* Allocator_Reallocate(Allocator_t* allocator, void* ptr, bufsize_t oldsize, bufsize_t newsize);










#endif /* _CLOX_MEMORY_H_ */

