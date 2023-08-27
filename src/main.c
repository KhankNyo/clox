

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "include/clox.h"


int main(int argc, char** argv)
{
	Clox_t clox;
	Clox_Init(&clox, 5 * 1024);
	
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

