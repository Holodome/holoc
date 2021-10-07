#include "ir.h"

static void process_decl(IR *ir, AST *ast);

static u32 
infer_type(IR *ir, AST *expr) {
    u32 type = AST_TYPE_NONE;
    switch (expr->kind) {
    case AST_BINARY: {
        u32 left_type = infer_type(ir, expr->binary.left);
        u32 right_type = infer_type(ir, expr->binary.right);
        if (left_type == AST_TYPE_FLOAT || right_type == AST_TYPE_FLOAT) {
            type = AST_TYPE_FLOAT;
        } else {
            type = AST_TYPE_INT;
        }
    } break;
    case AST_UNARY: {
        u32 subexpr_type = infer_type(ir, expr->unary.expr);
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
        Symbol_Table_Entry *entry = symbol_table_lookup(ir->ctx->st, expr->ident.name);
        if (entry) {
            type = entry->ast_type;
        } else {
            report_error_ast(ir->ctx->er, expr, "Undeclared identifier");
        }
    } break;
    INVALID_DEFAULT_CASE;
    }
    return type;
}

static void 
process_expr(IR *ir, AST *expr) {
    switch (expr->kind) {
    case AST_LIT: {
        // nop
    } break;
    case AST_IDENT: {
        Symbol_Table_Entry *entry = symbol_table_lookup(ir->ctx->st, expr->ident.name);
        if (!entry) {
            report_error_ast(ir->ctx->er, expr, "Undeclared identifier");
        }
    } break;
    case AST_UNARY: {
        process_expr(ir, expr->unary.expr);
    } break;
    case AST_BINARY: {
        process_expr(ir, expr->binary.left);
        process_expr(ir, expr->binary.right);
    } break;
    INVALID_DEFAULT_CASE;
    }
}

static void 
process_statement(IR *ir, AST *statement) {
    switch (statement->kind) {
    case AST_BLOCK: {
        AST_LIST_ITER(&statement->block.statements, it) {
            process_statement(ir, it);   
        }
    } break;
    case AST_ASSIGN: {
        AST *lvalue = statement->assign.lvalue;
        AST *rvalue = statement->assign.rvalue;
        assert(lvalue->kind == AST_IDENT);
        Symbol_Table_Entry *entry = symbol_table_lookup(ir->ctx->st, lvalue->ident.name);
        if (!entry) {
            report_error_ast(ir->ctx->er, statement, "Variable assignment before declaration");
        } 
        process_expr(ir, rvalue);
    } break;
    case AST_DECL: {
        process_decl(ir, statement);
    } break;
    case AST_IF: {
        process_expr(ir, statement->ifs.cond);
        // gen_code_for_expr(statement->ifs.cond);
        if (statement->ifs.else_block) {
            process_statement(ir,statement->ifs.else_block);
        }
        process_statement(ir, statement->ifs.block);
        
    } break;
    case AST_WHILE: {
        process_expr(ir, statement->whiles.cond);
        process_statement(ir, statement->whiles.block);
    } break;
    case AST_PRINT: {
        // @TODO(hl):
    } break;
    INVALID_DEFAULT_CASE;
    }
}

static void 
process_decl(IR *ir, AST *ast) {
    assert(ast->kind == AST_DECL);
    AST *decl_name = ast->decl.ident;
    assert(decl_name->kind == AST_IDENT);
    Symbol_Table_Entry *entry = symbol_table_lookup(ir->ctx->st, decl_name->ident.name);
    if (entry) {
        report_error_ast(ir->ctx->er, ast, "Variable of same name is already declared");
    } else {
        u32 type_kind = 0;
        AST *decl_type = ast->decl.type;
        if (!decl_type) {
            type_kind = infer_type(ir, ast->decl.expr);
            if (!type_kind) {
                report_error_ast(ir->ctx->er, ast, "Failed to infer type for declaration");
            }
        } else {
            type_kind = decl_type->type.kind;
        }
        symbol_table_add_entry(ir->ctx->st, decl_name->ident.name, type_kind, ast->src_loc);
        process_expr(ir, ast->decl.expr);
    }
}

static void 
process_func_decl(IR *ir, AST *ast) {
    assert(ast->kind == AST_FUNC_DECL);
    AST_Func_Decl *decl = &ast->func_decl;
    AST *func_name = decl->name;
    assert(func_name->kind == AST_IDENT);
    symbol_table_add_entry(ir->ctx->st, func_name->ident.name, AST_TYPE_PROC, ast->src_loc);
    symbol_table_push_scope(ir->ctx->st);
    AST *sign = decl->sign;
    assert(sign->kind == AST_FUNC_SIGNATURE);
    // Add arguments to scope
    AST_LIST_ITER(&sign->func_sign.arguments, it) {
        process_decl(ir, it);
    }
    process_statement(ir, decl->block);
    symbol_table_pop_scope(ir->ctx->st);
}

void 
ir_process_toplevel(IR *ir, AST *ast) {
    switch (ast->kind) {
    case AST_DECL: {
        process_decl(ir, ast);    
    } break;
    case AST_FUNC_DECL: {
        process_func_decl(ir, ast);
    } break;
    INVALID_DEFAULT_CASE;
    }
}