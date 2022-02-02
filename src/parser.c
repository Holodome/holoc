#include "parser.h"

#include <assert.h>
#include <stddef.h>
#include <stdio.h>

#include "ast.h"
#include "c_lang.h"
#include "c_types.h"
#include "error_reporter.h"
#include "token_iter.h"

static c_type *
find_typedef(parser *p, string name) {
    c_type *type = NULL;
    (void)p;
    (void)name;
    NOT_IMPL;
    return type;
}

static c_type *
parse_type(parser *p) {
    c_type *type = NULL;
    (void)p;
    NOT_IMPL;
    return type;
}

bool
token_is_typename(parser *p, token *tok) {
    bool result = false;
    if (tok->kind == TOK_KW) {
        switch (tok->kw) {
        default:
            break;
        case C_KW_AUTO:
        case C_KW_CHAR:
        case C_KW_CONST:
        case C_KW_DOUBLE:
        case C_KW_ENUM:
        case C_KW_EXTERN:
        case C_KW_FLOAT:
        case C_KW_INT:
        case C_KW_LONG:
        case C_KW_REGISTER:
        case C_KW_RESTRICT:
        case C_KW_RETURN:
        case C_KW_SHORT:
        case C_KW_SIGNED:
        case C_KW_STRUCT:
        case C_KW_TYPEDEF:
        case C_KW_UNION:
        case C_KW_UNSIGNED:
        case C_KW_VOID:
        case C_KW_VOLATILE:
        case C_KW_ATOMIC:
        case C_KW_BOOL:
        case C_KW_COMPLEX:
        case C_KW_DECIMAL32:
        case C_KW_DECIMAL64:
        case C_KW_DECIMAL128:
        case C_KW_THREAD_LOCAL:
            result = true;
            break;
        }
    }

    if (!result && tok->kind == TOK_ID) {
        result = find_typedef(p, tok->str) != 0;
    }
    return result;
}

uint64_t
evaluate_sizeof(parser *p) {
    (void)p;
    NOT_IMPL;
    return 0;
}

ast *
parse_funccall(parser *p) {
    ast *node = NULL;
    (void)p;
    NOT_IMPL;
    return node;
}

ast *
parse_expr_primary(parser *p) {
    ast *node = NULL;

    token *tok = ti_peek(p->it);
    if (IS_PUNCT(tok, '(')) {
        ti_eat(p->it);
        node = parse_expr(p);
        tok  = ti_peek(p->it);
        if (!IS_PUNCT(tok, ')')) {
            report_error_token(tok, "Missing closing paren");
        } else {
            ti_eat(p->it);
        }
    } else if (IS_KW(tok, C_KW_SIZEOF)) {
        source_loc loc = tok->loc;

        uint64_t sizeof_value = evaluate_sizeof(p);

        node = make_ast_num_int(p->a, loc, sizeof_value, get_standard_type(C_TYPE_ULLINT));
    } else if (IS_KW(tok, C_KW_ALIGNOF)) {
        NOT_IMPL;
    } else if (IS_KW(tok, C_KW_GENERIC)) {
        NOT_IMPL;
    } else if (tok->kind == TOK_ID) {
        NOT_IMPL;
    } else if (tok->kind == TOK_NUM) {
        NOT_IMPL;
    } else if (tok->kind == TOK_STR) {
        NOT_IMPL;
    } else {
        report_error_token(tok, "Expected expression");
    }
    return node;
}

ast *
parse_expr_postfix(parser *p) {
    ast *node = parse_expr_primary(p);

    token *tok = ti_peek(p->it);
    for (;;) {
        if (tok->kind == TOK_ID) {
            node = parse_funccall(p);
        } else if (IS_PUNCT(tok, '[')) {
            // a[b] is alias for *(a + b)
            ti_eat(p->it);
            ast *idx = parse_expr(p);
            ast *add = make_ast_binary(p->a, AST_BIN_ADD, node, idx);
            node     = make_ast_unary(p->a, node->loc, AST_UN_DEREF, add);

            tok = ti_peek(p->it);
            if (!IS_PUNCT(tok, ']')) {
                report_error_token(tok, "Expected ']'");
            } else {
                tok = ti_eat_peek(p->it);
            }
        } else if (IS_PUNCT(tok, '.')) {
            NOT_IMPL;
        } else if (IS_PUNCT(tok, C_PUNCT_ARROW)) {
            NOT_IMPL;
        } else if (IS_PUNCT(tok, C_PUNCT_INC)) {
            ti_eat(p->it);

            node = make_ast_unary(p->a, node->loc, AST_UN_POSTINC, node);

            tok = ti_peek(p->it);
        } else if (IS_PUNCT(tok, C_PUNCT_DEC)) {
            ti_eat(p->it);

            node = make_ast_unary(p->a, node->loc, AST_UN_POSTDEC, node);

            tok = ti_peek(p->it);
        } else {
            break;
        }
    }
    return node;
}

