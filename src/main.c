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
#include "file_storage.h"
#include "pp_lexer.h"
#include "preprocessor.h"
#include "str.h"

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
    string *filenames;  // da
    string *include_paths; // da
    mode mode;
} program_settings;

static program_settings settings;

static void
parse_clargs(uint32_t argc, char **argv) {
    allocator *a      = get_system_allocator();
    string *filenames = 0;
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
            char *path = option + 2;
            uint32_t path_len = strlen(path);
            char *new_path = aalloc(a, path_len + 1);
            memcpy(new_path, path, path_len + 1);
            da_push(settings.include_paths, string(new_path, path_len), a);
        } else {
            da_push(filenames, stringz(option), a);
        }
    }
    settings.filenames = filenames;
}

static void
mptv(string filename) {
    allocator *a    = get_system_allocator();
    file_storage fs = {0};
    fs.a            = a;
    add_default_include_paths(&settings.include_paths);
    fs.include_paths = settings.include_paths;
    fs.include_path_count = da_size(settings.include_paths);

    file *f = get_file(&fs, filename, 0);

    char token_buffer[4096];
    pp_lexer *lex = aalloc(a, sizeof(*lex));
    init_pp_lexer(lex, f->contents.data, STRING_END(f->contents), token_buffer,
                  sizeof(token_buffer));
    pp_token tok = {0};
    while (pp_lexer_parse(lex, &tok)) {
        char buffer[4096];
        fmt_pp_tok_verbose(buffer, sizeof(buffer), &tok);
        printf("%s\n", buffer);
        memset(&tok, 0, sizeof(tok));
    }
}

static void
mptf(string filename) {
    allocator *a    = get_system_allocator();
    file_storage fs = {0};
    fs.a            = a;
    add_default_include_paths(&settings.include_paths);
    fs.include_paths = settings.include_paths;
    fs.include_path_count = da_size(settings.include_paths);

    file *f = get_file(&fs, filename, 0);

    char token_buffer[4096];
    pp_lexer *lex = aalloc(a, sizeof(*lex));
    init_pp_lexer(lex, f->contents.data, STRING_END(f->contents), token_buffer,
                  sizeof(token_buffer));
    pp_token tok = {0};
    while (pp_lexer_parse(lex, &tok)) {
        char buffer[4096];
        fmt_pp_tok(buffer, sizeof(buffer), &tok);
        if (tok.at_line_start) {
            printf("\n");
        } else if (tok.has_whitespace) {
            printf(" ");
        }
        printf("%s", buffer);
        memset(&tok, 0, sizeof(tok));
    }
}

static void
mpt(string filename) {
    allocator *a    = get_system_allocator();
    file_storage fs = {0};
    fs.a            = a;
    add_default_include_paths(&settings.include_paths);
    fs.include_paths = settings.include_paths;
    fs.include_path_count = da_size(settings.include_paths);

    file *f = get_file(&fs, filename, 0);

    char token_buffer[4096];
    pp_lexer *lex = aalloc(a, sizeof(*lex));
    init_pp_lexer(lex, f->contents.data, STRING_END(f->contents), token_buffer,
                  sizeof(token_buffer));
    pp_token tok = {0};
    while (pp_lexer_parse(lex, &tok)) {
        char buffer[4096];
        fmt_pp_tok(buffer, sizeof(buffer), &tok);
        printf("%s\n", buffer);
        memset(&tok, 0, sizeof(tok));
    }
}

static void
mtp(string filename) {
    allocator *a = get_system_allocator();

    file_storage fs = {0};
    fs.a            = a;
    add_default_include_paths(&settings.include_paths);
    fs.include_paths = settings.include_paths;
    fs.include_path_count = da_size(settings.include_paths);

    char pp_buf[4096];
    preprocessor *pp  = aalloc(a, sizeof(*pp));
    pp->a             = a;
    pp->fs            = &fs;
    init_pp(pp, filename, pp_buf, sizeof(pp_buf));

    token tok = {0};
    while (pp_parse(pp, &tok)) {
        char buffer[4096];
        fmt_token(buffer, sizeof(buffer), &tok);
        printf("%s\n", buffer);
    }
}

static void
mtpv(string filename) {
    allocator *a = get_system_allocator();

    file_storage fs = {0};
    fs.a            = a;
    add_default_include_paths(&settings.include_paths);
    fs.include_paths = settings.include_paths;
    fs.include_path_count = da_size(settings.include_paths);

    char pp_buf[4096];
    preprocessor *pp  = aalloc(a, sizeof(*pp));
    pp->a             = a;
    pp->fs            = &fs;
    init_pp(pp, filename, pp_buf, sizeof(pp_buf));

    token tok = {0};
    while (pp_parse(pp, &tok)) {
        char buffer[4096];
        fmt_token_verbose(buffer, sizeof(buffer), &tok);
        printf("%s\n", buffer);
    }
}

static void
mtpf(string filename) {
    allocator *a = get_system_allocator();

    file_storage fs = {0};
    fs.a            = a;
    add_default_include_paths(&settings.include_paths);
    fs.include_paths = settings.include_paths;
    fs.include_path_count = da_size(settings.include_paths);

    char pp_buf[4096];
    preprocessor *pp  = aalloc(a, sizeof(*pp));
    pp->a             = a;
    pp->fs            = &fs;
    init_pp(pp, filename, pp_buf, sizeof(pp_buf));

    token tok = {0};
    while (pp_parse(pp, &tok)) {
        char buffer[4096];
        fmt_token(buffer, sizeof(buffer), &tok);
        if (tok.at_line_start) {
            printf("\n");
        } else if (tok.has_whitespace) {
            printf(" ");
        }
        printf("%s", buffer);
    }
}

static void
mast(string filename) {
    NOT_IMPL;
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
    parse_clargs(argc, argv);

    if (!da_size(settings.filenames)) {
        fprintf(stderr, "error: no input files");
        return 1;
    }

    for (uint32_t filename_idx = 0; filename_idx < da_size(settings.filenames);
         filename_idx++) {
        string filename = settings.filenames[filename_idx];
        /* printf("Filename %u: %.*s\n", filename_idx, filename.len, */
        /*        filename.data); */

        process_file(filename);
    }

    return 0;
}
