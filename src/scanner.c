

#include <stdio.h>

#include "include/common.h"
#include "include/memory.h"
#include "include/scanner.h"















void Scanner_Init(Scanner_t* scanner, const char* src, size_t src_size)
{
    scanner->start = src;
    scanner->curr = src;
    scanner->line = 1;
}
