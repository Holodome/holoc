#include "lexer.h"
#include "parser.h"
#include "bytecode_builder.h"

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

void do_interp(const char *filename, const char *out_filename) {
    MemoryArena interp_arena = {0};
    StringStorage *ss = create_string_storage(STRING_STORAGE_HASH_SIZE, &interp_arena);
    ErrorReporter *er = create_error_reporter(get_stdout_stream(), get_stderr_stream(), &interp_arena);
    
    FileID in_file_id = fs_open_file(filename, FILE_MODE_READ);
    InStream in_file_st = {0};
    init_in_streamf(&in_file_st, fs_get_handle(in_file_id), 
        arena_alloc(&interp_arena, IN_STREAM_DEFAULT_BUFFER_SIZE), IN_STREAM_DEFAULT_BUFFER_SIZE,
        IN_STREAM_DEFAULT_THRESHLOD);
    Lexer *lexer = create_tokenizer(er, ss, &in_file_st, in_file_id);
    Parser *parser = create_parser(lexer, ss, er);
    BytecodeBuilder *builder = create_bytecode_builder(er);
    for (;;) {
        AST *toplevel = parser_parse_toplevel(parser);
        if (!toplevel || is_error_reported(er)) {
            break;
        }
        fmt_ast_tree(ss, get_stdout_stream(), toplevel, 0);
        bytecode_builder_proccess_toplevel(builder, toplevel);
    }
    destroy_parser(parser);
    destroy_tokenizer(lexer);
    fs_close_file(in_file_id);
    
    if (!is_error_reported(er)) {
        FileID out_file = fs_open_file(out_filename, FILE_MODE_WRITE);
        bytecode_builder_emit_code(builder, fs_get_handle(out_file));
        fs_close_file(out_file);
    }
    destroy_bytecode_builder(builder);
    arena_clear(&interp_arena);
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
    
    do_interp(settings.filename, "out.pkex");
    
    outf("Exited without errors\n");
    out_stream_flush(get_stdout_stream());
    out_stream_flush(get_stderr_stream());
    return 0;
}
