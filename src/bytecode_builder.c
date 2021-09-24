#include "bytecode_builder.h"
#include <time.h>

BytecodeBuilder *create_bytecode_builder(ErrorReporter *reporter) {
    BytecodeBuilder *builder = arena_bootstrap(BytecodeBuilder, arena);
    builder->er = reporter;
    return builder;
}

void destroy_bytecode_builder(BytecodeBuilder *builder) {
    arena_clear(&builder->arena);    
}

static BytecodeBuilderVar *lookup_var(BytecodeBuilder *builder, const char *name) {
    BytecodeBuilderVar *result = 0;
    
    return result;
}

static u32 infer_type(BytecodeBuilder *builder, AST *expr) {
    u32 type = AST_TYPE_NONE;
    switch (expr->kind) {
        case AST_BINARY: {
            u32 left_type = infer_type(builder, expr->binary.left);
            u32 right_type = infer_type(builder, expr->binary.right);
            if (left_type == AST_TYPE_FLOAT || right_type == AST_TYPE_FLOAT) {
                type = AST_TYPE_FLOAT;
            } else {
                type = AST_TYPE_INT;
            }
        } break;
        case AST_UNARY: {
            u32 subexpr_type = infer_type(builder, expr->unary.expr);
            type = subexpr_type;
        } break;
        case AST_LITERAL: {
            switch (expr->literal.kind) {
                case AST_LITERAL_INT: {
                    type = AST_TYPE_INT;
                } break;
                case AST_LITERAL_REAL: {
                    type = AST_TYPE_FLOAT;
                } break;
                INVALID_DEFAULT_CASE;
            }
        } break;
        case AST_IDENT: {
            BytecodeBuilderVar *var = lookup_var(builder, expr->ident.name);
            if (var) {
                type = var->type;
            } else {
                report_error_ast(builder->er, expr, "Use of undeclared identifier '%s'",
                    expr->ident.name);
            }
        } break;
        INVALID_DEFAULT_CASE;
    }
    return type;
}

static u64 compile_time_expr_evaluate(BytecodeBuilder *builder, AST *expr, u32 type) {
    u64 result = 0;
    
    return result;
}

static void add_static_variable(BytecodeBuilder *builder, AST *decl) {
    assert(decl->kind == AST_DECL);
    
    u64 storage = 0;
    AST *ident_ast = decl->decl.ident;
    assert(ident_ast->kind == AST_IDENT);
    const char *ident_str = arena_alloc_str(&builder->arena, ident_ast->ident.name);
    
    u32 decl_type = AST_TYPE_NONE;
    switch (decl->decl.type) {
        case AST_TYPE_NONE: {
            u32 inferred_type = infer_type(builder, decl->decl.expr);
            decl_type = inferred_type;
        } break;
        case AST_TYPE_INT: {
            decl_type = AST_TYPE_INT;
        } break;
        case AST_TYPE_FLOAT: {
            decl_type = AST_TYPE_FLOAT;
        } break;
        INVALID_DEFAULT_CASE;
    }
    storage = compile_time_expr_evaluate(builder, decl->decl.expr, decl_type);
    
    BytecodeBuilderVar *var = arena_alloc_struct(&builder->arena, BytecodeBuilderVar);
    LLIST_ADD(builder->static_vars, var);
    var->ident = ident_str;
    var->storage = storage;
    var->type = decl_type;
    var->is_immutable = decl->decl.is_immutable;
}

static void add_func_def(BytecodeBuilder *builder, AST *decl) {
    assert(decl->kind == AST_FUNC_DECL);
}

void bytecode_builder_proccess_toplevel(BytecodeBuilder *builder, AST *toplevel) {
    if (!toplevel) {
        UNREACHABLE;
    }
    
    switch (toplevel->kind) {
        case AST_DECL: {
            add_static_variable(builder, toplevel);        
        } break;
        case AST_FUNC_DECL: {
            add_func_def(builder, toplevel);        
        } break;
        INVALID_DEFAULT_CASE;
    }
}

void bytecode_builder_emit_code(BytecodeBuilder *builder, OSFileHandle *out) {
    assert(os_is_file_valid(out));
    
    BytecodeExecutableHeader header = {0};
    header.magic_value = BYTECODE_MAGIC_VALUE;
    header.version_major = BYTECODE_VERSION_MAJOR;
    header.version_minor = BYTECODE_VERSION_MINOR;
    header.compiler_version_major = COMPILER_VERSION_MAJOR;
    header.compiler_version_minor = COMPILER_VERSION_MINOR;
    header.compile_epoch = (u64)time(0);
}
