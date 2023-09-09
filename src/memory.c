

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "include/common.h"
#include "include/memory.h"




#if  defined(DEBUG)
#  define BUFFER_ALLOCATION_CHK
typedef enum Magic_t {
    MAGIC_FRONT_ALLOC = 1,
    MAGIC_BACK_ALLOC,
    MAGIC_FRONT_FREE,
    MAGIC_BACK_FREE,
} Magic_t;
#endif /* DEBUG */



struct FreeHeader_t
{
#ifdef BUFFER_ALLOCATION_CHK
	Magic_t _magic_front;
	bufsize_t capacity;
	FreeHeader_t* next;
    Magic_t _magic_back;
#else
    bufsize_t capacity;
    FreeHeader_t* next;
#endif /* BUFFER_ALLOCATION_CHK */ 
};


typedef enum NodeType_t
{
    NODE_FREED,
    NODE_ALIVE,
} NodeType_t;


typedef struct Split_t
{
	FreeHeader_t* new_node;
	FreeHeader_t* new_free_node;
} Split_t;



static bool extend_capacity(Allocator_t* allocator, FreeHeader_t* header, NodeType_t type, bufsize_t newcap);
static FreeHeader_t* get_free_node(Allocator_t* allocator, bufsize_t nbytes);
static void insert_free_node(Allocator_t* allocator, FreeHeader_t* node);
static Split_t split_node(FreeHeader_t* node, NodeType_t type, bufsize_t new_size);
static void set_header(FreeHeader_t* header, size_t capacity, FreeHeader_t* next, NodeType_t type);
static void dbg_print_nodes(const Allocator_t allocator, const char* title);

#define GET_HEADER(ptr) (FreeHeader_t*)(((uint8_t*)(ptr)) - sizeof(FreeHeader_t))
#define GET_PTR(header_ptr) (((uint8_t*)(header_ptr)) + sizeof(FreeHeader_t))

#define MIN_SIZE 8
#define ANY_SIZE ((size_t)-1)
#define MIN_CAPACITY (MIN_SIZE + sizeof(FreeHeader_t))





void Allocator_Init(Allocator_t* allocator, bufsize_t initial_capacity)
{
	allocator->capacity = initial_capacity + sizeof(FreeHeader_t);
    allocator->auto_defrag = false;
	allocator->head = malloc(initial_capacity + sizeof(FreeHeader_t));
	if (NULL == allocator->head)
	{
		fprintf(stderr, 
			"Cannot initialize allocator because malloc returned NULL, "
            "size requested: %"PRIu64"\n", 
            (uint64_t)initial_capacity
        );
		exit(EXIT_FAILURE);
	}

	allocator->free_head = (FreeHeader_t*)allocator->head;
    allocator->free_head->next = NULL;
    allocator->free_head->capacity = initial_capacity;
}


void Allocator_KillEmAll(Allocator_t* allocator)
{
	free(allocator->head);
	allocator->head = NULL;
	allocator->free_head = NULL;
	allocator->capacity = 0;
}





void* Allocator_Alloc(Allocator_t* allocator, bufsize_t nbytes)
{
    dbg_print_nodes(*allocator, "Allocating");
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
    if (header->capacity < newsize && 
        ((void)dbg_print_nodes(*alloc, "Resizing"),
        !extend_capacity(alloc, header, NODE_ALIVE, newsize)))
    {
		void* newbuf = Allocator_Alloc(alloc, newsize);
		memcpy(newbuf, ptr, header->capacity);
		Allocator_Free(alloc, ptr);
		return newbuf;
	}
    return ptr;
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
		fprintf(stderr, "Header of buffer at location %p was overwritten with %x\n", 
            ptr, header->_magic_front
        );
		abort();
	}
	
	if (header->_magic_back != MAGIC_BACK_ALLOC)
	{
		fprintf(stderr, "Back of header of buffer at location %p was overwritten with %x\n", 
            ptr, header->_magic_back
        );
		abort();
	}
#endif /* BUFFER_ALLOCATION_CHK */

    dbg_print_nodes(*allocator, "Freeing");
	insert_free_node(allocator, GET_HEADER(ptr));
}


/* TODO: loop instead of recursion */
void Allocator_Defrag(Allocator_t* allocator, size_t num_pointers)
{
    if (0 == num_pointers 
    || NULL == allocator->free_head
    || NULL == allocator->free_head->next)
    {
        return;
    }

    FreeHeader_t* curr = allocator->free_head;
    allocator->free_head = curr->next;

    dbg_print_nodes(*allocator, "Defragging");
    extend_capacity(allocator, curr, NODE_FREED, ANY_SIZE);
    insert_free_node(allocator, curr);

    Allocator_Defrag(allocator, num_pointers - 1);
}















