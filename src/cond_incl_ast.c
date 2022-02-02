#include "cond_incl_ast.h"

#include <assert.h>
#include <stddef.h>

#include "allocator.h"
#include "ast.h"
#include "c_lang.h"
#include "c_types.h"

ast *
ast_cond_incl_expr_primary(allocator *a, token **tokp) {
    ast *node  = NULL;
    token *tok = *tokp;
    if (IS_PUNCT(tok, '(')) {
        tok  = tok->next;
        node = ast_cond_incl_expr(a, &tok);
        if (!IS_PUNCT(tok, ')')) {
            NOT_IMPL;
        } else {
            tok = tok->next;
        }
    } else if (tok->kind == TOK_NUM) {
        if (!c_type_kind_is_int(tok->type->kind)) {
            NOT_IMPL;
        } else {
            ast_number *num = make_ast(a, AST_NUM, tok->loc);
            num->uint_value = tok->uint_value;
            num->type       = tok->type;
            node            = (ast *)num;
        }
        tok = tok->next;
    } else {
        NOT_IMPL;
    }
    *tokp = tok;
    return node;
}

ast *
ast_cond_incl_expr_unary(allocator *a, token **tokp) {
    ast *node  = NULL;
    token *tok = *tokp;
    if (IS_PUNCT(tok, '+')) {
        tok           = tok->next;
        ast_unary *un = make_ast(a, AST_UN, tok->loc);
        un->un_kind   = AST_UN_PLUS;
        un->expr      = ast_cond_incl_expr_primary(a, &tok);
        node          = (ast *)un;
    } else if (IS_PUNCT(tok, '-')) {
        tok           = tok->next;
        ast_unary *un = make_ast(a, AST_UN, tok->loc);
        un->un_kind   = AST_UN_MINUS;
        un->expr      = ast_cond_incl_expr_primary(a, &tok);
        node          = (ast *)un;
    } else if (IS_PUNCT(tok, '!')) {
        tok           = tok->next;
        ast_unary *un = make_ast(a, AST_UN, tok->loc);
        un->un_kind   = AST_UN_LNOT;
        un->expr      = ast_cond_incl_expr_primary(a, &tok);
        node          = (ast *)un;
    } else if (IS_PUNCT(tok, '~')) {
        tok           = tok->next;
        ast_unary *un = make_ast(a, AST_UN, tok->loc);
        un->un_kind   = AST_UN_NOT;
        un->expr      = ast_cond_incl_expr_primary(a, &tok);
        node          = (ast *)un;
    } else {
        node = ast_cond_incl_expr_primary(a, &tok);
    }
    *tokp = tok;
    return node;
}

ast *
ast_cond_incl_expr_mul(allocator *a, token **tokp) {
    ast *node  = ast_cond_incl_expr_unary(a, tokp);
    token *tok = *tokp;
    for (;;) {
        if (IS_PUNCT(tok, '*')) {
            tok             = tok->next;
            ast_binary *bin = make_ast(a, AST_BIN, tok->loc);
            bin->bin_kind   = AST_BIN_MUL;
            bin->left       = node;
            bin->right      = ast_cond_incl_expr_unary(a, &tok);
            node            = (ast *)bin;
            continue;
        } else if (IS_PUNCT(tok, '/')) {
            tok             = tok->next;
            ast_binary *bin = make_ast(a, AST_BIN, tok->loc);
            bin->bin_kind   = AST_BIN_DIV;
            bin->left       = node;
            bin->right      = ast_cond_incl_expr_unary(a, &tok);
            node            = (ast *)bin;
            continue;
        } else if (IS_PUNCT(tok, '%')) {
            tok             = tok->next;
            ast_binary *bin = make_ast(a, AST_BIN, tok->loc);
            bin->bin_kind   = AST_BIN_MOD;
            bin->left       = node;
            bin->right      = ast_cond_incl_expr_unary(a, &tok);
            node            = (ast *)bin;
            continue;
        }

        break;
    }
    *tokp = tok;
    return node;
}

