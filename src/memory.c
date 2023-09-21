

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "include/common.h"
#include "include/memory.h"
#include "include/debug.h"
#include "include/vm.h"
#include "include/value.h"
#include "include/compiler.h"




#ifdef DEBUG_ALLOCATION_CHK
typedef enum Magic_t {
    MAGIC_FRONT_ALLOC = 1,
    MAGIC_BACK_ALLOC,
    MAGIC_FRONT_FREE,
    MAGIC_BACK_FREE,
} Magic_t;
#endif /* DEBUG */



struct FreeHeader_t
{
#ifdef DEBUG_ALLOCATION_CHK
	Magic_t _magic_front;
	bufsize_t capacity;
	FreeHeader_t* next;
    Magic_t _magic_back;
#else
    bufsize_t capacity;
    FreeHeader_t* next;
#endif /* DEBUG_ALLOCATION_CHK */ 
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


static void gc_mark_root(VM_t* vm);
static void gc_trace_references(VM_t* vm);
static void gc_blacken_obj(VM_t* vm, Obj_t* obj);
static void gc_mark_valarr(VM_t* vm, ValueArr_t* va);
static void gc_sweep(VM_t* vm);

#define GET_HEADER(ptr) ((FreeHeader_t*)(((uint8_t*)(ptr)) - sizeof(FreeHeader_t)))
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
	FreeHeader_t* node = get_free_node(allocator, nbytes);

