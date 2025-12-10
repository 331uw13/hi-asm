#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#include "tokenizer.h"
#include "fileio.h"
#include "common.h"
#include "error.h"


// Token characters dont require space to be in between them.
// For example: "600, i32" and "600,i32"  are bot valid.
static const char TOKEN_CHAR[] = {
    '{', '}', '(', ')', ':', ',', '.', '@'
};


struct token_map_elem {
    const enum token_type type;
    const char*           type_str;
};

static const struct token_map_elem TOKEN_MAP[] = {
    { TOK_EOL, "__EOL__" },
    { TOK_EOF, "__EOF__" },
    { TOK_COMMENT, "//" },
    { TOK_FUNC, "func" },
    { TOK_MOV, "mov" },
    { TOK_ADD, "add" },
    { TOK_VAR, "var" },
    { TOK_ARROW_L, "<-" },
    { TOK_COMMA, "," },
    { TOK_COLON, ":" },
    { TOK_DOT, "." },
    { TOK_OPEN_BRACKET, "(" },
    { TOK_CLOSE_BRACKET, ")" },
    { TOK_OPEN_SCOPE, "{" },
    { TOK_CLOSE_SCOPE, "}" },
    { TOK_AT, "@" },
    { TOK_TYPE_VOID, "void" },
    { TOK_TYPE_I32, "i32" },
};


// Prepare to add new element to token array.
// Allocate more memory if needed.
static bool token_array_prep_add(struct token_array* tokens, int num_add) {

    if(tokens->token_count + num_add < tokens->array_num_alloc) {
        return true;
    }

    num_add += 100;

    const size_t new_memsize = sizeof *tokens->array * (tokens->array_num_alloc + num_add);
    struct token* tmp_ptr = realloc(tokens->array, new_memsize);
    if(!tmp_ptr) {
        PRINT_MEMERROR("realloc");
        return false;
    }

    tokens->array = tmp_ptr;
    tokens->array_num_alloc += num_add;
    return true;
}



static bool add_token(struct token_array* tokens, size_t line, size_t column, char* str) {
    if(!token_array_prep_add(tokens, 1)) {
        return false;
    }
    
    struct token* curr_tok = &tokens->array[tokens->token_count++];

    curr_tok->type = TOK_SYMBOL;
    curr_tok->raw_data_empty = true;
    curr_tok->line = line;
    curr_tok->column = column;

    for(size_t i = 0; i < ARRAY_LEN(TOKEN_MAP); i++) {
        const struct token_map_elem* elem = &TOKEN_MAP[i];
        if(strcmp(str, elem->type_str) == 0) {
            curr_tok->type = elem->type;
            break;
        }
    }
   
    size_t str_len = strlen(str);
    if(str_len >= sizeof(curr_tok->raw_data)-1) {
        errmsg(tokens->file_path, line, column, "Too long symbol \"%s\"", str);
        return false;
    }

    memset(curr_tok->raw_data, 0, sizeof(curr_tok->raw_data));
    memcpy(curr_tok->raw_data, str, str_len);

    if(curr_tok->type == TOK_SYMBOL) {
        curr_tok->raw_data_empty = false;
    }

    return true;
}


bool tokenize(const char* input_file, struct token_array* tokens) {
    bool result = false;
    char* input_data = NULL;
    size_t input_size = 0;

    tokens->array = NULL;
    tokens->array_num_alloc = 0;
    tokens->token_count = 0;

    if(!map_file(input_file, PROT_READ, &input_data, &input_size)) {
        goto out;
    }


    const size_t input_file_len = strlen(input_file);
    tokens->file_path = calloc(input_file_len+1, sizeof *tokens->file_path);
    memcpy(tokens->file_path,
            input_file,
            input_file_len);

    char* ch = &input_data[0];
    size_t num_lines = 1;
    size_t column    = 0;

    char buffer[64] = { 0 };
    size_t buf_idx = 0;

    while(ch < input_data + input_size) {
        char prev_ch = *ch;
        if(ch > input_data) {
            prev_ch = *(ch - 1);
        }

        if(*ch == '\n') {

            if(prev_ch != '\n') {
                if(buf_idx > 0) {
                    if(!add_token(tokens, num_lines, column, buffer)) {
                        goto error;
                    }

                    memset(buffer, 0, sizeof(buffer));
                    buf_idx = 0;
                }

                add_token(tokens, num_lines, column, "__EOL__");
            }

            memset(buffer, 0, sizeof(buffer));
            num_lines++;
            column = 0;
            buf_idx = 0;

            ch++;
            continue;
        }
        

        if(*ch == ' ') {
            if(buf_idx == 0) {
                goto skip;
            }
            if(!add_token(tokens, num_lines, column, buffer)) {
                goto error;
            }

            memset(buffer, 0, sizeof(buffer));
            buf_idx = 0;
        
        }
        else {

            // Test for token characters first.
            for(size_t i = 0; i < ARRAY_LEN(TOKEN_CHAR); i++) {
                if(*ch == TOKEN_CHAR[i]) {

                    if(buf_idx > 0) {
                        if(!add_token(tokens, num_lines, column, buffer)) {
                            goto error;
                        }
                        memset(buffer, 0, sizeof(buffer));
                        buf_idx = 0;
                    }   

                    char tmp[2] = { *ch, 0 };
                    if(add_token(tokens, num_lines, column, tmp)) {
                        goto skip;
                    }
                    else {
                        goto error;
                    }
                }
            }

            if(buf_idx >= sizeof(buffer)) {
                errmsg(input_file, num_lines, column, "Too long token \"%s\"", buffer);
                free_token_array(tokens);
                goto error;
            }

            buffer[buf_idx++] = *ch;
        }
skip:
        column++;
        ch++;
    }

     
    add_token(tokens, num_lines, 0, "__EOF__");

error:
    munmap(input_data, input_size);
out:
    return result;
}

void free_token_array(struct token_array* tokens) {
    freeif(tokens->array);
    freeif(tokens->file_path);
}

