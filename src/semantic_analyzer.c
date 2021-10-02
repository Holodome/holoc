#include "semantic_analyzer.h"

static void 
process_decl(CompilerCtx *ctx) {
    (void)ctx;
}

static void 
process_func_decl(CompilerCtx *ctx) {
    (void)ctx;
}

void 
do_semantic_analysis(CompilerCtx *ctx, AST *ast) {
    switch (ast->kind) {
    case AST_DECL: {
        process_decl(ctx);    
    } break;
    case AST_FUNC_DECL: {
        
    } break;
    INVALID_DEFAULT_CASE;
    }
}