

#include <stdio.h>

#include "include/common.h"
#include "include/memory.h"
#include "include/scanner.h"
#include "include/compiler.h"





void Compile(const char* src)
{
    Scanner_t scanner;
    Scanner_Init(&scanner, src);


    line_t currline = 0;
    while (true)
    {
        Token_t tok = Scanner_ScanToken(&scanner);
        if (currline != tok.line)
        {
            printf("%4zu ", (size_t)tok.line);
            currline = tok.line;
        }
        else
        {
            printf("   | ");
        }
        printf("%2d '%.*s'\n", tok.type, (int)tok.len, tok.start);

        if (tok.type == TOKEN_EOF)
            break;
    }
}


