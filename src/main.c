#include "lib/general.h"

#include "lib/strings.h"
#include "tokenizer.h"
#include "interp.h"

enum {
    PROGRAM_TOKEN_VIEW,
    PROGRAM_AST_VIEW,
    PROGRAM_BYTECODE_GEN,
    PROGRAM_BYTECODE_EXEC,
};

const char *program_mode_strs[] = {
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
    SrcLoc *last_src_location = 0;
    
    Token *token = peek_tok(tokenizer);
    while (token->kind != TOKEN_EOS) {
        if (last_src_location && token->src_loc.line != last_src_location->line) {
            outf("\n");
        }
        last_src_location = &token->src_loc;
        
        char buffer[128];
        fmt_tok(buffer, sizeof(buffer), token);
        outf("%s ", buffer);
        token = peek_next_tok(tokenizer);
    }
    outf("\n");
}

int main(int argc, char **argv) {
    init_filesystem();
    ProgramSettings settings = parse_command_line_args(argc, argv);
    if (settings.print_help) {
        // @TODO
        outf("help\n");
    }
    if (!settings.is_valid) {
        outf("Aborting\n");
        return 1;
    }
    
    Interp *interp = create_interp(settings.filename, "out.kbex");
    do_interp(interp);
    destroy_interp(interp);
    
    outf("Exited without errors\n");
    out_stream_flush(get_stdout_stream());
    out_stream_flush(get_stderr_stream());
    return 0;
}
