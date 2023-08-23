

#include <stdio.h>

#include "include/common.h"
#include "include/memory.h"
#include "include/scanner.h"
#include "include/compiler.h"





void Compile(const char* src, size_t src_size)
{
    Scanner_t scanner;
    Scanner_Init(scanner, src, src_size);


    size_t currline = 0;
    while (true)
    {
        Token_t tok = Scanner_ScanToken(&scanner);
        if (currline != tok.line)
        {
            printf("%4zu ", tok.line);
            line = tok.line;
        }
        else
        {
            printf("   | ");
        }
        printf("%2d '%.*s'\n", tok.type, tok.len, tok.start);

        if (tok.type == TOKEN_EOF)
            break;
    }
}


