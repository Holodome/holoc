#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "allocator.h"
#include "bump_allocator.h"
#include "darray.h"
#include "pp_lexer.h"
#include "preprocessor.h"
#include "str.h"

typedef struct {
    bool ptv;   // print preprocessing tokens verbose, its on its own line
    bool ptvf;  // print preprocessing tokens verbose, as if they were on
                // logical source lines
    bool ptf;   // print preprocessing tokens, as if they were on logical source
                // lines
    bool pt;    // print preprocessing tokens, each on its own line
} program_settings;

static program_settings settings;

static void
erroutf(char *format, ...) {
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
}

static string
read_file_data(string filename, allocator *a) {
    FILE *file = fopen(filename.data, "r");
    assert(file);
    fseek(file, 0, SEEK_END);
    uint32_t size = ftell(file);
    fseek(file, 0, SEEK_SET);
    char *data = aalloc(a, size + 1);
    fread(data, 1, size, file);
    data[size] = 0;
    fclose(file);

    return string(data, size);
}

static void
canonicalize_newline(char *p) {
    char *read  = p;
    char *write = p;
    while (*read) {
        if (read[0] == '\r' && read[1] == '\n') {
            read += 2;
            *write++ = '\n';
        } else if (*read == '\r') {
            ++read;
            *write++ = '\n';
        } else {
            *write++ = *read++;
        }
    }
    *write = 0;
}

static void
replace_trigraphs(char *p) {
    char *read  = p;
    char *write = p;
    while (*read) {
        if (read[0] == '?' && read[1] == '?') {
            switch (read[2]) {
            default: {
                *write++ = *read++;
                *write++ = *read++;
            } break;
            case '<': {
                read += 3;
                *write++ = '{';
            } break;
            case '>': {
                read += 3;
                *write++ = '}';
            } break;
            case '(': {
                read += 3;
                *write++ = '[';
            } break;
            case ')': {
                read += 3;
                *write++ = ']';
            } break;
            case '=': {
                read += 3;
                *write++ = '#';
            } break;
            case '/': {
                read += 3;
                *write++ = '\\';
            } break;
            case '\'': {
                read += 3;
                *write++ = '^';
            } break;
            case '!': {
                read += 3;
                *write++ = '|';
            } break;
            case '-': {
                read += 3;
                *write++ = '~';
            } break;
            }
        } else {
            *write++ = *read++;
        }
    }
    *write = 0;
}

static void
remove_backslash_newline(char *p) {
    char *write = p;
    char *read  = p;
    uint32_t n  = 0;
    while (*read) {
        if (read[0] == '\\' && read[1] == '\n') {
            read += 2;
            ++n;
        } else if (*read == '\n') {
            *write++ = *read++;
            while (n--) {
                *write++ = '\n';
            }
            n = 0;
        } else {
            *write++ = *read++;
        }
    }

    while (n--) {
        *write++ = '\n';
    }

    *write = 0;
}

static void
process_file(string filename) {
    allocator *a     = get_system_allocator();
    string file_data = read_file_data(filename, a);

    char *file_contents = file_data.data;
    // BOM
    if (strcmp(file_contents, "\xef\xbb\xbf") == 0) {
        file_contents += 3;
    }
    // Phase 1
    canonicalize_newline(file_contents);
    replace_trigraphs(file_contents);
    // Phase 2
    remove_backslash_newline(file_contents);
    // Now we are ready for phase 3
    /* printf("%s", file_contents); */
    char lexer_buffer[4096];
    pp_lexer lexer         = {0};
    lexer.tok_buf          = lexer_buffer;
    lexer.tok_buf_capacity = sizeof(lexer_buffer);
    lexer.data             = file_contents;
    lexer.eof              = file_contents + strlen(file_contents);
    lexer.cursor           = lexer.data;

    if (settings.ptv) {
        while (pp_lexer_parse(&lexer)) {
            char token_buf[4096];
            fmt_pp_tok_verbose(&lexer.tok, token_buf, sizeof(token_buf));
            printf("%s\n", token_buf);
        }
        return;
    }

    if (settings.ptvf) {
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
        while (pp_lexer_parse(&lexer)) {
            char token_buf[4096];
            fmt_pp_tok(&lexer.tok, token_buf, sizeof(token_buf));
            printf("%s\n", token_buf);
        }
        return;
    }

    printf("Preprocessing:\n");
    bump_allocator pp_bump = {0};
    preprocessor *pp       = bump_alloc(&pp_bump, sizeof(preprocessor));
    pp->lex                = &lexer;
    pp->a                  = &pp_bump;
    do_pp(pp);
}

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
        } else {
            da_push(filenames, WRAP_Z(argv[arg_idx]), a);
        }
    }

    da_push(filenames, WRAP_Z("examples/example.c"), a);

    if (!da_size(filenames)) {
        erroutf("No input files\n");
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
