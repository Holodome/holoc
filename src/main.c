#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ast.h"
#include "c_lang.h"
#include "darray.h"
#include "error_reporter.h"
#include "file_storage.h"
#include "parser.h"
#include "pp_lexer.h"
#include "preprocessor.h"
#include "str.h"
#include "token_iter.h"

typedef enum {
    // print preprocessing tokens verbose, its on its own line
    M_PTV,
    // print preprocessing tokens, as if they were on logical source lines
    M_PTF,
    // print preprocessing tokens, each on its own line
    M_PT,
    // print c tokens, each on its own line
    M_TP,
    // print c tokens verbose, each on its own line
    M_TPV,
    // print c tokens, as if they were on logical source lines
    M_TPF,
    // print ast tree
    M_AST,
} mode;

typedef struct {
    string *filenames;      // da
    string *include_paths;  // da
    mode mode;
} program_settings;

static program_settings settings;

static void
parse_clargs(uint32_t argc, char **argv) {
    string *filenames = NULL;
    for (uint32_t arg_idx = 1; arg_idx < argc; arg_idx++) {
        char *option = argv[arg_idx];
        if (strcmp(option, "--ptv") == 0) {
            settings.mode = M_PTV;
        } else if (strcmp(option, "--ptf") == 0) {
            settings.mode = M_PTF;
        } else if (strcmp(option, "--pt") == 0) {
            settings.mode = M_PT;
        } else if (strcmp(option, "--tp") == 0) {
            settings.mode = M_TP;
        } else if (strcmp(option, "--tpv") == 0) {
            settings.mode = M_TPV;
        } else if (strcmp(option, "--tpf") == 0) {
            settings.mode = M_TPF;
        } else if (strcmp(option, "--ast") == 0) {
            settings.mode = M_AST;
        } else if (strncmp(option, "-I", 2) == 0) {
            char *path        = option + 2;
            uint32_t path_len = strlen(path);
            char *new_path    = malloc(path_len + 1);
            memcpy(new_path, path, path_len + 1);
            da_push(settings.include_paths, ((string){new_path, path_len}));
        } else {
            da_push(filenames, ((string){option, strlen(option)}));
        }
    }
    settings.filenames = filenames;
}

static void
mptv(string filename) {
    fs_add_include_paths(settings.include_paths, da_size(settings.include_paths));

    file *f = fs_get_file(filename, 0);

    char token_buf[4096];
    pp_lexer *lex = calloc(1, sizeof(pp_lexer));
    pp_lexer_init(lex, f->contents.data, STRING_END(f->contents));
    pp_token tok = {0};
    uint32_t _;
    while (pp_lexer_parse(lex, &tok, token_buf, sizeof(token_buf), &_)) {
        char buf[4096];
        fmt_pp_tok_verbose(buf, sizeof(buf), &tok);
        printf("%s\n", buf);
        memset(&tok, 0, sizeof(tok));
    }
}

static void
mptf(string filename) {
    fs_add_include_paths(settings.include_paths, da_size(settings.include_paths));

    file *f = fs_get_file(filename, 0);

    char token_buf[4096];
    pp_lexer *lex = calloc(1, sizeof(pp_lexer));
    pp_lexer_init(lex, f->contents.data, STRING_END(f->contents));
    pp_token tok = {0};
    uint32_t _;
    while (pp_lexer_parse(lex, &tok, token_buf, sizeof(token_buf), &_)) {
        char buf[4096];
        fmt_pp_tok(buf, sizeof(buf), &tok);
        if (tok.at_line_start) {
            printf("\n");
        } else if (tok.has_whitespace) {
            printf(" ");
        }
        printf("%s", buf);
        memset(&tok, 0, sizeof(tok));
    }
    printf("\n");
}

static void
mpt(string filename) {
    fs_add_include_paths(settings.include_paths, da_size(settings.include_paths));

    file *f = fs_get_file(filename, 0);

    char token_buf[4096];
    pp_lexer *lex = calloc(1, sizeof(pp_lexer));
    pp_lexer_init(lex, f->contents.data, STRING_END(f->contents));
    pp_token tok = {0};
    uint32_t _;
    while (pp_lexer_parse(lex, &tok, token_buf, sizeof(token_buf), &_)) {
        char buf[4096];
        fmt_pp_tok(buf, sizeof(buf), &tok);
        printf("%s\n", buf);
        memset(&tok, 0, sizeof(tok));
    }
}

static void
mtp(string filename) {
    fs_add_include_paths(settings.include_paths, da_size(settings.include_paths));

    preprocessor *pp = calloc(1, sizeof(preprocessor));
    pp_init(pp, filename);

    char buf[4096];
    uint32_t _;
    token tok = {0};
    while (pp_parse(pp, &tok, buf, sizeof(buf), &_)) {
        char fmt_buf[4096];
        fmt_token(fmt_buf, sizeof(fmt_buf), &tok);
        printf("%s\n", buf);
    }
}

static void
mtpv(string filename) {
    fs_add_include_paths(settings.include_paths, da_size(settings.include_paths));

    preprocessor *pp = calloc(1, sizeof(preprocessor));
    pp_init(pp, filename);

    char buf[4096];
    uint32_t _;
    token tok = {0};
    while (pp_parse(pp, &tok, buf, sizeof(buf), &_)) {
        char fmt_buf[4096];
        fmt_token_verbose(fmt_buf, sizeof(fmt_buf), &tok);
        printf("%s\n", fmt_buf);
    }
}

static void
mtpf(string filename) {
    fs_add_include_paths(settings.include_paths, da_size(settings.include_paths));

    preprocessor *pp = calloc(1, sizeof(preprocessor));
    pp_init(pp, filename);

    char buf[4096];
    uint32_t _;
    token tok = {0};
    while (pp_parse(pp, &tok, buf, sizeof(buf), &_)) {
        char fmt_buf[4096];
        fmt_token(fmt_buf, sizeof(fmt_buf), &tok);
        if (tok.at_line_start) {
            printf("\n");
        } else if (tok.has_whitespace) {
            printf(" ");
        }
        printf("%s", fmt_buf);
    }
    printf("\n");
}

static void
mast(string filename) {
    token_iter *it = calloc(1, sizeof(token_iter));
    ti_init(it, filename);

    parser *p = calloc(1, sizeof(parser));
    p->it     = it;
    parse(p);
}

static void
process_file(string filename) {
    switch (settings.mode) {
        INVALID_DEFAULT_CASE;
    case M_PTV:
        mptv(filename);
        break;
    case M_PTF:
        mptf(filename);
        break;
    case M_PT:
        mpt(filename);
        break;
    case M_TPV:
        mtpv(filename);
        break;
    case M_TPF:
        mtpf(filename);
        break;
    case M_TP:
        mtp(filename);
        break;
    case M_AST:
        mast(filename);
        break;
    }
}

int
main(int argc, char **argv) {
    fs_add_default_include_paths();

    parse_clargs(argc, argv);

    if (!da_size(settings.filenames)) {
        fprintf(stderr, "error: no input files");
        return 1;
    }

    for (uint32_t filename_idx = 0; filename_idx < da_size(settings.filenames);
         filename_idx++) {
        string filename = settings.filenames[filename_idx];
        process_file(filename);
    }
    er_print_final_stats();
    return 0;
}
