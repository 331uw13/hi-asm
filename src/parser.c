#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "parser.h"
#include "error.h"
#include "common.h"


bool is_correct_order
(
    struct token_array* tokens,
    struct token* curr_tok, 
    enum token_type* token_types,
    size_t size
);




struct token* parse_var(struct token_array* tokens, struct token* curr_tok) {
    enum token_type order[] = {
        TOK_VAR, TOK_AT, TOK_SYMBOL, TOK_COMMA, TOK__ANY_TYPE__
    };

    if(!is_correct_order(tokens, curr_tok, order, ARRAY_LEN(order))) {
        return NULL;
    }


    struct token* name_tok = curr_tok + 2;
    struct token* type_tok = curr_tok + 4;

    switch(type_tok->type) {
        case TOK_TYPE_VOID:
            curr_tok->data.var.type = TYPE_VOID;
            break;

        case TOK_TYPE_I32:
            curr_tok->data.var.type = TYPE_I32;
            break;
    }

    memset(curr_tok->data.var.name, 
            0, sizeof(curr_tok->data.var.name));

    curr_tok->data.var.name_len = strlen(name_tok->raw_data);
    memcpy(curr_tok->data.var.name,
            name_tok->raw_data,
            curr_tok->data.var.name_len);


    for(size_t i = 1; i < ARRAY_LEN(order); i++) {
        zero_token(curr_tok + i);
    }

    curr_tok->type = PTOK_NEW_VAR;

    return curr_tok;
}

struct token* parse_atvar(struct token_array* tokens, struct token* curr_tok) {
    enum token_type order[] = {
        TOK_AT, TOK_SYMBOL, TOK_ARROW_L
    };

    if(!is_correct_order(tokens, curr_tok, order, ARRAY_LEN(order))) {
        return NULL;
    }

    struct token* name_tok = curr_tok + 1;
    


    memset(curr_tok->data.var.name, 
            0, sizeof(curr_tok->data.var.name));

    curr_tok->data.var.name_len = strlen(name_tok->raw_data);
    memcpy(curr_tok->data.var.name,
            name_tok->raw_data,
            curr_tok->data.var.name_len);

    for(size_t i = 1; i < ARRAY_LEN(order); i++) {
        zero_token(curr_tok + i);
    }

    
    curr_tok->type = PTOK_VAR;

    return curr_tok;
}

struct token* parse_func(struct token_array* tokens, struct token* curr_tok) {

    // TODO: Allow order to notice any type. (TOK__ANY_TYPE__ ?)
    enum token_type order[] = {
        TOK_FUNC, TOK_COLON, TOK__ANY_TYPE__, TOK_DOT, TOK_SYMBOL
    };

    if(!is_correct_order(tokens, curr_tok, order, ARRAY_LEN(order))) {
        return NULL;
    }



    struct token* type_tok = curr_tok + 2;
    struct token* label_tok = curr_tok + 4;

    curr_tok->type = PTOK_FUNC;
    

    switch(type_tok->type) {
        case TOK_TYPE_VOID:
            curr_tok->data.func.ret_type = TYPE_VOID;
            break;

        case TOK_TYPE_I32:
            curr_tok->data.func.ret_type = TYPE_I32;
            break;

    }


    memset(curr_tok->data.func.label,
            0, sizeof(curr_tok->data.func.label));
    curr_tok->data.func.label_len = strlen(label_tok->raw_data);
    memcpy(curr_tok->data.func.label,
            label_tok->raw_data,
            curr_tok->data.func.label_len);


    for(size_t i = 1; i < ARRAY_LEN(order); i++) {
        zero_token(curr_tok + i);
    }

    return curr_tok;
}

struct token* parse_sym(struct token_array* tokens, struct token* curr_tok) {

    if(curr_tok->raw_data_empty) {
        errmsg(tokens->file_path, curr_tok->line, curr_tok->column,
                "Symbol token doesnt have data.");
        return NULL;
    }


    if(is_literal_int32(curr_tok->raw_data)) {
        curr_tok->type = PTOK_LIT_I32;
        curr_tok->data.lit_i32.value = atoi(curr_tok->raw_data);
    }


        
    return curr_tok;
}


bool parse_tokens(struct token_array* tokens) {


    struct token* curr_tok = &tokens->array[0];
    while(curr_tok != NULL && curr_tok->type != TOK_EOF) {

        switch(curr_tok->type) {
       

            case TOK_AT:
                curr_tok = parse_atvar(tokens, curr_tok);
                break;

            case TOK_VAR:
                curr_tok = parse_var(tokens, curr_tok);
                break;

            case TOK_SYMBOL:
                curr_tok = parse_sym(tokens, curr_tok);
                break;
        
            case TOK_FUNC:
                curr_tok = parse_func(tokens, curr_tok);
                break;
        }

        if(!curr_tok) {
            break;
        }
        curr_tok++;
    }


    return (curr_tok != NULL);
}


bool is_correct_order
(
    struct token_array* tokens, 
    struct token* curr_tok,
    enum token_type* token_types, 
    size_t size
){
    size_t index = 0;
    while(curr_tok != NULL && curr_tok->type != TOK_EOF) {
        
        enum token_type expect = token_types[index];

        if(expect == TOK__ANY_TYPE__) {
            if(curr_tok->type != TOK_TYPE_VOID
            && curr_tok->type != TOK_TYPE_I32) {
                errmsg(tokens->file_path, curr_tok->line, curr_tok->column,
                        "Expected TYPE, but found \"%s\"", 
                        curr_tok->raw_data);
                return false;
            }
        }
        else 
        if(curr_tok->type != expect) {
            errmsg(tokens->file_path, curr_tok->line, curr_tok->column,
                    "Expected %s, but found \"%s\"", 
                    get_token_name(token_types[index]),
                    curr_tok->raw_data);
            return false;
        }

        curr_tok++;
        index++;
        if(index >= size) {
            break;
        }
    }

    return true;
}



