#include "lib/general.h"

#include "lib/memory.h"
#include "lib/strings.h"
#include "lib/stream.h"

#include "compiler_ctx.h"
#include "lexer.h"

static void 
fmt_src_loc(Out_Stream *stream, const Src_Loc *loc, File_Registry *fr, u32 depth) {
    switch (loc->kind) {
    case SRC_LOC_FILE: {
        out_streamf(stream, "%*c FILE '%s'%u:%u ", 
            depth, ' ',
            get_file_path(fr, loc->file), loc->line + 1, loc->symb + 1);
    } break;
    case SRC_LOC_MACRO: {
        out_streamf(stream, "%*c MACRO %u", depth, ' ', loc->symb + 1);
        // fmt_src_loc(stream, loc->parent, fr, depth + 1);
    } break;
    case SRC_LOC_MACRO_ARG: {
        out_streamf(stream, "%*c MACRO ARG: %u", depth, ' ', loc->symb + 1);
    } break;
    }
    if (loc->parent) {
        out_streamf(stream, "\n");
        fmt_src_loc(stream, loc->parent, fr, depth + 1);
    }
}

int 
main(int argc, char **argv) {
    const char *include_paths[] = {
        "examples/tests/pp",
        "examples/tests",
        "src"
    };
    
    Compiler_Ctx *ctx = create_compiler_ctx();
    ctx->fr->include_seach_paths_count = ARRAY_SIZE(include_paths);
    ctx->fr->include_search_paths = include_paths;
    // Lexer *lexer = create_lexer(ctx, "concat.h");
    Lexer *lexer = create_lexer(ctx, "example.c");
    preprocess(lexer, STDOUT);
    // for (;;) {
    //     Token *token = peek_tok(lexer);
    //     if (token->kind == TOKEN_EOS) {
    //         break;
    //     }
    //     out_streamf(STDOUT, "\n");
    //     fmt_token(STDOUT, token);
    //     fmt_src_loc(STDOUT, token->src_loc, ctx->fr, 1);
    //     eat_tok(lexer);
    // }
    // out_stream_flush(get_stdout_stream());
    
    UNUSED(argc);
    UNUSED(argv);
    return 0;
}