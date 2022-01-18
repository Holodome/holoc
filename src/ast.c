#include "ast.h"

#include "buffer_writer.h"
#include "c_lang.h"
#include "c_types.h"
#include "str.h"

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
        case AST_UNARY_MINUS:
            buf_write(w, "-");
            fmt_astw(un->expr, w);
            break;
        case AST_UNARY_PLUS:
            buf_write(w, "+");
            fmt_astw(un->expr, w);
            break;
        case AST_UNARY_LNOT:
            buf_write(w, "!");
            fmt_astw(un->expr, w);
            break;
        case AST_UNARY_NOT:
            buf_write(w, "~");
            fmt_astw(un->expr, w);
            break;
        case AST_UNARY_PREINC:
            buf_write(w, "++");
            fmt_astw(un->expr, w);
            break;
        case AST_UNARY_POSTINC:
            fmt_astw(un->expr, w);
            buf_write(w, "++");
            break;
        case AST_UNARY_PREDEC:
            buf_write(w, "--");
            fmt_astw(un->expr, w);
            break;
        case AST_UNARY_POSTDEC:
            fmt_astw(un->expr, w);
            buf_write(w, "--");
            break;
        case AST_UNARY_DEREF:
            buf_write(w, "*");
            fmt_astw(un->expr, w);
            break;
        case AST_UNARY_ADDR:
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
        fmt_astw(ter->left, w);
        buf_write(w, " : ");
        fmt_astw(ter->right, w);
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
fmt_ast(ast *node, char *buf, uint32_t buf_size) {
    buffer_writer w = {buf, buf + buf_size};
    fmt_astw(node, &w);
    return w.cursor - buf;
}
