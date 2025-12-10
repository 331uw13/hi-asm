#include <stdlib.h>
#include <string.h>

#include "common.h"

void freeif(void* ptr) {
    if(ptr) {
        free(ptr);
        ptr = NULL;
    }
}



bool is_literal_int32(const char* str) {
    const size_t len = strlen(str);
    for(size_t i = 0; i < len; i++) {
        if((str[i] < '0') || (str[i] > '9')) {
            return false;
        }
    }
    return true;
}

