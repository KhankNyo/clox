

#include "include/clox.h"


int main(int argc, char** argv)
{
	Clox_t clox;
    size_t memsize = CLOX_DEFAULT_ALLOC_MEMSIZE;
    if (argc > 2)
    {
        int i = 0;
        while (i < argc - 1)
        {
            i += 1;
        }
    }
	Clox_Init(&clox, memsize);
	
	if (1 == argc)
	{
		Clox_Repl(&clox);
	}
	else if (2 == argc)
	{
		Clox_RunFile(&clox, argv[1]);
	}
	else
	{
		Clox_PrintUsage(stderr, argv[0]);
		exit(CLOX_UNIX_ENONET);	/* unix exit code for invalid usage */
	}

	if (clox.err != CLOX_NOERR)
		exit(clox.err);
	Clox_Free(&clox);
	return 0;
}