    DEBUG_ALLOC_PRINT("\nAllocated pointer: %p, size: %u", GET_PTR(node), (unsigned)node->capacity);
    dbg_print_nodes(*allocator, "Allocating");
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



void* Allocator_Realloc(Allocator_t* allocator, void* ptr, bufsize_t newsize)
{
    if (NULL == ptr)
    {
        return Allocator_Alloc(allocator, newsize);
    }
    if (0 == newsize)
    {
        Allocator_Free(allocator, ptr);
        return NULL;
    }

    FreeHeader_t* header = GET_HEADER(ptr);
    if (header->capacity < newsize)
    {
        dbg_print_nodes(*allocator, "Resizing");
        if (!extend_capacity(allocator, header, NODE_ALIVE, newsize))
        {
            void* newbuf = Allocator_Alloc(allocator, newsize);
            memcpy(newbuf, ptr, header->capacity);
            Allocator_Free(allocator, ptr);
            return newbuf;
        }
    }
    return ptr;
}




void Allocator_Free(Allocator_t* allocator, void* ptr)
{
	if (NULL == ptr)
	{
		return;
	}

#ifdef DEBUG_ALLOCATION_CHK
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
    memset(ptr, 0, GET_HEADER(ptr)->capacity);
#endif /* DEBUG_ALLOCATION_CHK */


    DEBUG_ALLOC_PRINT("\nFreeing pointer: %p, size: %u", ptr, (unsigned)(GET_HEADER(ptr)->capacity));
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










void* GC_Reallocate(VM_t* vm, void* ptr, bufsize_t oldsize, bufsize_t newsize)
{
    if (oldsize < newsize)
    {
        vm->bytes_allocated += newsize - oldsize;
#ifdef DEBUG_STRESS_GC
        GC_CollectGarbage(vm);
#endif /* DEBUG_STRESS_GC */
        if (vm->bytes_allocated > vm->next_gc)
        {
            GC_CollectGarbage(vm);
        }
    }


    if (0 == newsize)
    {
        Allocator_Free(vm->alloc, ptr);
        return NULL;
    }
    return Allocator_Realloc(vm->alloc, ptr, newsize);
}


void GC_CollectGarbage(VM_t* vm)
{
    DEBUG_GC_PRINT("-- gc begin\n");

    size_t before_gc = vm->bytes_allocated;

    gc_mark_root(vm);
    gc_trace_references(vm);
    Table_RemoveWhite(&vm->strings);
    gc_sweep(vm);
    vm->next_gc = vm->bytes_allocated * GC_HEAP_GROW_FACTOR;

    DEBUG_GC_PRINT("-- gc end\n");
    DEBUG_GC_PRINT("   collected %zu bytes (from %zu to %zu) next at %zu\n",
        before_gc - vm->bytes_allocated, 
        before_gc, vm->bytes_allocated,
        vm->next_gc
    );
}




void GC_MarkVal(VM_t* vm, Value_t val)
{
    if (IS_OBJ(val))
    {
        GC_MarkObj(vm, AS_OBJ(val));
    }
}


void GC_MarkObj(VM_t* vm, Obj_t* obj)
{
    if (NULL == obj || obj->is_marked)
        return;

#ifdef DEBUG_LOG_GC
    fprintf(GC_LOG_FILE, "%p mark ", (void*)obj);
    Value_Print(GC_LOG_FILE, OBJ_VAL(obj));
    fputc('\n', GC_LOG_FILE);
#endif /* DEBUG_LOG_GC */

    obj->is_marked = true;

    if (vm->gray_count + 1 > vm->gray_capacity)
    {
        vm->gray_capacity = GROW_CAPACITY(vm->gray_capacity);
        vm->gray_stack = Allocator_Realloc(vm->alloc, 
            vm->gray_stack, vm->gray_capacity * sizeof(Obj_t*)
        );
    }
    vm->gray_stack[vm->gray_count++] = obj;
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


void panic()
{
    CLOX_ASSERT(false && "panic");
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
    if (curr == node)
    {
        panic();
        CLOX_ASSERT(curr != node && "double free");
    }


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
#ifdef DEBUG_ALLOCATION_CHK
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
#endif /* DEBUG_ALLOCATION_CHK */

	header->next = next;
	header->capacity = capacity;
}



static void dbg_print_nodes(const Allocator_t allocator, const char* title)
{
#ifdef ALLOCATOR_DEBUG
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









static void gc_mark_root(VM_t* vm)
{
    for (Value_t* val = vm->stack; val < vm->sp; val++)
    {
        GC_MarkVal(vm, *val);
    }

    for (int i = 0; i < vm->frame_count; i++)
    {
        GC_MarkObj(vm, (Obj_t*)vm->frames[i].closure);
    }

    for (ObjUpval_t* upval = vm->open_upvals; 
        NULL != upval; 
        upval = upval->next)
    {
        GC_MarkObj(vm, (Obj_t*)upval);
    }

    Table_Mark(&vm->globals);
    Compiler_MarkObj(vm->compiler);
}



static void gc_trace_references(VM_t* vm)
{
    while (vm->gray_count > 0)
    {
        Obj_t* obj = vm->gray_stack[--vm->gray_count];
        gc_blacken_obj(vm, obj);
    }
}


static void gc_blacken_obj(VM_t* vm, Obj_t* obj)
{
#ifdef DEBUG_LOG_GC
    fprintf(stderr, "%p blacken ", (void*)obj);
    Value_Print(stderr, OBJ_VAL(obj));
    fputc('\n', stderr);
#endif /* DEBUG_LOG_GC */

    switch (obj->type)
    {
    case OBJ_NATIVE:
    case OBJ_STRING:
        break;

    case OBJ_INSTANCE:
    {
        ObjInstance_t* inst = (ObjInstance_t*)obj;
        GC_MarkObj(vm, (Obj_t*)inst->klass);
        Table_Mark(&inst->fields);
    }
    break;

    case OBJ_CLASS:
    {
        ObjClass_t* klass = (ObjClass_t*)obj;
        GC_MarkObj(vm, (Obj_t*)klass->name);
    }
    break;

    case OBJ_UPVAL:
        GC_MarkVal(vm, ((ObjUpval_t*)obj)->closed);
        break;

    case OBJ_FUNCTION:
    {
        ObjFunction_t* fun = (ObjFunction_t*)obj;
        GC_MarkObj(vm, (Obj_t*)fun->name);
        gc_mark_valarr(vm, &fun->chunk.consts);
    }
    break;

    case OBJ_CLOSURE:
    {
        ObjClosure_t* closure = (ObjClosure_t*)obj;
        GC_MarkObj(vm, (Obj_t*)closure->fun);
        for (int i = 0; i < closure->upval_count; i++)
        {
            GC_MarkObj(vm, (Obj_t*)closure->upvals[i]);
        }
    }
    break;
    }
}


static void gc_mark_valarr(VM_t* vm, ValueArr_t* va)
{
    for (size_t i = 0; i < va->size; i++)
    {
        GC_MarkVal(vm, va->vals[i]);
    }
}



static void gc_sweep(VM_t* vm)
{
    Obj_t* prev = NULL;
    Obj_t* curr = vm->head;
    size_t freed_count = 0;
    while (NULL != curr)
    {
        if (curr->is_marked) /* then advance */
        {
            curr->is_marked = false; /* for next sweep cycle */
            prev = curr;
            curr = curr->next;
        }
        else 
        {
            Obj_t* the_sinful = curr;

            curr = curr->next;
            if (NULL == prev)
                vm->head = curr;
            else
                prev->next = curr;

            Obj_Free(vm, the_sinful);
            freed_count += 1;
        }
    }


    Allocator_Defrag(vm->alloc, freed_count);
}

