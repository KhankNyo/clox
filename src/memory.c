

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "include/common.h"
#include "include/memory.h"




#if  defined(DEBUG)
#  define BUFFER_ALLOCATION_CHK
#  define MAGIC_FRONT_ALLOC 0x0df00d90
#  define MAGIC_BACK_ALLOC 0xefbe0d90
#  define MAGIC_FRONT_FREE 0xefbeadde
#  define MAGIC_BACK_FREE 0x0df0adba
#endif /* DEBUG */



struct FreeHeader_t
{
#ifdef BUFFER_ALLOCATION_CHK
	uint32_t _magic_front;
#endif /* BUFFER_ALLOCATION_CHK */

	bufsize_t capacity;
	FreeHeader_t* next;

#ifdef BUFFER_ALLOCATION_CHK
	uint32_t _magic_back;
#endif /* BUFFER_ALLOCATION_CHK */ 
};


typedef struct Split_t
{
	FreeHeader_t* new_node;
	FreeHeader_t* new_free_node;
} Split_t;



static FreeHeader_t* get_free_node(Allocator_t* allocator, bufsize_t nbytes);
static void insert_free_node(Allocator_t* allocator, FreeHeader_t* node);
static Split_t split_free_buffer(bufsize_t new_node_capacity, uint8_t* bufstart, bufsize_t bufsize);
static void set_header(FreeHeader_t* header, size_t capacity, FreeHeader_t* next, bool is_freed);

#define GET_HEADER(ptr) (FreeHeader_t*)(((uint8_t*)(ptr)) - sizeof(FreeHeader_t))
#define GET_PTR(header_ptr) (((uint8_t*)(header_ptr)) + sizeof(FreeHeader_t))

#define MIN_SIZE 16
#define MIN_CAPACITY (MIN_SIZE + sizeof(FreeHeader_t))





void Allocator_Init(Allocator_t* allocator, bufsize_t initial_capacity)
{
	allocator->used_size = 0;
	allocator->capacity = initial_capacity;
	allocator->head = malloc(initial_capacity + sizeof(FreeHeader_t));
	if (NULL == allocator->head)
	{
		fprintf(stderr, 
			"Cannot initialize allocator because malloc returned NULL, size requested: %llu\n", (uint64_t)initial_capacity);
		exit(EXIT_FAILURE);
	}

	allocator->free_head = NULL;
}


void Allocator_KillEmAll(Allocator_t* allocator)
{
	free(allocator->head);
	allocator->head = NULL;
	allocator->free_head = NULL;
	allocator->used_size = 0;
	allocator->capacity = 0;
}





void* Allocator_Alloc(Allocator_t* allocator, bufsize_t nbytes)
{
	FreeHeader_t* node = get_free_node(allocator, nbytes);
	if (NULL == node)
	{
		fprintf(stderr, 
			"Out of memory, hint: initialize allocator struct at %p with more capacity\n",
			(void*)allocator
		);
		exit(EXIT_FAILURE);
	}


	return GET_PTR(node);
}



void* Allocator_Reallocate(Allocator_t* alloc, void* ptr, bufsize_t oldsize, bufsize_t newsize)
{
	if (newsize == oldsize) 
	{
		return NULL;
	}
	if (0 == oldsize)
	{
		return Allocator_Alloc(alloc, newsize);
	}
	if (0 == newsize)
	{
		Allocator_Free(alloc, ptr);
		return NULL;
	}
	
	FreeHeader_t* header = GET_HEADER(ptr);
	if (header->capacity >= newsize)
	{
		return ptr;
	}
	else
	{
		void* newbuf = Allocator_Alloc(alloc, newsize);
		memcpy(newbuf, ptr, header->capacity);
		Allocator_Free(alloc, ptr);
		return newbuf;
	}
}



void Allocator_Free(Allocator_t* allocator, void* ptr)
{
	if (NULL == ptr)
	{
		return;
	}
#ifdef BUFFER_ALLOCATION_CHK
	FreeHeader_t* header = GET_HEADER(ptr);
	if (header->_magic_front != MAGIC_FRONT_ALLOC)
	{
		fprintf(stderr, "Header of buffer at location %p was overwritten with %x\n", ptr, header->_magic_front);
		abort();
	}
	
	if (header->_magic_back != MAGIC_BACK_ALLOC)
	{
		fprintf(stderr, "Back of header of buffer at location %p was overwritten with %x\n", ptr, header->_magic_back);
		abort();
	}
#endif /* BUFFER_ALLOCATION_CHK */


	insert_free_node(allocator, GET_HEADER(ptr));
}






