ast *
ast_cond_incl_expr_add(allocator *a, token **tokp) {
    ast *node  = ast_cond_incl_expr_mul(a, tokp);
    token *tok = *tokp;
    for (;;) {
        if (IS_PUNCT(tok, '+')) {
            tok             = tok->next;
            ast_binary *bin = make_ast(a, AST_BIN, tok->loc);
            bin->bin_kind   = AST_BIN_ADD;
            bin->left       = node;
            bin->right      = ast_cond_incl_expr_mul(a, &tok);
            node            = (ast *)bin;
            continue;
        } else if (IS_PUNCT(tok, '-')) {
            tok             = tok->next;
            ast_binary *bin = make_ast(a, AST_BIN, tok->loc);
            bin->bin_kind   = AST_BIN_SUB;
            bin->left       = node;
            bin->right      = ast_cond_incl_expr_mul(a, &tok);
            node            = (ast *)bin;
            continue;
        }

        break;
    }
    *tokp = tok;
    return node;
}

ast *
ast_cond_incl_expr_shift(allocator *a, token **tokp) {
    ast *node  = ast_cond_incl_expr_add(a, tokp);
    token *tok = *tokp;
    for (;;) {
        if (IS_PUNCT(tok, C_PUNCT_LSHIFT)) {
            tok             = tok->next;
            ast_binary *bin = make_ast(a, AST_BIN, tok->loc);
            bin->bin_kind   = AST_BIN_LSHIFT;
            bin->left       = node;
            bin->right      = ast_cond_incl_expr_add(a, &tok);
            node            = (ast *)bin;
            continue;
        } else if (IS_PUNCT(tok, C_PUNCT_RSHIFT)) {
            tok             = tok->next;
            ast_binary *bin = make_ast(a, AST_BIN, tok->loc);
            bin->bin_kind   = AST_BIN_RSHIFT;
            bin->left       = node;
            bin->right      = ast_cond_incl_expr_add(a, &tok);
            node            = (ast *)bin;
            continue;
        }

        break;
    }
    *tokp = tok;
    return node;
}

ast *
ast_cond_incl_expr_rel(allocator *a, token **tokp) {
    ast *node  = ast_cond_incl_expr_shift(a, tokp);
    token *tok = *tokp;
    for (;;) {
        if (IS_PUNCT(tok, '<')) {
            tok             = tok->next;
            ast_binary *bin = make_ast(a, AST_BIN, tok->loc);
            bin->bin_kind   = AST_BIN_L;
            bin->left       = node;
            bin->right      = ast_cond_incl_expr_shift(a, &tok);
            node            = (ast *)bin;
            continue;
        } else if (IS_PUNCT(tok, '>')) {
            tok             = tok->next;
            ast_binary *bin = make_ast(a, AST_BIN, tok->loc);
            bin->bin_kind   = AST_BIN_G;
            bin->left       = node;
            bin->right      = ast_cond_incl_expr_shift(a, &tok);
            node            = (ast *)bin;
            continue;
        } else if (IS_PUNCT(tok, C_PUNCT_LEQ)) {
            tok             = tok->next;
            ast_binary *bin = make_ast(a, AST_BIN, tok->loc);
            bin->bin_kind   = AST_BIN_LE;
            bin->left       = node;
            bin->right      = ast_cond_incl_expr_shift(a, &tok);
            node            = (ast *)bin;
            continue;
        } else if (IS_PUNCT(tok, C_PUNCT_GEQ)) {
            tok             = tok->next;
            ast_binary *bin = make_ast(a, AST_BIN, tok->loc);
            bin->bin_kind   = AST_BIN_GE;
            bin->left       = node;
            bin->right      = ast_cond_incl_expr_shift(a, &tok);
            node            = (ast *)bin;
            continue;
        }

        break;
    }
    *tokp = tok;
    return node;
}

