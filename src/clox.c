

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "include/memory.h"
#include "include/clox.h"
#include "include/vm.h"





static char* load_file_content(Allocator_t* alloc, const char* file_path, size_t* src_size);
static void unload_file_content(Allocator_t* alloc, char* file_content);


void Clox_Init(Clox_t *clox, size_t allocator_capacity)
{
    Allocator_Init(&clox->alloc, allocator_capacity);
    VM_Init(&clox->vm, &clox->alloc);
    clox->err = CLOX_NOERR;
}


void Clox_Free(Clox_t* clox)
{
    VM_Free(&clox->vm);
    Allocator_KillEmAll(&clox->alloc);
}






void Clox_RunFile(Clox_t* clox, const char* file_path)
{
    size_t src_size = 0;
    char* src = load_file_content(&clox->alloc, file_path, &src_size);
    InterpretResult_t ret = VM_Interpret(&clox->vm, src);
    unload_file_content(&clox->alloc, src);

    if (ret == INTERPRET_COMPILE_ERROR)
        clox->err = CLOX_UNIX_ENOPKG;
    else if (ret == INTERPRET_RUNTIME_ERROR)
        clox->err = CLOX_UNIX_ECOMM;
}


void Clox_Repl(Clox_t* clox)
{
    char line[1024] = { 0 };
    while (true)
    {
        printf("> ");
        if (!fgets(line, sizeof line, stdin))
        {
            printf("\n");
            break;
        }
        else if (strncmp(line, "exit\n", sizeof("exit")) == 0)
        {
            break;
        }
        else if (strncmp(line, "reset\n", sizeof("reset")) == 0)
        {
            VM_Reset(&clox->vm);
            continue;
        }


        VM_Interpret(&clox->vm, line);
    }
}



void Clox_PrintUsage(FILE* fout, const char* program_path)
{
    fprintf(fout, "Usage: %s [path]\n", program_path);
}


















static char* load_file_content(Allocator_t* alloc, const char* file_path, size_t* src_size)
{
#ifdef _MSC_VER
    FILE* f = NULL;
    (void)fopen_s(&f, file_path, "rb");
#else
    FILE* f = fopen(file_path, "rb");
#endif /* _MSC_VER */
    if (NULL == f)
    {
        fprintf(stderr, "Could not open file '%s'.\n", file_path);
        exit(CLOX_UNIX_EBADMSG);
    }

    /* lovely C way to find file's size */
    fseek(f, 0, SEEK_END);
    const size_t file_size = ftell(f);
    fseek(f, 0, SEEK_SET);


    char* file_content = Allocator_Alloc(alloc, file_size + 1);
    const size_t read_size = fread(file_content, 1, file_size, f);
    fclose(f);
    if (read_size != file_size)
    {
        fprintf(stderr, 
            "Could not read file properly: read %zu bytes, expected to read %zu bytes\n",
            read_size, file_size
        );
        exit(CLOX_UNIX_EBADMSG);
    }
    file_content[file_size] = '\0';
    *src_size = file_size;
    return file_content;
}



static void unload_file_content(Allocator_t* alloc, char* file_content)
{
    Allocator_Free(alloc, file_content);
}