static bool extend_capacity(Allocator_t* allocator, FreeHeader_t* node, NodeType_t type, bufsize_t newcap)
{
    if (NULL == allocator->free_head)
    {
        return false;
    }

    const uint8_t* node_edge = ((uint8_t*)node) + node->capacity + sizeof(FreeHeader_t);
    FreeHeader_t* prev = NULL;
    FreeHeader_t* curr = allocator->free_head;
    /* stop search if we found 2 consecutive nodes */
    while (NULL != curr && node_edge != (const uint8_t*)curr)
    {
        prev = curr;
        curr = curr->next;
    }


    if (NULL == curr)
        return false;

    const size_t new_node_capacity = 
        node->capacity + curr->capacity + sizeof(FreeHeader_t);
    if (ANY_SIZE == newcap)
        newcap = new_node_capacity;
    else if (newcap > new_node_capacity)
        return false;


    if (NULL == prev)
    {
        allocator->free_head = curr->next;
    }
    else 
    {
        prev->next = curr->next;
    }

    curr->next = NULL;
    node->capacity = new_node_capacity;
    Split_t split = split_node(node, type, newcap);
    insert_free_node(allocator, split.new_free_node);
    return true;
}





static FreeHeader_t* get_free_node(Allocator_t* allocator, bufsize_t nbytes)
{
    FreeHeader_t* prev = NULL;
	FreeHeader_t* curr = allocator->free_head;
	while (NULL != curr && curr->capacity < nbytes)
	{
		prev = curr;
		curr = curr->next;
	}


    if (NULL == curr)
    {
        if (!allocator->auto_defrag)
        {
            return NULL;
        }

        Allocator_Defrag(allocator, ALLOCATOR_DEFRAG_DEFAULT);
        prev = NULL;
        curr = allocator->free_head;
        while (NULL != curr && curr->capacity < nbytes)
        {
            prev = curr;
            curr = curr->next;
        }

        if (NULL == curr)
        {
            return NULL;
        }
    }


    /* remove the node we just found from the free list */
    if (NULL == prev)
    {
        allocator->free_head = curr->next;
    }
    else 
    {
        prev->next = curr->next;
    }
    /* alive from the dead muahhahahahh */
    Split_t split = split_node(curr, NODE_ALIVE, nbytes);
    insert_free_node(allocator, split.new_free_node);
    return split.new_node;
}



static void insert_free_node(Allocator_t* allocator, FreeHeader_t* node)
{
    if (NULL == node)
    {
        return;
    }
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


    if (NULL == curr)
    {
        CLOX_ASSERT(prev != NULL && "unreachable");
        prev->next = node;
        return;
    }


    if (NULL == prev)
    {
        allocator->free_head = node;
    }
    else 
    {
        prev->next = node;
    }
    node->next = curr;
}





static Split_t split_node(FreeHeader_t* node, NodeType_t type, bufsize_t new_size)
{
    const size_t total_capacity = node->capacity + sizeof(FreeHeader_t);
    if (new_size % MIN_SIZE)
        new_size = (new_size + MIN_SIZE) & ~(MIN_SIZE - 1);
    else if (new_size < MIN_SIZE)
        new_size = MIN_SIZE;


    Split_t split = {.new_node = node, .new_free_node = NULL};
    const size_t taken = new_size + sizeof(FreeHeader_t);
    if (total_capacity < taken)
        return split;


    set_header(split.new_node, new_size, NULL, type);
    if (total_capacity < taken + MIN_CAPACITY)
        return split;


    split.new_free_node = (FreeHeader_t*)((uint8_t*)node + taken);
    set_header(split.new_free_node, 
        total_capacity - taken - sizeof(FreeHeader_t), NULL, NODE_FREED
    );
    return split;
}



static void set_header(FreeHeader_t* header, size_t capacity, FreeHeader_t* next, NodeType_t type)
{
#ifdef BUFFER_ALLOCATION_CHK
	if (type == NODE_ALIVE)
	{
		header->_magic_back = MAGIC_BACK_ALLOC;
		header->_magic_front = MAGIC_FRONT_ALLOC;
	}
	else
	{
		header->_magic_back = MAGIC_BACK_FREE;
		header->_magic_front = MAGIC_FRONT_FREE;
	}
#else
    (void)type;
#endif /* BUFFER_ALLOCATION_CHK */

	header->next = next;
	header->capacity = capacity;
}



static void dbg_print_nodes(const Allocator_t allocator, const char* title)
{
#ifdef DEBUG
    fprintf(stdout, "\n <== ALLOCATOR: %s ==> \n", title);
    const FreeHeader_t* node = allocator.free_head;
    int indent = 0;
    while (NULL != node)
    {
        fprintf(stdout, "%-*sNode %p: capacity: %zu\n", indent, " ", (void*)node, node->capacity);
        indent += 2;
        node = node->next;
    }
    fprintf(stdout, "\n");
#else
    (void)allocator;
    (void)title;
#endif 
}