static FreeHeader_t* get_free_node(Allocator_t* allocator, bufsize_t nbytes)
{
	if (NULL == allocator->free_head)
	{
		Split_t split = split_free_buffer(nbytes, 
			&allocator->head[allocator->used_size], 
			allocator->capacity - allocator->used_size
		);
		if (split.new_free_node != NULL)
		{
			allocator->free_head = split.new_free_node;
		}
		return split.new_node;
	}


	FreeHeader_t* prev = NULL;
	FreeHeader_t* curr = allocator->free_head;
	while (NULL != curr && curr->capacity < nbytes)
	{
		prev = curr;
		curr = curr->next;
	}


	Split_t split = { 0 };
	if (NULL == curr) /* no free buffer has enough capacity */
	{
		split = split_free_buffer(nbytes,
			&allocator->head[allocator->used_size],
			allocator->capacity - allocator->used_size
		);
	}
	else if (NULL == prev)	/* the first buffer have enough capacity */
	{
		FreeHeader_t* next = curr->next;
		split = split_free_buffer(nbytes, (uint8_t*)curr, curr->capacity + sizeof(FreeHeader_t));
		allocator->free_head = next;
	}
	else /* found a free buffer that has an appropriate size */
	{
		prev->next = curr->next;
		split = split_free_buffer(nbytes, (uint8_t*)curr, curr->capacity + sizeof(FreeHeader_t));
	}
	

	if (split.new_free_node != NULL)
	{
		insert_free_node(allocator, split.new_free_node);
	}
	return split.new_node;
}



/* TODO: replace size-order insertion with address-order insertion for coalessing later */
static void insert_free_node(Allocator_t* allocator, FreeHeader_t* node)
{
	if (NULL == allocator->free_head)
	{
		allocator->free_head = node;
		return;
	}

	FreeHeader_t* prev = NULL;
	FreeHeader_t* curr = allocator->free_head;
	while (NULL != curr && curr->capacity < node->capacity)
	{
		prev = curr;
		curr = curr->next;
	}
	CLOX_ASSERT(curr != node && "double free");

	set_header(node,      node->capacity, curr, true);
	if (NULL == prev)
	{
		allocator->free_head = node;
		return;
	}
	prev->next = node;
}





static Split_t split_free_buffer(bufsize_t alloc_size, uint8_t* bufstart, bufsize_t bufsize)
{
	if (alloc_size < MIN_SIZE)
	{
		alloc_size = MIN_SIZE;
	}
	else if (alloc_size % MIN_SIZE)
	{
		alloc_size += MIN_SIZE - (alloc_size % MIN_SIZE);
	}



	Split_t split = { 0 };


	/* if the whole buffer is not big enough, split is all NULL */
	const size_t new_node_capacity = alloc_size + sizeof(FreeHeader_t);
	if (new_node_capacity > bufsize)
		return split;
	split.new_node = (FreeHeader_t*)bufstart;
	set_header(split.new_node, alloc_size, NULL, false);


	/* if the other half is not big enough, new_free_node is NULL */
	const size_t new_free_capacity = bufsize - new_node_capacity;
	if (new_free_capacity >= MIN_CAPACITY)
	{
		split.new_free_node = (FreeHeader_t*)&bufstart[new_node_capacity];
		set_header(split.new_free_node, new_free_capacity - sizeof(FreeHeader_t), NULL, true);
	}
	return split;
}



static void set_header(FreeHeader_t* header, size_t capacity, FreeHeader_t* next, bool is_freed)
{
	(void)is_freed;
#ifdef BUFFER_ALLOCATION_CHK
	if (!is_freed)
	{
		header->_magic_back = MAGIC_BACK_ALLOC;
		header->_magic_front = MAGIC_FRONT_ALLOC;
	}
	else
	{
		header->_magic_back = MAGIC_BACK_FREE;
		header->_magic_front = MAGIC_FRONT_FREE;
	}
#endif /* BUFFER_ALLOCATION_CHK */

	header->next = next;
	header->capacity = capacity;
}




