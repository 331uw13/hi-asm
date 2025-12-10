#ifndef COMMON_UTILITIES_H
#define COMMON_UTILITIES_H

#include <stdbool.h>


#define ARRAY_LEN(a) (sizeof(a) / sizeof(*a))

void freeif(void* ptr);



bool is_literal_int32(const char* str);


#endif
