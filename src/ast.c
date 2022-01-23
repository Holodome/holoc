#include "ast.h"

#include <assert.h>

#include "buffer_writer.h"
#include "c_lang.h"
#include "c_types.h"
#include "str.h"
#include "allocator.h"

static uint64_t AST_STRUCT_SIZES[] = {
    sizeof(ast),           sizeof(ast_identifier),
    sizeof(ast_string),    sizeof(ast_number),
    sizeof(ast_unary),     sizeof(ast_binary),
    sizeof(ast_ternary),   sizeof(ast_if),
    sizeof(ast_for),       sizeof(ast_do),
    sizeof(ast_switch),    sizeof(ast_case),
    sizeof(ast_block),     sizeof(ast_goto),
    sizeof(ast_label),     sizeof(ast_func_call),
    sizeof(ast_cast),      sizeof(ast_member),
    sizeof(ast_return),    sizeof(ast_struct_decl),
    sizeof(ast_enum_decl), sizeof(ast_enum_field),
    sizeof(ast_decl),      sizeof(ast_func),
    sizeof(ast_typedef),   sizeof(ast_type)};

static string AST_BINARY_STRS[] = {
    WRAP_Z("(unknown)"), WRAP_Z("+"),   WRAP_Z("-"),   WRAP_Z("*"),
    WRAP_Z("/"),         WRAP_Z("%"),   WRAP_Z("<="),  WRAP_Z("<"),
    WRAP_Z(">="),        WRAP_Z(">"),   WRAP_Z("=="),  WRAP_Z("!="),
    WRAP_Z("&"),         WRAP_Z("|"),   WRAP_Z("^"),   WRAP_Z("<<"),
    WRAP_Z(">>"),        WRAP_Z("&&"),  WRAP_Z("||"),  WRAP_Z("="),
    WRAP_Z("+="),        WRAP_Z("-="),  WRAP_Z("*="),  WRAP_Z("/="),
    WRAP_Z("%="),        WRAP_Z("<<="), WRAP_Z(">>="), WRAP_Z("&="),
    WRAP_Z("|="),        WRAP_Z("^="),  WRAP_Z(","),
};

static string AST_UN_STRS[] = {
    WRAP_Z("-"),  WRAP_Z("+"),  WRAP_Z("!"),  WRAP_Z("~"), WRAP_Z("++"),
    WRAP_Z("++"), WRAP_Z("--"), WRAP_Z("--"), WRAP_Z("*"), WRAP_Z("&"),
};

void
fmt_astw(void *node, buffer_writer *w) {
    switch (((ast *)node)->kind) {
    default:
        break;
    case AST_ID: {
        ast_identifier *ident = node;
        buf_write(w, "%s", ident->ident.data);
    } break;
    case AST_STR: {
        ast_string *str = node;
        fmt_c_strw((fmt_c_str_args){.type = str->type, .str = str->str}, w);
    } break;
    case AST_NUM: {
        ast_number *num = node;
        fmt_c_numw((fmt_c_num_args){.uint_value  = num->uint_value,
                                    .float_value = num->float_value,
                                    .type        = num->type},
                   w);
    } break;
    case AST_UN: {
        ast_unary *un = node;
        switch (un->un_kind) {
        case AST_UN_MINUS:
            buf_write(w, "-");
            fmt_astw(un->expr, w);
            break;
        case AST_UN_PLUS:
            buf_write(w, "+");
            fmt_astw(un->expr, w);
            break;
        case AST_UN_LNOT:
            buf_write(w, "!");
            fmt_astw(un->expr, w);
            break;
        case AST_UN_NOT:
            buf_write(w, "~");
            fmt_astw(un->expr, w);
            break;
        case AST_UN_PREINC:
            buf_write(w, "++");
            fmt_astw(un->expr, w);
            break;
        case AST_UN_POSTINC:
            fmt_astw(un->expr, w);
            buf_write(w, "++");
            break;
        case AST_UN_PREDEC:
            buf_write(w, "--");
            fmt_astw(un->expr, w);
            break;
        case AST_UN_POSTDEC:
            fmt_astw(un->expr, w);
            buf_write(w, "--");
            break;
        case AST_UN_DEREF:
            buf_write(w, "*");
            fmt_astw(un->expr, w);
            break;
        case AST_UN_ADDR:
            buf_write(w, "&");
            fmt_astw(un->expr, w);
            break;
        }
    } break;
    case AST_BIN: {
        ast_binary *bin = node;
        string op_str   = AST_BINARY_STRS[bin->bin_kind];
        fmt_astw(bin->left, w);
        buf_write(w, " %s ", op_str.data);
        fmt_astw(bin->right, w);
    } break;
    case AST_TER: {
        ast_ternary *ter = node;
        fmt_astw(ter->cond, w);
        buf_write(w, " ? ");
        fmt_astw(ter->cond_true, w);
        buf_write(w, " : ");
        fmt_astw(ter->cond_false, w);
    } break;
    case AST_FUNC_CALL: {
        ast_func_call *call = node;
        (void)call;
    } break;
    case AST_CAST: {
        ast_cast *cast = node;
        buf_write(w, "(");
        fmt_c_typew(cast->type, w);
        buf_write(w, ")");
        buf_write(w, "(");
        fmt_astw(cast->expr, w);
        buf_write(w, ")");
    } break;
    case AST_MEMB: {
        ast_member *memb = node;
        buf_write(w, "(");
        fmt_astw(memb->obj, w);
        buf_write(w, ").%s", memb->field.data);
    } break;
    }
}

