#include "compiler_ctx.h"

#include "lib/memory.h"

#include "file_registry.h"
#include "error_reporter.h"
#include "string_storage.h"

Compiler_Ctx *
create_compiler_ctx(void) {
    Compiler_Ctx *ctx = arena_bootstrap(Compiler_Ctx, arena);
    ctx->fr = create_file_registry(ctx);
    ctx->ss = create_string_storage(ctx);
    ctx->er = 0; // @TODO
    return ctx;
}