ast *
ast_cond_incl_expr_eq(allocator *a, token **tokp) {
    ast *node  = ast_cond_incl_expr_rel(a, tokp);
    token *tok = *tokp;
    for (;;) {
        if (IS_PUNCT(tok, C_PUNCT_EQ)) {
            tok             = tok->next;
            ast_binary *bin = make_ast(a, AST_BIN, tok->loc);
            bin->bin_kind   = AST_BIN_EQ;
            bin->left       = node;
            bin->right      = ast_cond_incl_expr_rel(a, &tok);
            node            = (ast *)bin;
            continue;
        } else if (IS_PUNCT(tok, C_PUNCT_NEQ)) {
            tok             = tok->next;
            ast_binary *bin = make_ast(a, AST_BIN, tok->loc);
            bin->bin_kind   = AST_BIN_NEQ;
            bin->left       = node;
            bin->right      = ast_cond_incl_expr_rel(a, &tok);
            node            = (ast *)bin;
            continue;
        }

        break;
    }
    *tokp = tok;
    return node;
}

ast *
ast_cond_incl_expr_and(allocator *a, token **tokp) {
    ast *node  = ast_cond_incl_expr_eq(a, tokp);
    token *tok = *tokp;
    for (;;) {
        if (IS_PUNCT(tok, '&')) {
            tok             = tok->next;
            ast_binary *bin = make_ast(a, AST_BIN, tok->loc);
            bin->bin_kind   = AST_BIN_AND;
            bin->left       = node;
            bin->right      = ast_cond_incl_expr_eq(a, &tok);
            node            = (ast *)bin;
            continue;
        }

        break;
    }
    *tokp = tok;
    return node;
}

ast *
ast_cond_incl_expr_xor(allocator *a, token **tokp) {
    ast *node  = ast_cond_incl_expr_and(a, tokp);
    token *tok = *tokp;
    for (;;) {
        if (IS_PUNCT(tok, '^')) {
            tok             = tok->next;
            ast_binary *bin = make_ast(a, AST_BIN, tok->loc);
            bin->bin_kind   = AST_BIN_XOR;
            bin->left       = node;
            bin->right      = ast_cond_incl_expr_and(a, &tok);
            node            = (ast *)bin;
            continue;
        }

        break;
    }
    *tokp = tok;
    return node;
}

ast *
ast_cond_incl_expr_or(allocator *a, token **tokp) {
    ast *node  = ast_cond_incl_expr_xor(a, tokp);
    token *tok = *tokp;
    for (;;) {
        if (IS_PUNCT(tok, '|')) {
            tok             = tok->next;
            ast_binary *bin = make_ast(a, AST_BIN, tok->loc);
            bin->bin_kind   = AST_BIN_OR;
            bin->left       = node;
            bin->right      = ast_cond_incl_expr_xor(a, &tok);
            node            = (ast *)bin;
            continue;
        }

        break;
    }
    *tokp = tok;
    return node;
}

ast *
ast_cond_incl_expr_land(allocator *a, token **tokp) {
    ast *node  = ast_cond_incl_expr_or(a, tokp);
    token *tok = *tokp;
    for (;;) {
        if (IS_PUNCT(tok, C_PUNCT_LAND)) {
            tok             = tok->next;
            ast_binary *bin = make_ast(a, AST_BIN, tok->loc);
            bin->bin_kind   = AST_BIN_LAND;
            bin->left       = node;
            bin->right      = ast_cond_incl_expr_or(a, &tok);
            node            = (ast *)bin;
            continue;
        }

        break;
    }
    *tokp = tok;
    return node;
}

ast *
ast_cond_incl_expr_lor(allocator *a, token **tokp) {
    ast *node  = ast_cond_incl_expr_land(a, tokp);
    token *tok = *tokp;
    for (;;) {
        if (IS_PUNCT(tok, C_PUNCT_LOR)) {
            tok             = tok->next;
            ast_binary *bin = make_ast(a, AST_BIN, tok->loc);
            bin->bin_kind   = AST_BIN_LOR;
            bin->left       = node;
            bin->right      = ast_cond_incl_expr_land(a, &tok);
            node            = (ast *)bin;
            continue;
        }

        break;
    }
    *tokp = tok;
    return node;
}

