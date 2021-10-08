#include "ir.h"

#include "lib/lists.h"

static void emit_code_for_decl(IR *ir, AST *ast);

void 
add_node(IR_Node_List *list, IR_Node *node) {
    CDLIST_ADD_LAST(&list->sentinel, node);
    // DLIST_ADD_LAST(list, node);    
}

static void 
fmt_ir_var(char *bf, uptr bf_sz, IR *ir, IR_Var var) {
    if (!var.is_temp) {
        u32 len0 = fmt(bf, bf_sz, "$");
        bf += len0;
        bf_sz -= len0;
        u32 len = string_storage_get(ir->ctx->ss, var.id, bf, bf_sz);
        bf += len;
        bf_sz -= len;
        fmt(bf, bf_sz, "%u", var.number);
    } else {
        fmt(bf, bf_sz, "%%%u", var.number);
    }
}

static void 
dump_ir_list(IR *ir, IR_Node_List *list) {
    char var_buf[4096];
    Out_Stream *stream = get_stdout_stream();
    u32 idx = 0;
    IR_Node *node;
    CDLIST_ITER(&list->sentinel, node) {
        ++idx;
        switch (node->kind) {
        INVALID_DEFAULT_CASE;
        case IR_NODE_UN: {
            fmt_ir_var(var_buf, sizeof(var_buf), ir, node->un.dest);
            out_streamf(stream, "%u un kind %u %s=", idx, node->un.kind, var_buf);
            fmt_ir_var(var_buf, sizeof(var_buf), ir, node->un.what);
            out_streamf(stream, "%s\n", var_buf);
        } break;
        case IR_NODE_BIN: {
            fmt_ir_var(var_buf, sizeof(var_buf), ir, node->bin.dest);
            out_streamf(stream, "%u bin kind %u %s=", idx, node->bin.kind, var_buf);
            fmt_ir_var(var_buf, sizeof(var_buf), ir, node->bin.left);
            out_streamf(stream, "%s ", var_buf);
            fmt_ir_var(var_buf, sizeof(var_buf), ir, node->bin.right);
            out_streamf(stream, "%s\n", var_buf);
        } break;
        case IR_NODE_VAR: {
            fmt_ir_var(var_buf, sizeof(var_buf), ir, node->var.dest);
            out_streamf(stream, "%u %s=", idx, var_buf);
            fmt_ir_var(var_buf, sizeof(var_buf), ir, node->var.source);
            out_streamf(stream, "%s\n", var_buf);
        } break;
        case IR_NODE_LIT: {
            fmt_ir_var(var_buf, sizeof(var_buf), ir, node->lit.dest);
            out_streamf(stream, "%u lit kind %u %s=%lld\n", idx, node->un.kind, var_buf, 
                node->lit.int_value);
        } break;
        }
    }
}

static IR_Node *
new_ir_node(IR *ir, u32 kind) {
    IR_Node *node = arena_alloc_struct(ir->arena, IR_Node);
    node->kind = kind;
    return node;
}

static IR_Var *
get_var_internal(IR *ir, String_ID id) {
    IR_Var *result = 0;
    
    u64 default_value = (u64)-1;
    u64 hash_idx = hash64_get(&ir->variable_hash, id.value, default_value);
    if (hash_idx != default_value) {
        result = ir->vars + hash_idx;
    } else {
        assert(ir->nvars < ARRAY_SIZE(ir->vars));
        u32 var_idx = ir->nvars++;
        result = ir->vars + var_idx;
        result->id = id;
        hash64_set(&ir->variable_hash, id.value, var_idx);
    }
    
    return result;
}

static IR_Var 
get_var(IR *ir, String_ID id) {
    IR_Var *internal = get_var_internal(ir, id);
    assert(internal);
    return *internal;
}

static IR_Var 
record_var_assingment(IR *ir, String_ID id) {
    IR_Var *internal = get_var_internal(ir, id);
    assert(internal);
    ++internal->number;
    return *internal;
}

static IR_Var 
get_new_temp(IR *ir) {
    IR_Var result = {0};
    result.is_temp = true;
    result.number = ir->current_temp_n++;
    return result;
}

