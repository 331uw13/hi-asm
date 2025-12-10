#include <stdio.h>

#include "tokenizer.h"
#include "parser.h"
#include "asm_code_gen.h"



void print_help(char** argv) {
    printf(
            "%s [input file] [output file]\n"
            "\n"
            "'-' as output file will write results to stdout.\n"
            ,argv[0]);
}

int main(int argc, char** argv) {
    int exit_code = 0;
    
    if(argc != 3) {
        print_help(argv);
        exit_code = 1;
        goto out;
    }


    const char* input_file = argv[1];
    const char* output_file = argv[2];

    struct token_array tokens;
    if(tokenize(input_file, &tokens)) {
        exit_code = 1;
        goto out;
    }
   
    if(!parse_tokens(&tokens)) {
        exit_code = 1;
        goto free_and_out;
    }
 
    // Some cleanup.
    remove_empty_tokens(&tokens);

    
    for(size_t i = 0; i < tokens.token_count; i++) {
        struct token* tok = &tokens.array[i];
        
        if(tok->type == TOK_EOL) {
            printf("\033[2;4;95m<TOK_EOL>\033[0m\n");
            continue;
        }

        printf("%s", get_token_name(tok->type));
        if(!tok->raw_data_empty) {
            printf(" - \033[34m'%s'\033[0m", tok->raw_data);
        }
        printf("\n");
    }

    printf("\033[2;90m--- end of tokens --- \033[0m\n");
    
    asm_code_gen(&tokens, output_file);

free_and_out:
    free_token_array(&tokens);
out:
    return exit_code;
}
