#include <string.h>
#include <stdint.h>

#include "token.h"

const char* get_token_name(enum token_type type) {
    switch(type) {
        case TOK_NONE: return "\033[90m<TOK_NONE>\033[0m";
        case TOK_EOL: return "TOK_EOL";
        case TOK_EOF: return "TOK_EOF";
        case TOK_COMMENT: return "TOK_COMMENT";
        case TOK_AT: return "TOK_AT";
        case TOK_VAR: return "TOK_VAR";
        case TOK_MOV: return "TOK_MOV";
        case TOK_ADD: return "TOK_ADD";
        case TOK_ARROW_L: return "TOK_ARROW_L";
        case TOK_COMMA: return "TOK_COMMA";
        case TOK_COLON: return "TOK_COLON";
        case TOK_DOT: return "TOK_DOT";
        case TOK_OPEN_BRACKET: return "TOK_OPEN_BRACKET";
        case TOK_CLOSE_BRACKET: return "TOK_CLOSE_BRACKET";
        case TOK_OPEN_SCOPE: return "TOK_OPEN_SCOPE";
        case TOK_CLOSE_SCOPE: return "TOK_CLOSE_SCOPE";
        case TOK_TYPE_I32: return "TOK_TYPE_I32";
        case TOK_TYPE_VOID: return "TOK_TYPE_VOID";
        case TOK_SYMBOL: return "TOK_SYMBOL";
        case TOK_FUNC: return "TOK_FUNC";
        case PTOK_NEW_VAR: return "PTOK_NEW_VAR";
        case PTOK_VAR: return "PTOK_VAR";
        case PTOK_LIT_I32: return "PTOK_LIT_I32";
        case PTOK_FUNC: return "PTOK_FUNC";
    }

    return "<Unknown token>";
}


void set_token_rawdata(struct token* tok, char* buf, size_t len) {
    if(len >= sizeof(tok->raw_data)) {
        return;
    }
    memset(tok->raw_data, 0, sizeof(tok->raw_data));
    memmove(tok->raw_data, buf, len);
    tok->raw_data_empty = false;
}

void zero_token(struct token* tok) {
    tok->type = 0;
    if(!tok->raw_data_empty) {
        memset(tok->raw_data, 0, sizeof(tok->raw_data));
        tok->raw_data_empty = true;
    }
}

void remove_empty_tokens(struct token_array* tokens) {
    for(int64_t i = 0; i < (int64_t)tokens->token_count; i++) {
        struct token* tok = &tokens->array[i];
        
        if(tok->type != TOK_NONE) {
            continue;
        }

        memmove(&tokens->array[i],
                &tokens->array[i + 1],
                (tokens->token_count - (i + 1)) * sizeof *tokens->array );

        tokens->token_count--;
        i--;
    }
}

