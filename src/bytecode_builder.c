#include "bytecode_builder.h"
#include <time.h>
#include "lib/hashing.h"
#include "lib/lists.h"

BytecodeBuilder *create_bytecode_builder(CompilerCtx *ctx) {
    BytecodeBuilder *builder = arena_bootstrap(BytecodeBuilder, arena);
    builder->ctx = ctx;
    return builder;
}

void destroy_bytecode_builder(BytecodeBuilder *builder) {
    arena_clear(&builder->arena);    
}

static BytecodeBuilderVar *lookup_var(BytecodeBuilder *builder, StringID id) {
    BytecodeBuilderVar *result = 0;
    (void)builder;
    (void)id;
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
        BytecodeBuilderVar *var = lookup_var(builder, expr->ident.name);
        if (var) {
            type = var->type;
        } else {
            report_error_ast(builder->ctx->er, expr, "Use of undeclared identifier '%s'",
                expr->ident.name);
        }
    } break;
    INVALID_DEFAULT_CASE;
    }
    return type;
}

static u64 compile_time_expr_evaluate(BytecodeBuilder *builder, AST *expr, u32 type) {
    (void)builder;
    u64 result = 0;
    assert(expr->kind == AST_LIT);
    if (expr->lit.kind == AST_LIT_INT && type == AST_TYPE_INT) {
        result = expr->lit.value_int;
    } else {
        assert(false);
    }
    return result;
}

static void add_static_variable(BytecodeBuilder *builder, AST *decl) {
    assert(decl->kind == AST_DECL);
    
    u64 storage = 0;
    AST *ident_ast = decl->decl.ident;
    assert(ident_ast->kind == AST_IDENT);
    
    u32 decl_type = AST_TYPE_NONE;
    if (decl->decl.type) {
        AST *type = decl->decl.type;
        assert(type->kind == AST_TYPE);
        decl_type = type->type.kind;
    }
    if (!decl_type) {
        decl_type = infer_type(builder, decl->decl.expr);
        if (!decl_type) {
            report_error_ast(builder->ctx->er, decl, "Failed to infer type for declaration");
        }
    }
    storage = compile_time_expr_evaluate(builder, decl->decl.expr, decl_type);
    out_streamf(get_stdout_stream(), "\n");
    out_stream_flush(get_stdout_stream());
    
    BytecodeBuilderVar *var = arena_alloc_struct(&builder->arena, BytecodeBuilderVar);
    STACK_ADD(builder->static_vars, var);
    var->name = ident_ast->ident.name;
    var->storage = storage;
    var->type = decl_type;
    // var->is_immutable = decl->decl.is_immutable;
}

static void add_func_def(BytecodeBuilder *builder, AST *decl) {
    assert(decl->kind == AST_FUNC_DECL);
    BytecodeBuilderFunction *function = arena_alloc_struct(&builder->arena, BytecodeBuilderFunction);
    STACK_ADD(builder->functions, function);
    // Parse function name
    AST *func_name = decl->func_decl.name;
    assert(func_name->kind == AST_IDENT);
    // @TODO Parse function signature
    // AST *func_sign = decl->func_decl.sign;
    // assert(func_sign->kind == AST_FUNC_SIGNATURE);
    // u32 nreturn_values = 0;
    // AST_LIST_ITER(&func_sign->func_sign.return_types, value) {
    //     assert(nreturn_values < MAXIMUM_RETURN_VALUES);
    //     assert(value->kind == AST_IDENT);
    //     function->return_values[nreturn_values] = value->type.kind;
    //     ++nreturn_values;        
    // }
    // function->nreturn_values = nreturn_values;
    //@TODO Arguments
    
    // Parse function block
    AST *block = decl->func_decl.block;
    AST_LIST_ITER(&block->block.statements, statement) {
        switch (statement->kind) {
        case AST_ASSIGN: {
            
        } break;
        case AST_IF: {
            
        } break;
        case AST_RETURN: {
            
        } break;
        case AST_WHILE: {
            
        } break;
        case AST_PRINT: {
            
        } break;
        case AST_DECL: {
            
        } break;
    }
    }
}

void bytecode_builder_proccess_toplevel(BytecodeBuilder *builder, AST *toplevel) {
    (void)builder;
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
    (void)builder;
    assert(os_is_file_valid(out));
    
    BytecodeExecutableHeader header = {0};
    header.magic_value = BYTECODE_MAGIC_VALUE;
    header.version_major = BYTECODE_VERSION_MAJOR;
    header.version_minor = BYTECODE_VERSION_MINOR;
    header.compiler_version_major = COMPILER_VERSION_MAJOR;
    header.compiler_version_minor = COMPILER_VERSION_MINOR;
    header.compile_epoch = (u64)time(0);
}
