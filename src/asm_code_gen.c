#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <stdarg.h>
#include <limits.h>

#include "asm_code_gen.h"
#include "hashmap.h"


static struct {

    int out_fd;

    bool to_stdout;
}
gst; // Global state.




void cdprintf(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);

    char buffer[1024] = { 0 };
    ssize_t len = vsnprintf(buffer, sizeof(buffer)-1,
            fmt, args);

    if(len < 0) {
        fprintf(stderr, "%s: vsnprintf: %s\n",
                __func__, strerror(errno));
        goto error;
    }

    write(gst.to_stdout
            ? STDOUT_FILENO : gst.out_fd, buffer, len);

error:
    va_end(args);
}




void gen_base() { 
    cdprintf("section .text\n"
            "   global _start\n");

}


#define RBPOFF_NOTFOUND INT_MAX

int get_var_rbp_off(struct hashmap_t* map, const char* str_key) {
    struct hashmap_pair_t* pair = hashmap_get(map, strtokey(str_key));
    if(!pair) {
        return RBPOFF_NOTFOUND;
    }

    return *(int*)pair->ptr;
}

void asm_code_gen(struct token_array* tokens, const char* out_file) {
    gst.to_stdout = (strcmp(out_file, "-") == 0);
    
    gst.out_fd = -1;
    if(!gst.to_stdout) {
        gst.out_fd = open(out_file, 
                O_WRONLY | O_APPEND | O_CREAT, 
                S_IRUSR | S_IWUSR |
                S_IRGRP |
                S_IROTH);
    
        if(gst.out_fd < 0) {
            fprintf(stderr, "%s\n", strerror(errno));
            return;
        }
    }

    gen_base();


    struct curr_scope {
        int rbp_off;

        struct hashmap_t offset_map; // Variable offsets.
    }
    scope = {
        .rbp_off = 0,
        .offset_map = create_hashmap(32) // <- Initial size.
    };


    /*
    int A = 6200;
    int B = 1200;
    int C = 80;
    int D = 2;

    hashmap_add(&scope.offset_map, strtokey("Hello"), &A);
    hashmap_add(&scope.offset_map, strtokey("Hello2"), &D);
    hashmap_add(&scope.offset_map, strtokey("Hello5"), &C);

    struct hashmap_pair_t* pair = hashmap_get(&scope.offset_map, strtokey("Hello"));
    printf("pair = %p\n", pair);


    free_hashmap(&scope.offset_map);
    return;
    */


    struct token* tok = &tokens->array[0];
    while(tok->type != TOK_EOF) {

        switch(tok->type) {

            case PTOK_FUNC:
                cdprintf("\n%s:\n", tok->data.func.label);
                break;

            case TOK_OPEN_SCOPE:
                cdprintf(
                        "   push rbp\n"
                        "   mov rbp, rsp\n");
                break;

            case TOK_CLOSE_SCOPE:
                cdprintf(
                        "   pop rbp\n"
                        "   ret\n\n");
                hashmap_clear(&scope.offset_map);
                scope.rbp_off = 0;
                break;

            case PTOK_NEW_VAR:
                {
                    // Save variable's rbp offset.

                    // TODO: Cleanup later.

                    if(scope.rbp_off == 0) {
                        switch(tok->data.var.type) {
                            case TYPE_I32:
                                scope.rbp_off += 4;
                                break;

                            // ... more types will be added in the future.
                        }
                    }

                    hashmap_add_new(&scope.offset_map,
                            strtokey(tok->data.var.name), &scope.rbp_off, sizeof(scope.rbp_off));

                    switch(tok->data.var.type) {
                        case TYPE_I32:
                            scope.rbp_off += 4;
                            break;

                        // ... more types will be added in the future.
                    }
                }
                break;

            case TOK_MOV:
                {
                    struct token* lhs_tok = tok + 1;
                    struct token* rhs_tok = tok + 2;

                    int rbp_off = get_var_rbp_off
                        (&scope.offset_map, lhs_tok->data.var.name);

                    if(rbp_off == RBPOFF_NOTFOUND) {
                        break;
                    }

                    char rhs[32] = { 0 };

                    if(rhs_tok->type == PTOK_LIT_I32) {
                        snprintf(rhs, sizeof(rhs)-1,
                                "%i", rhs_tok->data.lit_i32.value);
                    }

                    cdprintf(
                            "   mov DWORD PTR [rbp-%i], %s\n",
                            rbp_off, rhs);
                }
                break;



        }



        tok++;
    }

    cdprintf(
            "_start:\n"
            "   call entry\n"
            "   mov rax, 60\n"
            "   mov rdi, 0\n"
            "   syscall\n\n");




    free_hashmap(&scope.offset_map);

    if(gst.out_fd > -1) {
        close(gst.out_fd);
    }
}



