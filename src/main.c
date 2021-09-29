#include "lexer.h"
#include "parser.h"
#include "bytecode_builder.h"
#include "lib/clarg_parse.h"

static void
do_compile(const char *filename, const char *out_filename) {
    MemoryArena interp_arena = {0};
    StringStorage *ss = create_string_storage(STRING_STORAGE_HASH_SIZE, &interp_arena);
    ErrorReporter *er = create_error_reporter(get_stdout_stream(), get_stderr_stream(), &interp_arena);
    
    FileID in_file_id = fs_open_file(filename, FILE_MODE_READ);
    InStream in_file_st = {0};
    init_in_streamf(&in_file_st, fs_get_handle(in_file_id), 
        arena_alloc(&interp_arena, IN_STREAM_DEFAULT_BUFFER_SIZE), IN_STREAM_DEFAULT_BUFFER_SIZE,
        IN_STREAM_DEFAULT_THRESHLOD);
    Lexer *lexer = create_lexer(er, ss, &in_file_st, in_file_id);
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
    destroy_lexer(lexer);
    fs_close_file(in_file_id);
    
    if (!is_error_reported(er)) {
        FileID out_file = fs_open_file(out_filename, FILE_MODE_WRITE);
        bytecode_builder_emit_code(builder, fs_get_handle(out_file));
        fs_close_file(out_file);
    }
    destroy_bytecode_builder(builder);
    arena_clear(&interp_arena);
}

static void
do_tokenizing(const char *filename) {
    MemoryArena interp_arena = {0};
    StringStorage *ss = create_string_storage(STRING_STORAGE_HASH_SIZE, &interp_arena);
    ErrorReporter *er = create_error_reporter(get_stdout_stream(), get_stderr_stream(), &interp_arena);
    
    FileID in_file_id = fs_open_file(filename, FILE_MODE_READ);
    InStream in_file_st = {0};
    init_in_streamf(&in_file_st, fs_get_handle(in_file_id), 
        arena_alloc(&interp_arena, IN_STREAM_DEFAULT_BUFFER_SIZE), IN_STREAM_DEFAULT_BUFFER_SIZE,
        IN_STREAM_DEFAULT_THRESHLOD);
    Lexer *lexer = create_lexer(er, ss, &in_file_st, in_file_id);
    u32 last_line_number = (u32)-1;
    for (;;) {
        Token *tok = peek_tok(lexer);
        if (tok->kind != TOKEN_EOS) {
            char token_bf[1024];
            fmt_tok(token_bf, sizeof(token_bf), ss, tok);
            if (tok->src_loc.line != last_line_number) {
                out_streamf(get_stdout_stream(), "\n");
            }
            last_line_number = tok->src_loc.line;
            out_streamf(get_stdout_stream(), "%s ", token_bf);
            eat_tok(lexer);
        } else {
            break;
        }
    }
    out_streamf(get_stdout_stream(), "\n");
    destroy_lexer(lexer);
    fs_close_file(in_file_id);
    
    arena_clear(&interp_arena);
}

enum {
    PROGRAM_COMPILE,
    PROGRAM_TOKENIZE,
    PROGRAM_AST,
    PROGRAM_AST_TO_SRC,
};

typedef struct {
    char **filenames;
    char *out_filename;
    char *mode_str;
    u32 mode;
} ProgramSettings;

int main(int argc, char **argv) {
    init_filesystem();
    CLArgInfo arg_infos[] = {
        {
            .name = "Filenames",
            .narg = CLARG_NARG,
            .output_offset = STRUCT_OFFSET(ProgramSettings, filenames),
            .type = CLARG_TYPE_STR
        },
        {
            .name = "-out",
            .narg = 1,
            .output_offset = STRUCT_OFFSET(ProgramSettings, out_filename),
            .type = CLARG_TYPE_STR
        },
        {
            .name = "-mode",
            .narg = 1,
            .output_offset = STRUCT_OFFSET(ProgramSettings, mode_str),
            .type = CLARG_TYPE_STR,
        }
    };
    ProgramSettings settings = {};
    clarg_parse(&settings, arg_infos, ARRAY_SIZE(arg_infos), argc, argv);
    if (!settings.mode_str) {
        settings.mode = PROGRAM_COMPILE;
    } else if (str_eq(settings.mode_str, "tokenize")) {
        settings.mode = PROGRAM_TOKENIZE;
    } else if (str_eq(settings.mode_str, "ast")) {
        settings.mode = PROGRAM_AST;
    } else if (str_eq(settings.mode_str, "ast_to_src")) {
        settings.mode = PROGRAM_AST_TO_SRC;
    } else if (str_eq(settings.mode_str, "compile")) {
        settings.mode = PROGRAM_COMPILE;
    } else {
        erroutf("Unknown -mode: %s\n", settings.mode_str);
    }
    
    assert(settings.filenames);
    const char *filename = settings.filenames[0];
    switch (settings.mode) {
        case PROGRAM_COMPILE: {
            do_compile(filename, "out.pkex");
        } break;
        case PROGRAM_TOKENIZE: {
            do_tokenizing(filename);
        } break;
        case PROGRAM_AST: {
            NOT_IMPLEMENTED;
        } break;
        case PROGRAM_AST_TO_SRC: {
            NOT_IMPLEMENTED;
        } break;
    }
    outf("Exited without errors\n");
    out_stream_flush(get_stdout_stream());
    out_stream_flush(get_stderr_stream());
    return 0;
}
