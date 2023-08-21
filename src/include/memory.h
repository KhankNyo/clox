#ifndef _CLOX_MEMORY_H_
#define _CLOX_MEMORY_H_

#include "common.h"

typedef struct FreeHeader_t FreeHeader_t;
typedef size_t bufsize_t;	/* configurable size */
typedef struct Allocator_t
{
	uint8_t* head;
	bufsize_t capacity;
	bufsize_t used_size;

	FreeHeader_t* free_head;
} Allocator_t;


/* TODO: thread safe */
extern Allocator_t g_alloc;



/* CLox macros */

#define GROW_CAPACITY(cap)\
	((cap) < 8 ? 8 : (cap) * 2)

#define GROW_ARRAY(type, ptr, oldsize, newsize)\
	Allocator_Reallocate(&g_alloc, ptr, (oldsize) * (sizeof(type)), newsize * sizeof(type))

#define FREE_ARRAY(type, ptr, oldsize)\
	Allocator_Reallocate(&g_alloc, ptr, sizeof(type) * (oldsize), 0)


/* global allocator macro */

#define GALLOCATOR_INIT(initial_capacity) Allocator_Init(&g_alloc, initial_capacity)
#define GALLOC(nbyte) Allocator_Alloc(&g_alloc, nbyte)
#define GFREE(ptr) Allocator_Free(&g_alloc, ptr)
#define GALLOCATOR_DESTROY() Allocator_KillEmAll(&g_alloc)











/* initializes the allocator (free list allocator) */
void Allocator_Init(Allocator_t* allocator, bufsize_t initial_capacity);

/* free all memory allocated by the allocator */
void Allocator_KillEmAll(Allocator_t* allocator);


/* 
*	\param 1 allocator: the allocator struct, must NOT be NULL
*	\param 2 nbytes: the size of the buffer 
	\returns a buffer with the capacity specified by nbytes
	\returns NULL if out of memory or nbytes is zero
*/
void* Allocator_Alloc(Allocator_t* allocator, bufsize_t nbytes);

/*
	frees a buffer returned by Allocator_Alloc
*/
void Allocator_Free(Allocator_t* allocator, void* ptr);



/* a wrapper around Allocator_Alloc and Allocator_Free
	oldsize	| newsize	| action
	0		| > 0		| allocates new buffer with newsize, returns ptr
	> 0		| 0			| frees ptr, returns NULL
	> 0		| < oldsize	| shrinks buffer, return the same ptr
	> 0		| > oldsize | reallocates ptr, return the same or a new ptr
	> 0		| ==oldsize | nop, returns NULL
*/
void* Allocator_Reallocate(Allocator_t* allocator, void* ptr, bufsize_t oldsize, bufsize_t newsize);










#endif /* _CLOX_MEMORY_H_ */