ast *
parse_expr_unary(parser *p) {
    ast *node = 0;

    token *tok = ti_peek(p->it);
    if (IS_PUNCT(tok, '+')) {
        source_loc loc = tok->loc;
        ti_eat(p->it);

        ast *expr = parse_expr_cast(p);
        node      = make_ast_unary(p->a, loc, AST_UN_PLUS, expr);
    } else if (IS_PUNCT(tok, '-')) {
        source_loc loc = tok->loc;
        ti_eat(p->it);

        ast *expr = parse_expr_cast(p);
        node      = make_ast_unary(p->a, loc, AST_UN_MINUS, expr);
    } else if (IS_PUNCT(tok, '!')) {
        source_loc loc = tok->loc;
        ti_eat(p->it);

        ast *expr = parse_expr_cast(p);
        node      = make_ast_unary(p->a, loc, AST_UN_LNOT, expr);
    } else if (IS_PUNCT(tok, '~')) {
        source_loc loc = tok->loc;
        ti_eat(p->it);

        ast *expr = parse_expr_cast(p);
        node      = make_ast_unary(p->a, loc, AST_UN_NOT, expr);
    } else if (IS_PUNCT(tok, '&')) {
        source_loc loc = tok->loc;
        ti_eat(p->it);

        ast *expr = parse_expr_cast(p);
        node      = make_ast_unary(p->a, loc, AST_UN_DEREF, expr);
    } else if (IS_PUNCT(tok, '*')) {
        source_loc loc = tok->loc;
        ti_eat(p->it);

        ast *expr = parse_expr_cast(p);
        node      = make_ast_unary(p->a, loc, AST_UN_ADDR, expr);
    } else if (IS_PUNCT(tok, C_PUNCT_INC)) {
        source_loc loc = tok->loc;
        ti_eat(p->it);

        ast *expr = parse_expr_cast(p);
        node      = make_ast_unary(p->a, loc, AST_UN_PREINC, expr);
    } else if (IS_PUNCT(tok, C_PUNCT_DEC)) {
        source_loc loc = tok->loc;
        ti_eat(p->it);

        ast *expr = parse_expr_cast(p);
        node      = make_ast_unary(p->a, loc, AST_UN_PREDEC, expr);
    } else {
        node = parse_expr_postfix(p);
    }

    return node;
}

ast *
parse_expr_cast(parser *p) {
    ast *node = 0;

    token *tok = ti_peek(p->it);
    if (IS_PUNCT(tok, '(')) {
        source_loc loc = tok->loc;

        token *next = ti_peek_forward(p->it, 1);
        if (token_is_typename(p, next)) {
            ti_eat_multiple(p->it, 2);

            c_type *type = parse_type(p);

            tok = ti_peek(p->it);
            if (!IS_PUNCT(tok, ')')) {
                report_error_token(tok, "Expected ')'");
            } else {
                tok = ti_eat_peek(p->it);
            }

            ast *expr = parse_expr_cast(p);
            node      = make_ast_cast(p->a, loc, expr, type);
        }
    }

    if (node) {
        node = parse_expr_unary(p);
    }

    return node;
}

ast *
parse_expr_mul(parser *p) {
    ast *node = parse_expr_unary(p);

    token *tok = ti_peek(p->it);
    for (;;) {
        if (IS_PUNCT(tok, '*')) {
            ti_eat(p->it);
            ast *right = parse_expr_unary(p);
            node       = make_ast_binary(p->a, AST_BIN_MUL, node, right);
        } else if (IS_PUNCT(tok, '/')) {
            ti_eat(p->it);
            ast *right = parse_expr_unary(p);
            node       = make_ast_binary(p->a, AST_BIN_DIV, node, right);
        } else if (IS_PUNCT(tok, '%')) {
            ti_eat(p->it);
            ast *right = parse_expr_unary(p);
            node       = make_ast_binary(p->a, AST_BIN_MOD, node, right);
        } else {
            break;
        }
    }
    return node;
}

