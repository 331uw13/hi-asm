#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#include <unistd.h>

#include "error.h"



void errmsg
(
    const char* file,
    size_t line,
    int column,
    const char* fmt,
    ...
){
    va_list args;
    va_start(args, fmt);

    char buffer[512] = { 0 };

    ssize_t buflen = snprintf(buffer, sizeof(buffer)-1, 
            "\033[31mERROR \"%s\" (line %li, column %i) \033[90m<-\033[0m ",
            file,
            line,
            column);

    if(buflen < 0) {
        fprintf(stderr, "%s: %s\n", __func__, strerror(errno));
        goto skip;
    }

    buflen += vsnprintf(buffer + buflen, sizeof(buffer)-1,
            fmt, args);

    write(STDOUT_FILENO, buffer, buflen);
    write(STDOUT_FILENO, "\n", 1);

skip:
    va_end(args);
}



void print_memalloc_error_msg
(
    const char* func_name,
    const char* func_file,
    const char* cause
){
    fprintf(stderr, "(INTERNAL MEMORY ERROR): From(%s, %s()) %s: %s\n",
            func_file, func_name, cause, strerror(errno));
}
