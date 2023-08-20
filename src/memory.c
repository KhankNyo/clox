

#include <stdlib.h>
#include <stdio.h>
#include "include/common.h"
#include "include/memory.h"




void* reallocate(void* ptr, size_t oldsize, size_t newsize)
{
	if (0 == newsize)
	{
		free(ptr);
		return NULL;
	}

	void* newptr = realloc(ptr, newsize);
	if (NULL == newptr)
	{
		perror("realloc");
		exit(EXIT_FAILURE);
	}
	return newptr;
}