static void
emit_code_for_expression(IR *ir, AST *expr, IR_Var dest) {
    switch (expr->kind) {
    case AST_LIT: {
        IR_Node *node = new_ir_node(ir, IR_NODE_LIT);
        node->lit.dest = dest;
        node->lit.kind = expr->lit.kind;
        // @HACK other value types
        node->lit.int_value = expr->lit.value_int;
        add_node(ir->node_list, node);
    } break;
    case AST_IDENT: {
        Symbol_Table_Entry *entry = symbol_table_lookup(ir->ctx->st, expr->ident.name);
        if (!entry) {
            report_error_ast(ir->ctx->er, expr, "Undeclared identifier");
        } else {
            IR_Node *node = new_ir_node(ir, IR_NODE_VAR);
            node->var.dest = dest;
            node->var.source = get_var(ir, expr->ident.name);
            add_node(ir->node_list, node);
        }
    } break;
    case AST_BINARY: {
        IR_Var temp_left = get_new_temp(ir);
        emit_code_for_expression(ir, expr->binary.left, temp_left);
        IR_Var temp_right = get_new_temp(ir);
        emit_code_for_expression(ir, expr->binary.right, temp_right);
        
        IR_Node *node = new_ir_node(ir, IR_NODE_BIN);
        node->bin.kind = expr->binary.kind;
        node->bin.left = temp_left;
        node->bin.right = temp_right;
        node->bin.dest = dest;
        add_node(ir->node_list, node);
    } break;
    case AST_UNARY: {
        if (expr->unary.kind == AST_UNARY_PLUS) {
            emit_code_for_expression(ir, expr->unary.expr, dest);
        } else {
            IR_Var temp_value = get_new_temp(ir);
            emit_code_for_expression(ir, expr, temp_value);
            
            IR_Node *node = new_ir_node(ir, IR_NODE_UN);
            node->un.kind = expr->unary.kind;
            node->un.what = temp_value;
            node->un.dest = dest;
            add_node(ir->node_list, node);
        }
    } break;
    INVALID_DEFAULT_CASE;
    }
}

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
emit_code_for_statement(IR *ir, AST *statement) {
    switch (statement->kind) {
    case AST_BLOCK: {
        AST_LIST_ITER(&statement->block.statements, it) {
            emit_code_for_statement(ir, it);
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
        IR_Var left_value = record_var_assingment(ir, lvalue->ident.name);
        emit_code_for_expression(ir, rvalue, left_value);
    } break;
    case AST_DECL: {
        emit_code_for_decl(ir, statement);
    } break;
    case AST_IF: {
        IR_Var cond_var = get_new_temp(ir);
        emit_code_for_expression(ir, statement->ifs.cond, cond_var);
        // @TODO
        // gen_code_for_expr(statement->ifs.cond);
        // if (statement->ifs.else_block) {
        //     process_statement(ir,statement->ifs.else_block);
        // }
        // process_statement(ir, statement->ifs.block);
        
    } break;
    case AST_WHILE: {
        // process_expr(ir, statement->whiles.cond);
        // process_statement(ir, statement->whiles.block);
    } break;
    case AST_PRINT: {
        // @TODO(hl):
    } break;
    INVALID_DEFAULT_CASE;
    }
}

static void
emit_code_for_decl(IR *ir, AST *ast) {
    assert(ast->kind == AST_DECL);
    AST *decl_name = ast->decl.ident;
    assert(decl_name->kind == AST_IDENT);
    Symbol_Table_Entry *entry = symbol_table_lookup(ir->ctx->st, decl_name->ident.name);
    if (entry) {
        report_error_ast(ir->ctx->er, ast, "Variable of same name is already declared");
    } else {
        u32 type_kind = 0;
        AST *decl_type = ast->decl.type;
        if (!decl_type && ast->decl.expr) {
            type_kind = infer_type(ir, ast->decl.expr);
            if (!type_kind) {
                report_error_ast(ir->ctx->er, ast, "Failed to infer type for declaration");
            }
        } else if (decl_type) {
            type_kind = decl_type->type.kind;
        } else {
            report_error_ast(ir->ctx->er, ast, "Type is not supplied");
        }
        symbol_table_add_entry(ir->ctx->st, decl_name->ident.name, type_kind, ast->src_loc);
        if (ast->decl.expr) {
            IR_Var var = get_var(ir, decl_name->ident.name);
            emit_code_for_expression(ir, ast->decl.expr, var);
        }
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
        emit_code_for_decl(ir, it);
    }
    // Process the block
    // resent the compiler settigns
    clear_hash(&ir->variable_hash);
    mem_zero(&ir->vars, sizeof(ir->vars));
    IR_Node_List list = {0};
    CDLIST_INIT(&list.sentinel);
    ir->node_list = &list;
    emit_code_for_statement(ir, decl->block);
    dump_ir_list(ir, &list);
    symbol_table_pop_scope(ir->ctx->st);
}

void 
ir_process_toplevel(IR *ir, AST *ast) {
    switch (ast->kind) {
    case AST_DECL: {
        // d_emit_code_for_decl(ir, ast);    
    } break;
    case AST_FUNC_DECL: {
        process_func_decl(ir, ast);
    } break;
    INVALID_DEFAULT_CASE;
    }
}


IR *
create_ir(Compiler_Ctx *ctx, Memory_Arena *arena) {
    IR *ir = arena_alloc_struct(arena, IR);
    ir->arena = arena;
    ir->variable_hash = create_hash64(MAX_VARIBALES, arena);
    ir->ctx = ctx;
    return ir;
}