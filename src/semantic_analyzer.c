#include "semantic_analyzer.h"

static void process_decl(CompilerCtx *ctx, AST *ast);

static u32 
infer_type(CompilerCtx *ctx, AST *expr) {
    u32 type = AST_TYPE_NONE;
    switch (expr->kind) {
    case AST_BINARY: {
        u32 left_type = infer_type(ctx, expr->binary.left);
        u32 right_type = infer_type(ctx, expr->binary.right);
        if (left_type == AST_TYPE_FLOAT || right_type == AST_TYPE_FLOAT) {
            type = AST_TYPE_FLOAT;
        } else {
            type = AST_TYPE_INT;
        }
    } break;
    case AST_UNARY: {
        u32 subexpr_type = infer_type(ctx, expr->unary.expr);
        type = subexpr_type;
    } break;
    case AST_LIT: {
        switch (expr->lit.kind) {
        case AST_LIT_INT: {
            type = AST_TYPE_INT;
        } break;
        case AST_LIT_REAL: {
            type = AST_TYPE_FLOAT;
        } break;
        INVALID_DEFAULT_CASE;
        }
    } break;
    case AST_IDENT: {
        SymbolTableEntry *entry = symbol_table_lookup(ctx->st, expr->ident.name);
        if (entry) {
            type = entry->ast_type;
        } else {
            report_error_ast(ctx->er, expr, "Undeclared identifier");
        }
    } break;
    INVALID_DEFAULT_CASE;
    }
    return type;
}

static void 
process_expr(CompilerCtx *ctx, AST *expr) {
    switch (expr->kind) {
    case AST_LIT: {
        // nop
    } break;
    case AST_IDENT: {
        SymbolTableEntry *entry = symbol_table_lookup(ctx->st, expr->ident.name);
        if (!entry) {
            report_error_ast(ctx->er, expr, "Undeclared identifier");
        }
    } break;
    case AST_UNARY: {
        process_expr(ctx, expr->unary.expr);
    } break;
    case AST_BINARY: {
        process_expr(ctx, expr->binary.left);
        process_expr(ctx, expr->binary.right);
    } break;
    INVALID_DEFAULT_CASE;
    }
}

static void 
process_statement(CompilerCtx *ctx, AST *statement) {
    switch (statement->kind) {
    case AST_BLOCK: {
        AST_LIST_ITER(&statement->block.statements, it) {
            process_statement(ctx, it);   
        }
    } break;
    case AST_ASSIGN: {
        AST *lvalue = statement->assign.lvalue;
        AST *rvalue = statement->assign.rvalue;
        assert(lvalue->kind == AST_IDENT);
        SymbolTableEntry *entry = symbol_table_lookup(ctx->st, lvalue->ident.name);
        if (!entry) {
            report_error_ast(ctx->er, statement, "Variable assignment before declaration");
        } 
        process_expr(ctx, rvalue);
    } break;
    case AST_DECL: {
        process_decl(ctx, statement);
    } break;
    case AST_IF: {
        process_expr(ctx, statement->ifs.cond);
        process_statement(ctx, statement->ifs.block);
        if (statement->ifs.else_block) {
            process_statement(ctx,statement->ifs.else_block);
        }
    } break;
    case AST_WHILE: {
        process_expr(ctx, statement->whiles.cond);
        process_statement(ctx, statement->whiles.block);
    } break;
    case AST_PRINT: {
        // @TODO(hl):
    } break;
    INVALID_DEFAULT_CASE;
    }
}

static void 
process_decl(CompilerCtx *ctx, AST *ast) {
    assert(ast->kind == AST_DECL);
    AST *decl_name = ast->decl.ident;
    assert(decl_name->kind == AST_IDENT);
    SymbolTableEntry *entry = symbol_table_lookup(ctx->st, decl_name->ident.name);
    if (entry) {
        report_error_ast(ctx->er, ast, "Variable of same name is already declared");
    } else {
        u32 type_kind = 0;
        AST *decl_type = ast->decl.type;
        if (!decl_type) {
            type_kind = infer_type(ctx, ast->decl.expr);
            if (!type_kind) {
                report_error_ast(ctx->er, ast, "Failed to infer type for declaration");
            }
        } else {
            type_kind = decl_type->type.kind;
        }
        symbol_table_add_entry(ctx->st, decl_name->ident.name, type_kind, ast->src_loc);
        process_expr(ctx, ast->decl.expr);
    }
}

static void 
process_func_decl(CompilerCtx *ctx, AST *ast) {
    assert(ast->kind == AST_FUNC_DECL);
    ASTFuncDecl *decl = &ast->func_decl;
    AST *func_name = decl->name;
    assert(func_name->kind == AST_IDENT);
    symbol_table_add_entry(ctx->st, func_name->ident.name, AST_TYPE_PROC, ast->src_loc);
    symbol_table_push_scope(ctx->st);
    AST *sign = decl->sign;
    assert(sign->kind == AST_FUNC_SIGNATURE);
    // Add arguments to scope
    AST_LIST_ITER(&sign->func_sign.arguments, it) {
        process_decl(ctx, it);
    }
    process_statement(ctx, decl->block);
    symbol_table_pop_scope(ctx->st);
}

void 
do_semantic_analysis(CompilerCtx *ctx, AST *ast) {
    switch (ast->kind) {
    case AST_DECL: {
        process_decl(ctx, ast);    
    } break;
    case AST_FUNC_DECL: {
        process_func_decl(ctx, ast);
    } break;
    INVALID_DEFAULT_CASE;
    }
}