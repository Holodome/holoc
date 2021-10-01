#include "compiler_ctx.h"

CompilerCtx *
create_compiler_ctx(void) {
    CompilerCtx *ctx = arena_bootstrap(CompilerCtx, arena);
    ctx->er = create_error_reporter(get_stdout_stream(), get_stderr_stream(), &ctx->arena);
    ctx->ss = create_string_storage(&ctx->arena);
    ctx->st = create_symbol_table(&ctx->arena);
    return ctx;
}

void 
destroy_compiler_ctx(CompilerCtx *ctx) {
    arena_clear(&ctx->arena);
}

