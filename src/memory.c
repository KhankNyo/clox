

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "include/common.h"
#include "include/memory.h"




#if  defined(DEBUG)
#  define BUFFER_ALLOCATION_CHK
#  define MAGIC_FRONT_VALUE 0xdeadbeef
#  define MAGIC_BACK_VALUE 0xbadf00d
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

#define GET_HEADER(ptr) (FreeHeader_t*)(((uint8_t*)(ptr)) - sizeof(FreeHeader_t))
#define GET_PTR(header_ptr) (((uint8_t*)(header_ptr)) + sizeof(FreeHeader_t))

#define MIN_SIZE 4
#define MIN_BUF_SIZE (MIN_SIZE + sizeof(FreeHeader_t))





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


#ifdef BUFFER_ALLOCATION_CHK
	node->_magic_front = MAGIC_FRONT_VALUE;
	node->_magic_back = MAGIC_BACK_VALUE;
#endif /* BUFFER_ALLOCATION_CHK */


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
#ifdef BUFFER_ALLOCATION_CHK
	const FreeHeader_t* header = GET_HEADER(ptr);
	if (header->_magic_front != MAGIC_FRONT_VALUE)
	{
		fprintf(stderr, "Header of buffer at location %p was overwritten with %x\n", ptr, header->_magic_front);
		abort();
	}
	
	if (header->_magic_back != MAGIC_BACK_VALUE)
	{
		fprintf(stderr, "Back of header of buffer at location %p was overwritten with %x\n", ptr, header->_magic_back);
		abort();
	}
#endif /* BUFFER_ALLOCATION_CHK */


	insert_free_node(allocator, GET_HEADER(ptr));
}






















static FreeHeader_t* get_free_node(Allocator_t* allocator, bufsize_t nbytes)
{
	FreeHeader_t* curr = allocator->free_head;
	FreeHeader_t* prev = allocator->free_head;
	while (NULL != curr && curr->capacity < nbytes)
	{
		prev = curr;
		curr = curr->next;
	}


	if (NULL == curr)
	{
		/*
			if we cannot find a free node with appropriate size, 
			then we split the remaining buffer into the node we need and a free node
		*/
		Split_t split = split_free_buffer(nbytes,
			&allocator->head[allocator->used_size], 
			allocator->capacity - allocator->used_size
		);

		if (NULL == allocator->free_head)
		{
			allocator->free_head = split.new_free_node;
		}
		else if (NULL != split.new_free_node)
		{
			insert_free_node(allocator, split.new_free_node);
		}
		return split.new_node;
	}

	prev->next = curr->next;
	curr->next = NULL;
	return curr;
}



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

	/* curr->size >= node->size */
	if (NULL == prev)
	{
		node->next = curr;
		curr->next = NULL;
		allocator->free_head = node;
		return;
	}
	prev->next = node;
	node->next = curr;
}





static Split_t split_free_buffer(bufsize_t new_node_capacity, uint8_t* bufstart, bufsize_t bufsize)
{
	Split_t split = { 0 };
	/* if the whole buffer is not big enough, split is all NULL */
	if (new_node_capacity + sizeof(FreeHeader_t) > bufsize)
		return split;


	split.new_node = (FreeHeader_t*)bufstart;
	split.new_node->capacity = new_node_capacity;


	/* if the other half is not big enough, new_free_node is NULL */
	if (bufsize - new_node_capacity < MIN_BUF_SIZE)
		return split;
	
	split.new_free_node = (FreeHeader_t*)&bufstart[new_node_capacity + sizeof(FreeHeader_t)];
	split.new_free_node->next = NULL;
	split.new_free_node->capacity = bufsize - new_node_capacity - sizeof(FreeHeader_t);
	return split;
}