ast *
parse_expr_add(parser *p) {
    ast *node = parse_expr_mul(p);

    token *tok = ti_peek(p->it);
    for (;;) {
        if (IS_PUNCT(tok, '+')) {
            ti_eat(p->it);
            ast *right = parse_expr_mul(p);
            node       = make_ast_binary(p->a, AST_BIN_ADD, node, right);
        } else if (IS_PUNCT(tok, '-')) {
            ti_eat(p->it);
            ast *right = parse_expr_mul(p);
            node       = make_ast_binary(p->a, AST_BIN_SUB, node, right);
        } else {
            break;
        }
    }
    return node;
}

ast *
parse_expr_shift(parser *p) {
    ast *node = parse_expr_add(p);

    token *tok = ti_peek(p->it);
    for (;;) {
        if (IS_PUNCT(tok, C_PUNCT_LSHIFT)) {
            ti_eat(p->it);
            ast *right = parse_expr_add(p);
            node       = make_ast_binary(p->a, AST_BIN_LSHIFT, node, right);
        } else if (IS_PUNCT(tok, C_PUNCT_RSHIFT)) {
            ti_eat(p->it);
            ast *right = parse_expr_add(p);
            node       = make_ast_binary(p->a, AST_BIN_RSHIFT, node, right);
        } else {
            break;
        }
    }
    return node;
}

ast *
parse_expr_rel(parser *p) {
    ast *node = parse_expr_shift(p);

    token *tok = ti_peek(p->it);
    for (;;) {
        if (IS_PUNCT(tok, '<')) {
            ti_eat(p->it);
            ast *right = parse_expr_shift(p);
            node       = make_ast_binary(p->a, AST_BIN_L, node, right);
        } else if (IS_PUNCT(tok, '>')) {
            ti_eat(p->it);
            ast *right = parse_expr_shift(p);
            node       = make_ast_binary(p->a, AST_BIN_G, node, right);
        } else if (IS_PUNCT(tok, C_PUNCT_LEQ)) {
            ti_eat(p->it);
            ast *right = parse_expr_shift(p);
            node       = make_ast_binary(p->a, AST_BIN_LE, node, right);
        } else if (IS_PUNCT(tok, C_PUNCT_GEQ)) {
            ti_eat(p->it);
            ast *right = parse_expr_shift(p);
            node       = make_ast_binary(p->a, AST_BIN_GE, node, right);
        } else {
            break;
        }
    }
    return node;
}

ast *
parse_expr_eq(parser *p) {
    ast *node = parse_expr_rel(p);

    token *tok = ti_peek(p->it);
    for (;;) {
        if (IS_PUNCT(tok, C_PUNCT_EQ)) {
            ti_eat(p->it);
            ast *right = parse_expr_rel(p);
            node       = make_ast_binary(p->a, AST_BIN_EQ, node, right);
        } else if (IS_PUNCT(tok, C_PUNCT_NEQ)) {
            ti_eat(p->it);
            ast *right = parse_expr_rel(p);
            node       = make_ast_binary(p->a, AST_BIN_NEQ, node, right);
        } else {
            break;
        }
    }
    return node;
}

ast *
parse_expr_and(parser *p) {
    ast *node = parse_expr_eq(p);

    token *tok = ti_peek(p->it);
    for (;;) {
        if (IS_PUNCT(tok, '&')) {
            ti_eat(p->it);
            ast *right = parse_expr_eq(p);
            node       = make_ast_binary(p->a, AST_BIN_AND, node, right);
        } else {
            break;
        }
    }
    return node;
}

ast *
parse_expr_xor(parser *p) {
    ast *node = parse_expr_and(p);

    token *tok = ti_peek(p->it);
    for (;;) {
        if (IS_PUNCT(tok, '^')) {
            ti_eat(p->it);
            ast *right = parse_expr_and(p);
            node       = make_ast_binary(p->a, AST_BIN_XOR, node, right);
        } else {
            break;
        }
    }
    return node;
}

