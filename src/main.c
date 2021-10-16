#include "lib/general.h"

#include "lib/memory.h"
#include "lib/strings.h"

#include "compiler_ctx.h"
#include "lexer.h"

int 
main(int argc, char **argv) {
    const char *include_paths[] = {
        "examples/tests/pp"
    };
    
    Compiler_Ctx *ctx = create_compiler_ctx();
    ctx->fr->include_seach_paths_count = ARRAY_SIZE(include_paths);
    ctx->fr->include_search_paths = include_paths;
    Lexer *lexer = create_lexer(ctx, "macro_expansion.h");
    for (;;) {
        Token *token = peek_tok(lexer);
        if (token->kind == TOKEN_EOS) {
            break;
        }
        char buf[4096];
        fmt_token(buf, sizeof(buf), token);
        outf("%s\n", buf);
        eat_tok(lexer);
    }
    
    UNUSED(argc);
    UNUSED(argv);
    return 0;
}