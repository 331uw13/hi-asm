#ifndef TOKEN_H
#define TOKEN_H

#include <stddef.h>
#include <stdint.h>


enum token_type {
    TOK_NONE,
    TOK_EOL,
    TOK_EOF,
    TOK_COMMENT,
    TOK_AT,
    TOK_VAR,
    TOK_MOV,
    TOK_ADD,
    TOK_ARROW_L,
    TOK_COMMA,
    TOK_COLON,
    TOK_DOT,
    TOK_OPEN_BRACKET,
    TOK_CLOSE_BRACKET,
    TOK_OPEN_SCOPE,
    TOK_CLOSE_SCOPE,
    TOK_FUNC,
    TOK_SYMBOL,
    

    TOK_TYPE_VOID,
    TOK_TYPE_I32,
        
    // Assigned by the parser:
    PTOK_NEW_VAR,
    PTOK_VAR,
    PTOK_LIT_I32,

    PTOK_FUNC,
    PTOK_FUNC_CALL,


    // Special:
    TOK__ANY_TYPE__,
};



enum var_type {
    TYPE_VOID,
    TYPE_I32
};


struct token {
    enum token_type type;

    char            raw_data[64];
    bool            raw_data_empty;

    union {
        
        struct {
            int value;
        }
        lit_i32; // Literal 32bit int.
   
        struct {
            enum var_type type;
            char          name[64];
            uint32_t      name_len;
        }
        var;

        struct {
            enum var_type ret_type;
            char          label[64];
            uint32_t      label_len;
        }
        func;


    }
    data;

    size_t line;
    int    column;
};

struct token_array {
    struct token* array;
    size_t        array_num_alloc; // Number of tokens allocated.
    size_t        token_count;

    char*         file_path;
};

void        zero_token(struct token* tok);
void        set_token_data(struct token* tok, char* buf, size_t len);
const char* get_token_name(enum token_type type);

void        remove_empty_tokens(struct token_array* tokens);



#endif