uint32_t
fmt_ast(void *node, char *buf, uint32_t buf_size) {
    buffer_writer w = {buf, buf + buf_size};
    fmt_astw(node, &w);
    return w.cursor - buf;
}

void
fmt_ast_verbosew_internal(void *node, buffer_writer *w, uint32_t depth) {
    switch (((ast *)node)->kind) {
    case AST_NONE:
        buf_write(w, "%*c", depth, ' ');
        buf_write(w, "NOP\n");
        break;
    case AST_ID: {
        ast_identifier *id = node;
        buf_write(w, "%*c", depth, ' ');
        buf_write(w, "Id: %s\n", id->ident.data);
    } break;
    case AST_STR: {
        ast_string *str = node;
        buf_write(w, "%*c", depth, ' ');
        buf_write(w, "Str: ");
        fmt_c_strw((fmt_c_str_args){.type = str->type, .str = str->str}, w);
        buf_write(w, "\n");
    } break;
    case AST_NUM: {
        ast_number *num = node;
        buf_write(w, "%*c", depth, ' ');
        buf_write(w, "Num: ");
        fmt_c_numw((fmt_c_num_args){.uint_value  = num->uint_value,
                                    .float_value = num->float_value,
                                    .type        = num->type},
                   w);
        buf_write(w, "\n");
    } break;
    case AST_UN: {
        ast_unary *un = node;
        buf_write(w, "%*c", depth, ' ');
        buf_write(w, "Unary expr:\n");
        string op_str = AST_UN_STRS[un->un_kind];
        if (un->un_kind == AST_UN_POSTINC || un->un_kind == AST_UN_POSTDEC) {
            fmt_ast_verbosew_internal(un->expr, w, depth + 1);
            buf_write(w, "%*c%s\n", depth + 1, ' ', op_str.data);
        } else {
            buf_write(w, "%*c%s", depth + 1, ' ', op_str.data);
            fmt_ast_verbosew_internal(un->expr, w, depth + 1);
        }
    } break;
    case AST_BIN: {
        ast_binary *bin = node;
        buf_write(w, "%*c", depth, ' ');
        buf_write(w, "Binary expr:\n");
        string op_str = AST_BINARY_STRS[bin->bin_kind];
        fmt_ast_verbosew_internal(bin->left, w, depth + 1);
        buf_write(w, "%*c%s", depth + 1, ' ', op_str.data);
        fmt_ast_verbosew_internal(bin->right, w, depth + 1);
    } break;
    case AST_TER: {
        ast_ternary *ter = node;
        buf_write(w, "%*c", depth, ' ');
        buf_write(w, "Ternary expr:\n");
        fmt_ast_verbosew_internal(ter->cond, w, depth + 1);
        buf_write(w, "%*c?\n", depth + 1, ' ');
        fmt_ast_verbosew_internal(ter->cond_true, w, depth + 1);
        buf_write(w, "%*c:\n", depth + 1, ' ');
        fmt_ast_verbosew_internal(ter->cond_false, w, depth + 1);
    } break;
    case AST_IF: {
        ast_if *if_ = node;
        buf_write(w, "%*c", depth, ' ');
        buf_write(w, "If statement:\n");
        buf_write(w, "%*c", depth + 1, ' ');
        buf_write(w, "Cond:\n");
        fmt_ast_verbosew_internal(if_->cond, w, depth + 2);
        buf_write(w, "%*c", depth + 1, ' ');
        buf_write(w, "Cond-true:\n");
        fmt_ast_verbosew_internal(if_->cond_true, w, depth + 2);
        if (if_->cond_false) {
            buf_write(w, "%*c", depth + 1, ' ');
            buf_write(w, "Cond-false:\n");
            fmt_ast_verbosew_internal(if_->cond_true, w, depth + 2);
        }
    } break;
    case AST_FOR: {
        ast_for *for_ = node;
        if (for_->cond && !for_->init && !for_->iter) {
            // treat as while
            buf_write(w, "%*c", depth, ' ');
            buf_write(w, "While statement:\n");
            buf_write(w, "%*c", depth + 1, ' ');
            buf_write(w, "Cond:\n");
            fmt_ast_verbosew_internal(for_->cond, w, depth + 2);
            buf_write(w, "%*c", depth + 1, ' ');
            buf_write(w, "Loop:\n");
            fmt_ast_verbosew_internal(for_->loop, w, depth + 2);
        } else {
            // Treat as some modification of for
            buf_write(w, "%*c", depth, ' ');
            buf_write(w, "For statement:\n");
            buf_write(w, "%*c", depth + 1, ' ');
            buf_write(w, "Init:\n");
            fmt_ast_verbosew_internal(for_->init, w, depth + 2);
            buf_write(w, "%*c", depth + 1, ' ');
            buf_write(w, "Cond:\n");
            fmt_ast_verbosew_internal(for_->cond, w, depth + 2);
            buf_write(w, "%*c", depth + 1, ' ');
            buf_write(w, "Iter:\n");
            fmt_ast_verbosew_internal(for_->iter, w, depth + 2);
            buf_write(w, "%*c", depth + 1, ' ');
            buf_write(w, "Loop:\n");
            fmt_ast_verbosew_internal(for_->loop, w, depth + 2);
        }
    } break;
    case AST_DO: {
        ast_do *do_ = node;
        buf_write(w, "%*c", depth, ' ');
        buf_write(w, "Do-while statement:\n");
        buf_write(w, "%*c", depth + 1, ' ');
        buf_write(w, "Loop:\n");
        fmt_ast_verbosew_internal(do_->loop, w, depth + 2);
        buf_write(w, "%*c", depth + 1, ' ');
        buf_write(w, "Cond:\n");
        fmt_ast_verbosew_internal(do_->cond, w, depth + 2);
    } break;
    case AST_SWITCH: {
        ast_switch *switch_ = node;
        buf_write(w, "%*c", depth, ' ');
        buf_write(w, "Switch statement:\n");
        buf_write(w, "%*c", depth + 1, ' ');
        buf_write(w, "Expr:\n");
        fmt_ast_verbosew_internal(switch_->expr, w, depth + 2);
        buf_write(w, "%*c", depth + 1, ' ');
        buf_write(w, "Statements:\n");
        fmt_ast_verbosew_internal(switch_->sts, w, depth + 2);
    } break;
    case AST_CASE: {
        NOT_IMPL;
    } break;
    case AST_BLOCK: {
        ast_block *block = node;
        buf_write(w, "%*c", depth, ' ');
        buf_write(w, "Block:\n");
        buf_write(w, "%*c", depth + 1, ' ');
        buf_write(w, "Statements:\n");
        fmt_ast_verbosew_internal(block->st, w, depth + 2);
    } break;
    case AST_GOTO: {
        NOT_IMPL;
    } break;
    case AST_LABEL: {
        NOT_IMPL;
    } break;
    case AST_FUNC_CALL: {
    } break;
    case AST_CAST: {
        NOT_IMPL;
    } break;
    case AST_MEMB: {
        NOT_IMPL;
    } break;
    case AST_RETURN: {
        NOT_IMPL;
    } break;
    case AST_STRUCT: {
        NOT_IMPL;
    } break;
    case AST_ENUM: {
        ast_enum_decl *enum_ = node;
        buf_write(w, "%*c", depth, ' ');
        buf_write(w, "Enum declaration:\n");
        buf_write(w, "%*c", depth + 1, ' ');
        buf_write(w, "Name: %s\n", enum_->name.data);
        buf_write(w, "%*c", depth + 1, ' ');
        buf_write(w, "Enumerators: %s\n", enum_->name.data);
        for (ast_enum_field *field = enum_->fields; field;
             field                 = (void *)field->next) {
            fmt_ast_verbosew_internal(field, w, depth + 2);
        }
    } break;
    case AST_ENUM_VALUE: {
        ast_enum_field *field = node;
        buf_write(w, "%*c", depth, ' ');
        buf_write(w, "Enumerator: %s = %llu\n", field->name.data, field->value);
    } break;
    case AST_DECL: {
        NOT_IMPL;
    } break;
    case AST_FUNC: {
        NOT_IMPL;
    } break;
    case AST_TYPEDEF: {
        ast_typedef *td = node;
        buf_write(w, "%*c", depth, ' ');
        buf_write(w, "Typedef: %s\n", td->name.data);
        buf_write(w, "%*c", depth, ' ');
        buf_write(w, "Type:\n");
        fmt_ast_verbosew_internal(td->underlying, w, depth + 1);
    } break;
    case AST_TYPE: {
        NOT_IMPL;
    } break;
    }
}

void
fmt_ast_verbosew(void *node, buffer_writer *w) {
    fmt_ast_verbosew_internal(node, w, 0);
}

uint32_t
fmt_ast_verbose(void *node, char *buf, uint32_t buf_size) {
    buffer_writer w = {buf, buf + buf_size};
    fmt_ast_verbosew(node, &w);
    return w.cursor - buf;
}

void *
make_ast(struct allocator *a, ast_kind kind) {
    assert(kind < (sizeof(AST_STRUCT_SIZES) / sizeof(*AST_STRUCT_SIZES)));
    ast *node  = aalloc(a, AST_STRUCT_SIZES[kind]);
    node->kind = kind;
    return node;
}
