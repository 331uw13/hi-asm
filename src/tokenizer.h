#ifndef TOKENIZER_H
#define TOKENIZER_H

#include <stdbool.h>

#include "token.h"


bool tokenize(const char* input_file, struct token_array* tokens);
void free_token_array(struct token_array* tokens);



#endif
