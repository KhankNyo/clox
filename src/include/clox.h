#ifndef _CLOX_H_
#define _CLOX_H_



#include <stdio.h>

#include "memory.h"
#include "common.h"
#include "vm.h"


#define CLOX_DEFAULT_ALLOC_MEMSIZE (5 * 1024 * 1024)
#define CLOX_FLAG_MEM ((unsigned)1 << 0)
#define CLOX_FLAG_JIT ((unsigned)1 << 1)

typedef enum CloxUnixErr_t
{
    CLOX_NOERR = 0,
    /* 
    * the book said this is exit code for invalid usage, 
    * but from the stack exchange website,
    * it's "Machine is not on the network"??
    */
    CLOX_UNIX_ENONET = 64,
    CLOX_UNIX_ENOPKG = 65,
    CLOX_UNIX_ECOMM = 70,
    CLOX_UNIX_EBADMSG = 74,
} CloxUnixErr_t;

typedef struct Clox_t
{
    VM_t vm;
    Allocator_t alloc;
    CloxUnixErr_t err;
    unsigned flags;
} Clox_t;




/*
*   initializes the Clox struct
*/
void Clox_Init(Clox_t* clox, size_t allocator_capacity);

/*
*   frees allocated memory in the Clox struct
*/
void Clox_Free(Clox_t* clox);





/* 
*   compile and run the given file, or return with an error
*/
void Clox_RunFile(Clox_t* clox, const char* file_path);

/*
*   runs clox in command line mode
*/
void Clox_Repl(Clox_t* clox);


/*
*   prints Clox usage
*/
void Clox_PrintUsage(FILE* fout, const char* program_path);




#endif /* _CLOX_H_ */

