#ifndef _CLOX_MEMORY_H_
#define _CLOX_MEMORY_H_

#include "common.h"
#include "typedefs.h"
#include "value.h"



/* CLox macros */

#define GC_HEAP_GROW_FACTOR 2

#define ALLOCATE(p_vm, type, nbytes)\
	GC_Reallocate(p_vm, NULL, 0, sizeof(type) * nbytes)

#define FREE(p_vm, type, ptr)\
	GC_Reallocate(p_vm, ptr, sizeof(type), 0)

#define GROW_CAPACITY(cap)\
	((cap) < 8 ? 8 : (cap) * 2)

#define GROW_ARRAY(p_vm, type, ptr, oldsize, newsize)\
	GC_Reallocate(p_vm, ptr, (oldsize) * (sizeof(type)), newsize * sizeof(type))

#define FREE_ARRAY(p_vm, type, ptr, oldsize)\
	GC_Reallocate(p_vm, ptr, sizeof(type) * (oldsize), 0)









#ifndef bufsize_t
	typedef size_t bufsize_t;	/* affects the header's size, is configurable */
#endif /* bufsize_t */

typedef struct Allocator_t
{
	uint8_t* head;
	bufsize_t capacity;
	FreeHeader_t* free_head;
    bool auto_defrag;
} Allocator_t;

/* 
 *   \param 1: allocator: the allocator struct
 *   \param 2: capacity: the total amount of memory the given struct shall have
 *   initializes the allocator (free list allocator) 
 */
void Allocator_Init(Allocator_t* allocator, bufsize_t capacity);

/* 
 * free ALL memory allocated by the allocator,
 * even ones that have not been passed to Allocator_Free
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
 *   frees a buffer returned by Allocator_Alloc or Allocator_Realloc
 */
void Allocator_Free(Allocator_t* allocator, void* ptr);


/* 
 *   resize the buffer given by this function or Allocator_Alloc
 *   if NULL == ptr, return pointer given by Allocatro_Alloc, 
 *   if 0 == newsize, acts like Allocator_Free and returns NULL
 */
void* Allocator_Realloc(Allocator_t* allocator, void* ptr, bufsize_t newsize);



/* a wrapper around Allocator_Realloc
 *   oldsize | newsize   | action
 *   0       | > 0       | Allocator_Alloc
 *   > 0     | 0         | Allocator_Free
 *   > 0     | < oldsize | nop, returns ptr
 *   > 0     | > oldsize | reallocates ptr, return the same or a new ptr to a buf with the requested size
 *   > 0     | ==oldsize | nop, returns NULL
 */
void* GC_Reallocate(VM_t* vm, void* ptr, bufsize_t oldsize, bufsize_t newsize);


/* Clox vm's gc
 *  Searches for objects from root, and mark them as reachable
 */
void GC_CollectGarbage(VM_t* vm);

/* mark a value as reachable */
void GC_MarkVal(VM_t* vm, Value_t val);

/* mark an object as reachable */
void GC_MarkObj(VM_t* vm, Obj_t* obj);






/* 
 * defragments memory manually, 
 * highly recommended to use in places that does not need to be performant
 *
 * NOTE: this function uses recursion to defrag memory,
 *      so it is best to use a small number for num_pointers to prevent stack overflow
 *
 * if auto_defrag is true
 *  this function is automatically called in Allocator_Alloc if it cannot find a free list of memory
 */
#define ALLOCATOR_DEFRAG_DEFAULT 16
void Allocator_Defrag(Allocator_t* allocator, size_t num_pointers);










#endif /* _CLOX_MEMORY_H_ */

