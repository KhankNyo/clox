

#include <time.h>

#include "include/common.h"
#include "include/object.h"
#include "include/value.h"




Value_t Native_Clock(int argc, Value_t* argv)
{
    (void)argc, (void)argv;
    return NUMBER_VAL((double)clock() / CLOCKS_PER_SEC);
}