ast *
ast_cond_incl_expr_ternary(allocator *a, token **tokp) {
    token *tok = *tokp;
    ast *node  = ast_cond_incl_expr_lor(a, &tok);
    if (IS_PUNCT(tok, '?')) {
        ast_ternary *ter = make_ast(a, AST_TER, tok->loc);
        ter->cond        = node;
        ter->cond_true   = ast_cond_incl_expr(a, &tok);
        if (IS_PUNCT(tok, ':')) {
            tok             = tok->next;
            ter->cond_false = ast_cond_incl_expr_ternary(a, &tok);
            node            = (ast *)ter;
        } else {
            NOT_IMPL;
        }
    }
    *tokp = tok;
    return node;
}

ast *
ast_cond_incl_expr(allocator *a, token **tokp) {
    ast *node  = ast_cond_incl_expr_ternary(a, tokp);
    token *tok = *tokp;
    if (IS_PUNCT(tok, ',')) {
        tok             = tok->next;
        ast_binary *bin = make_ast(a, AST_BIN, tok->loc);
        bin->bin_kind   = AST_BIN_COMMA;
        bin->left       = node;
        bin->right      = ast_cond_incl_expr(a, &tok);
        *tokp           = tok;
    }
    return node;
}

int64_t
ast_cond_incl_eval(ast *node) {
    int64_t result = 0;
    switch (node->kind) {
        INVALID_DEFAULT_CASE;
    case AST_TER: {
        ast_ternary *ter = (void *)node;

        result = ast_cond_incl_eval(ter->cond) ? ast_cond_incl_eval(ter->cond_true)
                                               : ast_cond_incl_eval(ter->cond_false);
    } break;
    case AST_BIN: {
        ast_binary *bin = (void *)node;

        int64_t left  = ast_cond_incl_eval(bin->left);
        int64_t right = ast_cond_incl_eval(bin->right);

        switch (bin->kind) {
            INVALID_DEFAULT_CASE;
        case AST_BIN_ADD:
            result = left + right;
            break;
        case AST_BIN_SUB:
            result = left - right;
            break;
        case AST_BIN_MUL:
            result = left * right;
            break;
        case AST_BIN_DIV:
            result = left / right;
            break;
        case AST_BIN_MOD:
            result = left % right;
            break;
        case AST_BIN_LE:
            result = left <= right;
            break;
        case AST_BIN_L:
            result = left < right;
            break;
        case AST_BIN_GE:
            result = left >= right;
            break;
        case AST_BIN_G:
            result = left > right;
            break;
        case AST_BIN_EQ:
            result = left == right;
            break;
        case AST_BIN_NEQ:
            result = left != right;
            break;
        case AST_BIN_AND:
            result = left & right;
            break;
        case AST_BIN_OR:
            result = left | right;
            break;
        case AST_BIN_XOR:
            result = left ^ right;
            break;
        case AST_BIN_LSHIFT:
            result = left << right;
            break;
        case AST_BIN_RSHIFT:
            result = left >> right;
            break;
        case AST_BIN_LAND:
            result = left && right;
            break;
        case AST_BIN_LOR:
            result = left || right;
            break;
        }
    } break;
    case AST_UN: {
        ast_unary *un = (void *)node;

        int64_t expr = ast_cond_incl_eval(un->expr);

        switch (un->kind) {
            INVALID_DEFAULT_CASE;
        case AST_UN_MINUS:
            result = -expr;
            break;
        case AST_UN_PLUS:
            result = expr;
            break;
        case AST_UN_LNOT:
            result = !expr;
            break;
        case AST_UN_NOT:
            result = ~expr;
            break;
        }
    } break;
    case AST_NUM: {
        ast_number *num = (void *)node;
        assert(c_type_kind_is_int(num->type->kind));
        result = (int64_t)num->uint_value;
    } break;
    }

    return result;
}