ast *
parse_expr_or(parser *p) {
    ast *node = parse_expr_xor(p);

    token *tok = ti_peek(p->it);
    for (;;) {
        if (IS_PUNCT(tok, '|')) {
            ti_eat(p->it);
            ast *right = parse_expr_xor(p);
            node       = make_ast_binary(p->a, AST_BIN_OR, node, right);
        } else {
            break;
        }
    }
    return node;
}

ast *
parse_expr_land(parser *p) {
    ast *node = parse_expr_or(p);

    token *tok = ti_peek(p->it);
    for (;;) {
        if (IS_PUNCT(tok, C_PUNCT_LAND)) {
            ti_eat(p->it);
            ast *right = parse_expr_or(p);
            node       = make_ast_binary(p->a, AST_BIN_LAND, node, right);
        } else {
            break;
        }
    }
    return node;
}

ast *
parse_expr_lor(parser *p) {
    ast *node = parse_expr_land(p);

    token *tok = ti_peek(p->it);
    for (;;) {
        if (IS_PUNCT(tok, C_PUNCT_LOR)) {
            ti_eat(p->it);
            ast *right = parse_expr_land(p);
            node       = make_ast_binary(p->a, AST_BIN_LOR, node, right);
        } else {
            break;
        }
    }
    return node;
}

ast *
parse_expr_cond(parser *p) {
    ast *node = parse_expr_lor(p);

    token *tok = ti_peek(p->it);
    if (IS_PUNCT(tok, '?')) {
        tok = ti_eat_peek(p->it);

        ast *cond_true = parse_expr(p);
        tok            = ti_peek(p->it);

        if (!IS_PUNCT(tok, ':')) {
            report_error_token(tok, "':' expected");
        } else {
            ti_eat(p->it);
        }

        ast *cond_false = parse_expr_cond(p);

        node = make_ast_ternary(p->a, node, cond_true, cond_false);
    }
    return node;
}

ast *
parse_expr_assign(parser *p) {
    ast *node = parse_expr_cond(p);

    token *tok = ti_peek(p->it);
    if (tok->kind == TOK_PUNCT) {
        switch (tok->punct) {
        default:
            break;
        case '=':
            node = make_ast_binary(p->a, AST_BIN_A, node, parse_expr_assign(p));
            break;
        case C_PUNCT_IRSHIFT:
            node = make_ast_binary(p->a, AST_BIN_RSHIFTA, node, parse_expr_assign(p));
            break;
        case C_PUNCT_ILSHIFT:
            node = make_ast_binary(p->a, AST_BIN_LSHIFTA, node, parse_expr_assign(p));
            break;
        case C_PUNCT_IADD:
            node = make_ast_binary(p->a, AST_BIN_ADDA, node, parse_expr_assign(p));
            break;
        case C_PUNCT_ISUB:
            node = make_ast_binary(p->a, AST_BIN_SUBA, node, parse_expr_assign(p));
            break;
        case C_PUNCT_IMUL:
            node = make_ast_binary(p->a, AST_BIN_MULA, node, parse_expr_assign(p));
            break;
        case C_PUNCT_IDIV:
            node = make_ast_binary(p->a, AST_BIN_DIVA, node, parse_expr_assign(p));
            break;
        case C_PUNCT_IMOD:
            node = make_ast_binary(p->a, AST_BIN_MODA, node, parse_expr_assign(p));
            break;
        case C_PUNCT_IAND:
            node = make_ast_binary(p->a, AST_BIN_ANDA, node, parse_expr_assign(p));
            break;
        case C_PUNCT_IOR:
            node = make_ast_binary(p->a, AST_BIN_ORA, node, parse_expr_assign(p));
            break;
        case C_PUNCT_IXOR:
            node = make_ast_binary(p->a, AST_BIN_XORA, node, parse_expr_assign(p));
            break;
        }
    }

    return node;
}

ast *
parse_expr(parser *p) {
    ast *node = parse_expr_assign(p);

    token *tok = ti_peek(p->it);
    if (IS_PUNCT(tok, ',')) {
        node = make_ast_binary(p->a, AST_BIN_COMMA, node, parse_expr(p));
    }

    return node;
}

void
parse(parser *p) {
    for (;;) {
        ast *expr = parse_expr(p);
        if (!expr) {
            break;
        }

        char buffer[4096];
        fmt_ast(expr, buffer, sizeof(buffer));
        printf("%s\n", buffer);
    }
}
