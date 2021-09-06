#include "general.h"

#include <stdlib.h>
#include <stdio.h>
#include "strings.h"
#include "tokenizer.h"
#include "parser.h"

enum {
    PROGRAM_INTERP,
    PROGRAM_TOKEN_VIEW,
    PROGRAM_AST_VIEW,
    PROGRAM_BYTECODE_GEN,
    PROGRAM_BYTECODE_EXEC,
};

const char *program_mode_strs[] = {
    "interp",
    "token_view",
    "ast_view",
    "bytecode_gen",
    "bytecode_exec"
};

typedef struct {
    b32 is_valid;
    
    b32 print_help;
    u32 mode;
    char *filename;
} ProgramSettings;

static void settings_set_mode(ProgramSettings *settings, u32 mode) {
    if (settings->mode) {
        outf("WARN: Program mode already set ('%s'). Resetting it to '%s'\n", 
            program_mode_strs[settings->mode], mode);
    }
    settings->mode = mode;
} 

static ProgramSettings parse_command_line_args(int argc, char **argv) {
    ProgramSettings settings = {0};
    
    u32 cursor = 1;
    while (cursor < argc) {
        char *arg = argv[cursor];
        if (str_eq(arg, "-token_view")) {
            settings_set_mode(&settings, PROGRAM_TOKEN_VIEW);
            ++cursor;
        } else if (str_eq(arg, "-ast_view")) {
            settings_set_mode(&settings, PROGRAM_AST_VIEW);
            ++cursor;
        } else if (str_eq(arg, "-help")) {
            settings.print_help = TRUE;
            ++cursor;   
        } else {
            if (arg[0] == '-') {
                outf("Unexpected option '%s'\n", arg);
                ++cursor;
            } else {
                settings.filename = mem_alloc_str(arg);
                ++cursor;
            }
        }
    }
    
    // validate
    settings.is_valid = TRUE;
    if (!settings.filename) {
        outf("No input file provided\n");
        settings.is_valid = FALSE;
    }
    return settings;
}

static void ast_view(AST *ast, int depth) {
    
}

static void token_view(Tokenizer *tokenizer) {
    SourceLocation *last_source_location = 0;
    
    Token *token = peek_tok(tokenizer);
    while (token->kind != TOKEN_EOS) {
        if (last_source_location && token->source_loc.line != last_source_location->line) {
            outf("\n");
        }
        last_source_location = &token->source_loc;
        
        char buffer[128];
        fmt_tok(buffer, sizeof(buffer), token);
        outf("%s ", buffer);
        token = peek_next_tok(tokenizer);
    }
    outf("\n");
}

int main(int argc, char **argv) {
    ProgramSettings settings = parse_command_line_args(argc, argv);
    if (settings.print_help) {
        // @TODO
        outf("help\n");
    }
    if (!settings.is_valid) {
        outf("Aborting\n");
        return 1;
    }
    
    FileData *input_file_data = get_file_data(settings.filename);
    if (!input_file_data) {
        outf("Failed to read file '%s'\n", settings.filename);
    }
    
    Tokenizer tokenizer = create_tokenizer(input_file_data->data, input_file_data->data_size, settings.filename);
        
    switch (settings.mode) {
        case PROGRAM_INTERP: {
            Parser parser = create_parser(&tokenizer);
            AST *ast = parse(&parser);
            (void)ast;
        } break;
        case PROGRAM_TOKEN_VIEW: {
            token_view(&tokenizer);
        } break;
        case PROGRAM_AST_VIEW: {
            Parser parser = create_parser(&tokenizer);
            AST *ast = parse(&parser);
            (void)ast;
        } break;
    }
    
    printf("Exited without errors\n");
    return 0;
}
