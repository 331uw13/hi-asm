#ifndef ERRORS_H
#define ERRORS_H

#include <stddef.h>


void errmsg
(
    const char* file,
    size_t line,
    int column,
    const char* fmt,
    ...
);

void print_memalloc_error_msg
(
    const char* func_name,
    const char* func_file,
    const char* cause
);

#define PRINT_MEMERROR(cause)\
    print_memalloc_error_msg(__func__, __FILE__, cause)



#endif
