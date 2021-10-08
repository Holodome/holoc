#include "compiler_ctx.h"

#include "lib/stream.h"

#include "error_reporter.h"
#include "string_storage.h"
#include "symbol_table.h"

Compiler_Ctx *
create_compiler_ctx(void) {
    Compiler_Ctx *ctx = arena_bootstrap(Compiler_Ctx, arena);
    ctx->er = create_error_reporter(get_stdout_stream(), get_stderr_stream(), ctx->arena);
    ctx->ss = create_string_storage(ctx->arena);
    ctx->st = create_symbol_table(ctx->arena);
    return ctx;
}

void 
destroy_compiler_ctx(Compiler_Ctx *ctx) {
    arena_clear(ctx->arena);
}

