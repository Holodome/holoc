#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "allocator.h"
#include "ast.h"
#include "ast_builder.h"
#include "bump_allocator.h"
#include "c_lang.h"
#include "darray.h"
#include "pp_lexer.h"
#include "preprocessor.h"
#include "str.h"
#include "file_storage.h"

#define STB_SPRINTF_IMPLEMENTATION
#include "stb_sprintf.h"

typedef struct {
    // print preprocessing tokens verbose, its on its own line
    bool ptv;
    // print preprocessing tokens verbose, as if they were on logical source
    // lines
    bool ptvf;
    // print preprocessing tokens, as if they were on logical source lines
    bool ptf;
    // print preprocessing tokens, each on its own line
    bool pt;
    // print c tokens, each on its own line
    bool tp;
    // print c tokens verbose, each on its own line
    bool tpv;
    // dump ast tree in tree format
    bool ast;
} program_settings;

static program_settings settings;

static void 
process_file(string filename) {
    allocator *a = get_system_allocator();

    file_storage fs = {0};
    fs.a = a;
    add_default_include_paths(&fs);

    bump_allocator ba = {0};
    preprocessor *pp = aalloc(a, sizeof(*pp));
    pp->a = &ba;
    pp->ea = a;
    pp->fs = &fs;

    token *toks = do_pp(pp, filename);
    for (token *tok = toks; tok; tok = tok->next) {
        char buffer[4096];
        fmt_token_verbose(tok, buffer, sizeof(buffer));
        /* printf("%s\n", buffer); */
    }
}

#if 0

static void
process_file(string filename) {
    allocator *a     = get_system_allocator();

    if (settings.ptv) {
        char lexer_buffer[4096];
        pp_lexer lexer         = {0};
        lexer.tok_buf          = lexer_buffer;
        lexer.tok_buf_capacity = sizeof(lexer_buffer);
        lexer.data             = file_contents;
        lexer.eof              = file_contents + strlen(file_contents);
        lexer.cursor           = lexer.data;
        while (pp_lexer_parse(&lexer)) {
            char token_buf[4096];
            fmt_pp_tok_verbose(&lexer.tok, token_buf, sizeof(token_buf));
            printf("%s\n", token_buf);
        }
        return;
    }

    if (settings.ptvf) {
        char lexer_buffer[4096];
        pp_lexer lexer         = {0};
        lexer.tok_buf          = lexer_buffer;
        lexer.tok_buf_capacity = sizeof(lexer_buffer);
        lexer.data             = file_contents;
        lexer.eof              = file_contents + strlen(file_contents);
        lexer.cursor           = lexer.data;
        while (pp_lexer_parse(&lexer)) {
            char token_buf[4096];
            fmt_pp_tok_verbose(&lexer.tok, token_buf, sizeof(token_buf));
            if (lexer.tok.at_line_start) {
                printf("\n");
            } else if (lexer.tok.has_whitespace) {
                printf(" ");
            }
            printf("%s", token_buf);
        }
        return;
    }

    if (settings.ptf) {
        char lexer_buffer[4096];
        pp_lexer lexer         = {0};
        lexer.tok_buf          = lexer_buffer;
        lexer.tok_buf_capacity = sizeof(lexer_buffer);
        lexer.data             = file_contents;
        lexer.eof              = file_contents + strlen(file_contents);
        lexer.cursor           = lexer.data;
        while (pp_lexer_parse(&lexer)) {
            char token_buf[4096];
            fmt_pp_tok(&lexer.tok, token_buf, sizeof(token_buf));
            if (lexer.tok.at_line_start) {
                printf("\n");
            } else if (lexer.tok.has_whitespace) {
                printf(" ");
            }
            printf("%s", token_buf);
        }
        return;
    }

    if (settings.pt) {
        char lexer_buffer[4096];
        pp_lexer lexer         = {0};
        lexer.tok_buf          = lexer_buffer;
        lexer.tok_buf_capacity = sizeof(lexer_buffer);
        lexer.data             = file_contents;
        lexer.eof              = file_contents + strlen(file_contents);
        lexer.cursor           = lexer.data;
        while (pp_lexer_parse(&lexer)) {
            char token_buf[4096];
            fmt_pp_tok(&lexer.tok, token_buf, sizeof(token_buf));
            printf("%s\n", token_buf);
        }
        return;
    }

    bump_allocator pp_bump = {0};
    preprocessor *pp       = bump_alloc(&pp_bump, sizeof(preprocessor));
    pp->lex                = &lexer;
    pp->a                  = &pp_bump;
    pp->ea                 = a;
    token *toks            = do_pp(pp);

    if (settings.tp) {
        for (token *tok = toks; tok; tok = tok->next) {
            char buffer[4096];
            fmt_token(tok, buffer, sizeof(buffer));
            printf("%s\n", buffer);
        }
        return;
    }

    if (settings.tpv) {
        for (token *tok = toks; tok; tok = tok->next) {
            char buffer[4096];
            fmt_token_verbose(tok, buffer, sizeof(buffer));
            printf("%s\n", buffer);
        }
        return;
    }

    bump_allocator ast_b_bump = {0};
    ast_builder *ast_b        = bump_alloc(&ast_b_bump, sizeof(ast_builder));
    ast_b->a                  = &ast_b_bump;
    ast_b->ea                 = a;
    ast_b->toks               = toks;
    ast_b->tok                = toks;

    if (settings.ast) {
        for (;;) {
            ast *toplevel = build_toplevel_ast(ast_b);
            if (!toplevel) {
                break;
            }
            char buffer[16384];
            fmt_ast_verbose(toplevel, buffer, sizeof(buffer));
            printf("%s", buffer);
        }
        return;
    }
}

#endif

int
main(int argc, char **argv) {
    allocator *a = get_system_allocator();

    string *filenames = 0;
    for (int arg_idx = 1; arg_idx < argc; arg_idx++) {
        if (strcmp(argv[arg_idx], "--ptv") == 0) {
            settings.ptv = true;
        } else if (strcmp(argv[arg_idx], "--ptvf") == 0) {
            settings.ptvf = true;
        } else if (strcmp(argv[arg_idx], "--ptf") == 0) {
            settings.ptf = true;
        } else if (strcmp(argv[arg_idx], "--pt") == 0) {
            settings.pt = true;
        } else if (strcmp(argv[arg_idx], "--tp") == 0) {
            settings.tp = true;
        } else if (strcmp(argv[arg_idx], "--tpv") == 0) {
            settings.tpv = true;
        } else if (strcmp(argv[arg_idx], "--ast") == 0) {
            settings.ast = true;
        } else {
            da_push(filenames, stringz(argv[arg_idx]), a);
        }
    }

    /* da_push(filenames, WRAP_Z("examples/example.c"), a); */
    /* da_push(filenames, WRAP_Z("examples/pp/a.h"), a); */

    if (!da_size(filenames)) {
        fprintf(stderr, "error: no input files");
        return 1;
    }

    for (uint32_t filename_idx = 0; filename_idx < da_size(filenames);
         filename_idx++) {
        string filename = filenames[filename_idx];
        printf("Filename %u: %.*s\n", filename_idx, filename.len,
               filename.data);

        process_file(filename);
    }

    return 0;
}
