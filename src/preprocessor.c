#include "preprocessor.h"

#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "allocator.h"
#include "ast.h"
#include "bump_allocator.h"
#include "c_lang.h"
#include "c_types.h"
#include "file_storage.h"
#include "hashing.h"
#include "llist.h"
#include "pp_lexer.h"
#include "str.h"

static pp_macro **
get_macro(preprocessor *pp, uint32_t hash) {
    pp_macro **macrop = hash_table_sc_get_u32(
        pp->macro_hash, sizeof(pp->macro_hash) / sizeof(*pp->macro_hash),
        pp_macro, next, name_hash, hash);
    return macrop;
}

static pp_macro *
get_new_macro(preprocessor *pp) {
    pp_macro *macro = pp->macro_freelist;
    if (macro) {
        pp->macro_freelist = macro->next;
        memset(macro, 0, sizeof(*macro));
    } else {
        macro = bump_alloc(pp->a, sizeof(*macro));
    }
    return macro;
}

static void
free_macro_data(preprocessor *pp, pp_macro *macro) {
    while (macro->args) {
        pp_macro_arg *arg      = macro->args;
        macro->args            = arg->next;
        arg->next              = pp->macro_arg_freelist;
        pp->macro_arg_freelist = arg;
    }
}

static pp_macro_arg *
get_new_macro_arg(preprocessor *pp) {
    pp_macro_arg *arg = pp->macro_arg_freelist;
    if (arg) {
        pp->macro_arg_freelist = arg->next;
        memset(arg, 0, sizeof(*arg));
    } else {
        arg = bump_alloc(pp->a, sizeof(*arg));
    }
    return arg;
}

static pp_token *
get_new_token(preprocessor *pp) {
    pp_token *tok = pp->tok_freelist;
    if (tok) {
        pp->tok_freelist = tok->next;
        memset(tok, 0, sizeof(*tok));
    } else {
        tok = bump_alloc(pp->a, sizeof(*tok));
    }
    return tok;
}

static pp_conditional_include *
get_new_cond_incl(preprocessor *pp) {
    pp_conditional_include *incl = pp->cond_incl_freelist;
    if (incl) {
        LLIST_POP(pp->cond_incl_freelist);
        memset(incl, 0, sizeof(*incl));
    } else {
        incl = bump_alloc(pp->a, sizeof(*incl));
    }
    return incl;
}

#if 0
static pp_parse_stack *
get_new_parse_stack_entry(preprocessor *pp) {
    pp_parse_stack *entry = pp->parse_stack_freelist;
    if (entry) {
        LLIST_POP(pp->parse_stack_freelist);
        memset(entry, 0, sizeof(*entry));
    } else {
        entry = bump_alloc(pp->a, sizeof(*entry));
    }
    return entry;
}
#endif

static pp_token *
copy_pp_token(preprocessor *pp, pp_token *tok) {
    pp_token *new = get_new_token(pp);
    *new          = *tok;
    new->next     = 0;
    return new;
}

static void
copy_pp_token_loc(pp_token *dst, pp_token *src) {
    dst->loc = src->loc;
}

static void
get_function_like_macro_arguments(preprocessor *pp, pp_token **tokp,
                                  pp_macro *macro) {
    pp_token *tok = *tokp;
    if (tok->kind != PP_TOK_PUNCT || tok->punct_kind != ')') {
        uint32_t arg_idx  = 0;
        pp_macro_arg *arg = macro->args;

        for (;;) {
            pp_token *first_arg_token = tok;
            uint32_t tok_count        = 0;
            uint32_t parens_depth     = 0;
            for (;;) {
                if (tok->kind == PP_TOK_PUNCT && tok->punct_kind == ')' &&
                    parens_depth == 0) {
                    break;
                } else if (tok->kind == PP_TOK_PUNCT &&
                           tok->punct_kind == ',' && parens_depth == 0) {
                    tok = tok->next;
                    break;
                } else if (tok->kind == PP_TOK_PUNCT &&
                           tok->punct_kind == '(') {
                    ++parens_depth;
                } else if (tok->kind == PP_TOK_PUNCT &&
                           tok->punct_kind == ')') {
                    assert(parens_depth);
                    --parens_depth;
                }
                ++tok_count;
                tok = tok->next;
            }

            // TODO: Variadic
            ++arg_idx;
            arg->toks      = first_arg_token;
            arg->tok_count = tok_count;
            arg            = arg->next;

            if (tok->kind == PP_TOK_PUNCT && tok->punct_kind == ')') {
                if (arg_idx != macro->arg_count) {
                    NOT_IMPL;
                }
                break;
            }
        }
    }
    *tokp = tok;
}

static void
expand_function_like_macro(preprocessor *pp, pp_token **tokp, pp_macro *macro,
                           pp_token *initial) {
    pp_token *tok = *tokp;

    get_function_like_macro_arguments(pp, &tok, macro);
    if (tok->kind == PP_TOK_PUNCT && tok->punct_kind == ')') {
        tok = tok->next;
    } else {
        NOT_IMPL;
    }

    uint32_t idx                = 0;
    linked_list_constructor def = {0};
    for (pp_token *temp = macro->definition; idx < macro->definition_len;
         temp           = temp->next, ++idx) {
        bool is_arg = false;
        // Check if given token is a macro
        if (temp->kind == PP_TOK_ID) {
            for (pp_macro_arg *arg = macro->args; arg && !is_arg;
                 arg               = arg->next) {
                if (string_eq(arg->name, temp->str)) {
                    uint32_t arg_tok_idx = 0;
                    for (pp_token *arg_tok = arg->toks;
                         arg_tok_idx < arg->tok_count;
                         arg_tok = arg_tok->next, ++arg_tok_idx) {
                        pp_token *new_token = copy_pp_token(pp, arg_tok);
                        copy_pp_token_loc(new_token, initial);
                        LLISTC_ADD_LAST(&def, new_token);
                    }
                    is_arg = true;
                }
            }
        }

        if (is_arg) {
            continue;
        }
        // If given token is not arg, continue as usual
        pp_token *new_token = copy_pp_token(pp, temp);
        copy_pp_token_loc(new_token, initial);
        LLISTC_ADD_LAST(&def, new_token);
    }
    if (def.first) {
        ((pp_token *)def.last)->next = tok;
        tok                          = def.first;
    }
    *tokp = tok;
}

static bool
expand_macro(preprocessor *pp, pp_token **tokp) {
    bool result     = false;
    pp_token *tok   = *tokp;
    pp_macro *macro = 0;
    if (tok->kind == PP_TOK_ID) {
        uint32_t name_hash = hash_string(tok->str);
        macro              = *get_macro(pp, name_hash);
    }

    if (macro) {
        switch (macro->kind) {
            INVALID_DEFAULT_CASE;
        case PP_MACRO_OBJ: {
            pp_token *initial = tok;
            tok               = tok->next;

            uint32_t idx                = 0;
            linked_list_constructor def = {0};
            for (pp_token *temp                    = macro->definition;
                 idx < macro->definition_len; temp = temp->next, ++idx) {
                pp_token *new_token = copy_pp_token(pp, temp);
                copy_pp_token_loc(new_token, initial);
                LLISTC_ADD_LAST(&def, new_token);
            }
            // If expansion is not empty
            if (def.first) {
                ((pp_token *)def.last)->next = tok;
                tok                          = def.first;
            }
            tok->has_whitespace = initial->has_whitespace;
            tok->at_line_start  = initial->at_line_start;
            result              = true;
        } break;
        case PP_MACRO_FUNC: {
            pp_token *initial = tok;
            tok               = tok->next;
            if (!tok->has_whitespace && tok->kind == PP_TOK_PUNCT &&
                tok->punct_kind == '(') {
                tok = tok->next;
                expand_function_like_macro(pp, &tok, macro, initial);
                tok->has_whitespace = initial->has_whitespace;
                tok->at_line_start  = initial->at_line_start;
                result              = true;
            } else {
                tok = initial;
            }
        } break;
        case PP_MACRO_FILE:
            NOT_IMPL;
            break;
        case PP_MACRO_LINE:
            NOT_IMPL;
            break;
        case PP_MACRO_COUNTER:
            NOT_IMPL;
            break;
        case PP_MACRO_TIMESTAMP:
            NOT_IMPL;
            break;
        case PP_MACRO_BASE_FILE:
            NOT_IMPL;
            break;
        case PP_MACRO_DATE:
            NOT_IMPL;
            break;
        case PP_MACRO_TIME:
            NOT_IMPL;
            break;
        }
    }
    *tokp = tok;
    return result;
}

static void
define_macro_function_like_args(preprocessor *pp, pp_token **tokp,
                                pp_macro *macro) {
    pp_token *tok = *tokp;

    linked_list_constructor args = {0};
    for (;;) {
        if (tok->at_line_start) {
            break;
        }

        // Eat argument
        if (tok->kind == PP_TOK_PUNCT &&
            tok->punct_kind == PP_TOK_PUNCT_VARARGS) {
            if (macro->is_variadic) {
                NOT_IMPL;
            }

            macro->is_variadic = true;
            pp_macro_arg *arg  = get_new_macro_arg(pp);
            arg->name          = WRAP_Z("__VA_ARGS__");
            LLISTC_ADD_LAST(&args, arg);

            tok = tok->next;
        } else if (tok->kind != PP_TOK_ID) {
            NOT_IMPL;
            break;
        } else {
            if (macro->is_variadic) {
                NOT_IMPL;
            }

            pp_macro_arg *arg = get_new_macro_arg(pp);
            arg->name         = tok->str;
            LLISTC_ADD_LAST(&args, arg);
            ++macro->arg_count;

            tok = tok->next;
        }

        // Parse post-argument
        if (!tok->at_line_start && tok->kind == PP_TOK_PUNCT &&
            tok->punct_kind == ',') {
            tok = tok->next;
        } else {
            break;
        }
    }

    if (tok->at_line_start || tok->kind != PP_TOK_PUNCT ||
        tok->punct_kind != ')') {
        NOT_IMPL;
    } else {
        macro->args = args.first;
    }

    *tokp = tok;
}

static void
define_macro(preprocessor *pp, pp_token **tokp) {
    pp_token *tok = *tokp;
    if (tok->kind != PP_TOK_ID) {
        NOT_IMPL;
    }

    // Get macro
    string macro_name        = tok->str;
    uint32_t macro_name_hash = hash_string(macro_name);
    pp_macro **macrop        = get_macro(pp, macro_name_hash);
    if (*macrop) {
        NOT_IMPL;
    } else {
        pp_macro *macro = get_new_macro(pp);
        LLIST_ADD(*macrop, macro);
    }

    pp_macro *macro = *macrop;
    assert(macro);

    macro->name      = macro_name;
    macro->name_hash = macro_name_hash;

    tok = tok->next;
    // Function-like macro
    if (tok->kind == PP_TOK_PUNCT && tok->punct_kind == '(' &&
        !tok->has_whitespace) {
        tok = tok->next;
        if (tok->kind != PP_TOK_PUNCT || tok->punct_kind != ')') {
            define_macro_function_like_args(pp, &tok, macro);
        }

        if (tok->kind == PP_TOK_PUNCT && tok->punct_kind == ')') {
            tok = tok->next;
        } else {
            NOT_IMPL;
        }
        macro->kind = PP_MACRO_FUNC;
    } else {
        macro->kind = PP_MACRO_OBJ;
    }

    // Store definition
    macro->definition = tok;
    while (!tok->at_line_start) {
        ++macro->definition_len;
        tok = tok->next;
    }

    *tokp = tok;
}

static void
undef_macro(preprocessor *pp, pp_token **tokp) {
    pp_token *tok = *tokp;
    if (tok->kind != PP_TOK_ID) {
        NOT_IMPL;
    }

    string macro_name        = tok->str;
    uint32_t macro_name_hash = hash_string(macro_name);
    pp_macro **macrop        = get_macro(pp, macro_name_hash);
    if (*macrop) {
        pp_macro *macro = *macrop;
        free_macro_data(pp, macro);
        *macrop = macro->next;
        LLIST_ADD(pp->macro_freelist, macro);
    } else {
        NOT_IMPL;
    }
    *tokp = tok;
}

static void
push_cond_incl(preprocessor *pp, bool is_included) {
    pp_conditional_include *incl = get_new_cond_incl(pp);
    incl->is_included            = is_included;
    LLIST_ADD(pp->cond_incl_stack, incl);
}

static void
skip_cond_incl(preprocessor *pp, pp_token **tokp) {
    uint32_t depth = 0;
    pp_token *tok  = *tokp;
    while (tok->kind != PP_TOK_EOF) {
        if (tok->kind != PP_TOK_PUNCT || tok->punct_kind != '#' ||
            !tok->at_line_start) {
            tok = tok->next;
            continue;
        }

        pp_token *init = tok;
        tok            = tok->next;
        if (tok->kind != PP_TOK_ID) {
            continue;
        }

        if (string_eq(tok->str, WRAP_Z("if")) ||
            string_eq(tok->str, WRAP_Z("ifdef")) ||
            string_eq(tok->str, WRAP_Z("ifndef"))) {
            ++depth;
        } else if (string_eq(tok->str, WRAP_Z("elif")) ||
                   string_eq(tok->str, WRAP_Z("else")) ||
                   string_eq(tok->str, WRAP_Z("endif"))) {
            if (!depth) {
                tok = init;
                break;
            }
            if (string_eq(tok->str, WRAP_Z("endif"))) {
                --depth;
            }
        }
    }

    if (depth) {
        NOT_IMPL;
    }
    *tokp = tok;
}

static linked_list_constructor
get_pp_tokens_for_file(preprocessor *pp, string filename) {
    linked_list_constructor tokens = {0};

    file *current_file = 0;
    if (pp->parse_stack) {
        current_file = pp->parse_stack->file;
    }
    file *f = get_file(pp->fs, filename, current_file);

    pp_lexer *lex = bump_alloc(pp->a, sizeof(pp_lexer));
    init_pp_lexer(lex, f->contents.data, STRING_END(f->contents),
                  pp->lexer_buffer, sizeof(pp->lexer_buffer));

    for (;;) {
        pp_token *tok = get_new_token(pp);
        pp_lexer_parse(lex, tok);
        tok->loc.filename = f->name;
        if (tok->str.data) {
            allocator a = bump_get_allocator(pp->a);
            tok->str    = string_dup(&a, tok->str);
        }

        LLISTC_ADD_LAST(&tokens, tok);
        if (tok->kind == PP_TOK_EOF) {
            break;
        }
    }
    return tokens;
}

static void
include_file(preprocessor *pp, pp_token **tokp, string filename) {
    linked_list_constructor tokens  = get_pp_tokens_for_file(pp, filename);
    pp_token *new_after             = *tokp;
    *tokp                           = tokens.first;
    ((pp_token *)tokens.last)->next = new_after;
}

static ast *expr_ternary(allocator *a, token **tokp);

static ast *
expr(allocator *a, token **tokp) {
    ast *node  = expr_ternary(a, tokp);
    token *tok = *tokp;
    if (IS_PUNCT(tok, ',')) {
        tok             = tok->next;
        ast_binary *bin = make_ast(a, AST_BIN);
        bin->bin_kind   = AST_BIN_COMMA;
        bin->left       = node;
        bin->right      = expr(a, &tok);
        *tokp           = tok;
    }
    return node;
}

static ast *
expr_primary(allocator *a, token **tokp) {
    ast *node  = 0;
    token *tok = *tokp;
    if (IS_PUNCT(tok, '(')) {
        node = expr(a, &tok);
        if (!IS_PUNCT(tok, ')')) {
            NOT_IMPL;
        } else {
            tok = tok->next;
        }
    } else if (tok->kind == TOK_NUM) {
        if (!c_type_is_int(tok->type->kind)) {
            NOT_IMPL;
        } else {
            ast_number *num = make_ast(a, AST_NUM);
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

static ast *
expr_unary(allocator *a, token **tokp) {
    ast *node = 0;
    token *tok = *tokp;
    if (IS_PUNCT(tok, '+')) {
        tok = tok->next;
        ast_unary *un = make_ast(a, AST_UN);
        un->un_kind = AST_UN_PLUS;
        un->expr = expr_primary(a, &tok);
        node = (ast *)un;
    } else if (IS_PUNCT(tok, '-')) {
        tok = tok->next;
        ast_unary *un = make_ast(a, AST_UN);
        un->un_kind = AST_UN_MINUS;
        un->expr = expr_primary(a, &tok);
        node = (ast *)un;
    } else if (IS_PUNCT(tok, '!')) {
        tok = tok->next;
        ast_unary *un = make_ast(a, AST_UN);
        un->un_kind = AST_UN_LNOT;
        un->expr = expr_primary(a, &tok);
        node = (ast *)un;
    } else if (IS_PUNCT(tok, '~')) {
        tok = tok->next;
        ast_unary *un = make_ast(a, AST_UN);
        un->un_kind = AST_UN_NOT;
        un->expr = expr_primary(a, &tok);
        node = (ast *)un;
    } else {
        node = expr_primary(a, tokp);
    }
    *tokp = tok;
    return node;
}

static ast *
expr_mul(allocator *a, token **tokp) {
    ast *node = expr_unary(a, tokp);
    token *tok = *tokp;
    for (;;) {
        if (IS_PUNCT(tok, '*')) {
            tok = tok->next;
            ast_binary *bin = make_ast(a, AST_BIN);
            bin->bin_kind = AST_BIN_MUL;
            bin->left = node;
            bin->right = expr_unary(a, &tok);
            node = (ast *)bin;
            continue;
        } else if (IS_PUNCT(tok, '/')) {
            tok = tok->next;
            ast_binary *bin = make_ast(a, AST_BIN);
            bin->bin_kind = AST_BIN_DIV;
            bin->left = node;
            bin->right = expr_unary(a, &tok);
            node = (ast *)bin;
            continue;
        } else if (IS_PUNCT(tok, '%')) {
            tok = tok->next;
            ast_binary *bin = make_ast(a, AST_BIN);
            bin->bin_kind = AST_BIN_MOD;
            bin->left = node;
            bin->right = expr_unary(a, &tok);
            node = (ast *)bin;
            continue;
        }

        break;
    }
    *tokp = tok;
    return node;
}

static ast *
expr_add(allocator *a, token **tokp) {
    ast *node = expr_mul(a, tokp);
    token *tok = *tokp;
    for (;;) {
        if (IS_PUNCT(tok, '+')) {
            tok = tok->next;
            ast_binary *bin = make_ast(a, AST_BIN);
            bin->bin_kind = AST_BIN_ADD;
            bin->left = node;
            bin->right = expr_mul(a, &tok);
            node = (ast *)bin;
            continue;
        } else if (IS_PUNCT(tok, '-')) {
            tok = tok->next;
            ast_binary *bin = make_ast(a, AST_BIN);
            bin->bin_kind = AST_BIN_SUB;
            bin->left = node;
            bin->right = expr_mul(a, &tok);
            node = (ast *)bin;
            continue;
        }

        break;
    }
    *tokp = tok;
    return node;
}

static ast *
expr_shift(allocator *a, token **tokp) {
    ast *node = expr_add(a, tokp);
    token *tok = *tokp;
    for (;;) {
        if (IS_PUNCT(tok, C_PUNCT_LSHIFT)) {
            tok = tok->next;
            ast_binary *bin = make_ast(a, AST_BIN);
            bin->bin_kind = AST_BIN_LSHIFT;
            bin->left = node;
            bin->right = expr_add(a, &tok);
            node = (ast *)bin;
            continue;
        } else if (IS_PUNCT(tok, C_PUNCT_RSHIFT)) {
            tok = tok->next;
            ast_binary *bin = make_ast(a, AST_BIN);
            bin->bin_kind = AST_BIN_RSHIFT;
            bin->left = node;
            bin->right = expr_add(a, &tok);
            node = (ast *)bin;
            continue;
        }

        break;
    }
    *tokp = tok;
    return node;
}

static ast *
expr_rel(allocator *a, token **tokp) {
    ast *node = expr_shift(a, tokp);
    token *tok = *tokp;
    for (;;) {
        if (IS_PUNCT(tok, '<')) {
            tok = tok->next;
            ast_binary *bin = make_ast(a, AST_BIN);
            bin->bin_kind = AST_BIN_L;
            bin->left = node;
            bin->right = expr_shift(a, &tok);
            node = (ast *)bin;
            continue;
        } else if (IS_PUNCT(tok, '>')) {
            tok = tok->next;
            ast_binary *bin = make_ast(a, AST_BIN);
            bin->bin_kind = AST_BIN_G;
            bin->left = node;
            bin->right = expr_shift(a, &tok);
            node = (ast *)bin;
            continue;
        } else if (IS_PUNCT(tok, C_PUNCT_LEQ)) {
            tok = tok->next;
            ast_binary *bin = make_ast(a, AST_BIN);
            bin->bin_kind = AST_BIN_LE;
            bin->left = node;
            bin->right = expr_shift(a, &tok);
            node = (ast *)bin;
            continue;
        } else if (IS_PUNCT(tok, C_PUNCT_GEQ)) {
            tok = tok->next;
            ast_binary *bin = make_ast(a, AST_BIN);
            bin->bin_kind = AST_BIN_GE;
            bin->left = node;
            bin->right = expr_shift(a, &tok);
            node = (ast *)bin;
            continue;
        }

        break;
    }
    *tokp = tok;
    return node;
}

static ast *
expr_eq(allocator *a, token **tokp) {
    ast *node = expr_rel(a, tokp);
    token *tok = *tokp;
    for (;;) {
        if (IS_PUNCT(tok, C_PUNCT_EQ)) {
            tok = tok->next;
            ast_binary *bin = make_ast(a, AST_BIN);
            bin->bin_kind = AST_BIN_EQ;
            bin->left = node;
            bin->right = expr_rel(a, &tok);
            node = (ast *)bin;
            continue;
        } else if (IS_PUNCT(tok, C_PUNCT_NEQ)) {
            tok = tok->next;
            ast_binary *bin = make_ast(a, AST_BIN);
            bin->bin_kind = AST_BIN_NEQ;
            bin->left = node;
            bin->right = expr_rel(a, &tok);
            node = (ast *)bin;
            continue;
        }

        break;
    }
    *tokp = tok;
    return node;
}

static ast *
expr_and(allocator *a, token **tokp) {
    ast *node = expr_eq(a, tokp);
    token *tok = *tokp;
    for (;;) {
        if (IS_PUNCT(tok, '&')) {
            tok = tok->next;
            ast_binary *bin = make_ast(a, AST_BIN);
            bin->bin_kind = AST_BIN_AND;
            bin->left = node;
            bin->right = expr_eq(a, &tok);
            node = (ast *)bin;
            continue;
        }

        break;
    }
    *tokp = tok;
    return node;
}

static ast *
expr_xor(allocator *a, token **tokp) {
    ast *node = expr_eq(a, tokp);
    token *tok = *tokp;
    for (;;) {
        if (IS_PUNCT(tok, '^')) {
            tok = tok->next;
            ast_binary *bin = make_ast(a, AST_BIN);
            bin->bin_kind = AST_BIN_XOR;
            bin->left = node;
            bin->right = expr_eq(a, &tok);
            node = (ast *)bin;
            continue;
        }

        break;
    }
    *tokp = tok;
    return node;
}

static ast *
expr_or(allocator *a, token **tokp) {
    ast *node = expr_xor(a, tokp);
    token *tok = *tokp;
    for (;;) {
        if (IS_PUNCT(tok, '|')) {
            tok = tok->next;
            ast_binary *bin = make_ast(a, AST_BIN);
            bin->bin_kind = AST_BIN_OR;
            bin->left = node;
            bin->right = expr_xor(a, &tok);
            node = (ast *)bin;
            continue;
        }

        break;
    }
    *tokp = tok;
    return node;
}

static ast *
expr_land(allocator *a, token **tokp) {
    ast *node = expr_or(a, tokp);
    token *tok = *tokp;
    for (;;) {
        if (IS_PUNCT(tok, C_PUNCT_LAND)) {
            tok = tok->next;
            ast_binary *bin = make_ast(a, AST_BIN);
            bin->bin_kind = AST_BIN_LAND;
            bin->left = node;
            bin->right = expr_or(a, &tok);
            node = (ast *)bin;
            continue;
        }

        break;
    }
    *tokp = tok;
    return node;
}

static ast *
expr_lor(allocator *a, token **tokp) {
    ast *node = expr_land(a, tokp);
    token *tok = *tokp;
    for (;;) {
        if (IS_PUNCT(tok, C_PUNCT_LOR)) {
            tok = tok->next;
            ast_binary *bin = make_ast(a, AST_BIN);
            bin->bin_kind = AST_BIN_LOR;
            bin->left = node;
            bin->right = expr_land(a, &tok);
            node = (ast *)bin;
            continue;
        }

        break;
    }
    *tokp = tok;
    return node;
}

static ast *
expr_ternary(allocator *a, token **tokp) {
    token *tok = *tokp;
    ast *node  = expr_lor(a, &tok);
    if (IS_PUNCT(tok, '?')) {
        ast_ternary *ter = make_ast(a, AST_TER);
        ter->cond        = node;
        ter->cond_true   = expr(a, &tok);
        if (IS_PUNCT(tok, ';')) {
            tok             = tok->next;
            ter->cond_false = expr_ternary(a, &tok);
            node            = (ast *)ter;
        } else {
            NOT_IMPL;
        }
    }
    *tokp = tok;
    return node;
}

static int64_t
eval_pp_expr(preprocessor *pp, pp_token **tokp) {
    int64_t result = 0;

    // Copy all tokens from current line so we can process them independently
    linked_list_constructor copied = {0};
    pp_token *tok                  = *tokp;
    while (!tok->at_line_start) {
        pp_token *new_tok = copy_pp_token(pp, tok);
        tok               = tok->next;
        LLISTC_ADD_LAST(&copied, new_tok);
    }
    *tokp = tok;

    // Replace 'defined(_id)' and 'defined _id' sequences.
    tok                              = copied.first;
    linked_list_constructor modified = {0};
    while (tok) {
        if (tok->kind == PP_TOK_ID && string_eq(tok->str, WRAP_Z("defined"))) {
            bool has_paren = false;
            tok            = tok->next;
            if (tok->kind == PP_TOK_PUNCT && tok->punct_kind == '(') {
                tok       = tok->next;
                has_paren = true;
            }

            if (tok->kind != PP_TOK_ID) {
                NOT_IMPL;
            }

            // TODO: Clean this mess

            uint32_t macro_name_hash = hash_string(tok->str);
            pp_macro *macro          = *get_macro(pp, macro_name_hash);
            uint32_t token_value     = (macro != 0);
            char value_buf[2]        = {0};
            value_buf[0]             = token_value ? '1' : '0';

            allocator a         = bump_get_allocator(pp->a);
            pp_token *new_token = get_new_token(pp);
            new_token->kind     = PP_TOK_NUM;
            new_token->str      = string_memdup(&a, value_buf);

            LLISTC_ADD_LAST(&modified, new_token);

            tok = tok->next;
            if (has_paren) {
                if (tok->kind != PP_TOK_PUNCT || tok->punct_kind != ')') {
                    NOT_IMPL;
                }
                tok = tok->next;
            }
        } else {
            LLISTC_ADD_LAST(&modified, tok);
        }
    }

    // Now we can preprocess the expression
    tok = modified.first;
    while (tok) {
        if (expand_macro(pp, &tok)) {
            continue;
        }
        tok = tok->next;
    }

    // Now we have to replace all nonexistent identifiers with 0.
    tok = modified.first;
    while (tok) {
        if (tok->kind == PP_TOK_ID && !*get_macro(pp, hash_string(tok->str))) {
            allocator a      = bump_get_allocator(pp->a);
            char value_buf[] = "0";

            pp_token *new_token = get_new_token(pp);
            new_token->kind     = PP_TOK_NUM;
            new_token->str      = string_memdup(&a, value_buf);
            new_token->next     = tok->next;
            *tok                = *new_token;
        }
        tok = tok->next;
    }

    // So now in tok we have a list of tokens that form constant expression.
    // Convert these tokens to C ones.
    tok                               = modified.first;
    linked_list_constructor converted = {0};
    while (tok) {
        allocator a  = bump_get_allocator(pp->a);
        token *c_tok = aalloc(&a, sizeof(token));
        if (!convert_pp_token(tok, c_tok, &a)) {
            NOT_IMPL;
        }
        LLISTC_ADD_LAST(&converted, c_tok);
        tok = tok->next;
    }

    return result;
}

static bool
process_pp_directive(preprocessor *pp, pp_token **tokp) {
    bool result   = false;
    pp_token *tok = *tokp;
    if (tok->kind == PP_TOK_PUNCT && tok->punct_kind == '#' &&
        tok->at_line_start) {
        tok = tok->next;
        if (tok->kind == PP_TOK_ID) {
            if (string_eq(tok->str, WRAP_Z("define"))) {
                tok = tok->next;
                define_macro(pp, &tok);
            } else if (string_eq(tok->str, WRAP_Z("undef"))) {
                tok = tok->next;
                undef_macro(pp, &tok);
            } else if (string_eq(tok->str, WRAP_Z("if"))) {
                tok                 = tok->next;
                int64_t expr_result = eval_pp_expr(pp, &tok);
                push_cond_incl(pp, expr_result != 0);
                if (!expr_result) {
                    skip_cond_incl(pp, &tok);
                }
            } else if (string_eq(tok->str, WRAP_Z("elif"))) {
                tok                          = tok->next;
                pp_conditional_include *incl = pp->cond_incl_stack;
                if (!incl) {
                    NOT_IMPL;
                } else {
                    if (incl->is_after_else) {
                        NOT_IMPL;
                    }

                    if (!incl->is_included) {
                        int64_t expr_result = eval_pp_expr(pp, &tok);
                        if (expr_result) {
                            incl->is_included = true;
                        } else {
                            skip_cond_incl(pp, &tok);
                        }
                    } else {
                        skip_cond_incl(pp, &tok);
                    }
                }
            } else if (string_eq(tok->str, WRAP_Z("else"))) {
                pp_conditional_include *incl = pp->cond_incl_stack;
                if (!incl) {
                    NOT_IMPL;
                } else {
                    if (incl->is_after_else) {
                        NOT_IMPL;
                    }
                    incl->is_after_else = true;
                    if (incl->is_included) {
                        skip_cond_incl(pp, &tok);
                    }
                }
            } else if (string_eq(tok->str, WRAP_Z("endif"))) {
                tok                          = tok->next;
                pp_conditional_include *incl = pp->cond_incl_stack;
                if (!incl) {
                    NOT_IMPL;
                } else {
                    LLIST_POP(pp->cond_incl_stack);
                    LLIST_ADD(pp->cond_incl_freelist, incl);
                }
            } else if (string_eq(tok->str, WRAP_Z("ifdef"))) {
                tok = tok->next;
                if (tok->kind != PP_TOK_ID) {
                    NOT_IMPL;
                }
                uint32_t macro_name_hash = hash_string(tok->str);
                bool is_defined          = *get_macro(pp, macro_name_hash) != 0;
                push_cond_incl(pp, is_defined);
                if (!is_defined) {
                    skip_cond_incl(pp, &tok);
                }
            } else if (string_eq(tok->str, WRAP_Z("ifndef"))) {
                tok = tok->next;
                if (tok->kind != PP_TOK_ID) {
                    NOT_IMPL;
                }
                uint32_t macro_name_hash = hash_string(tok->str);
                bool is_defined          = *get_macro(pp, macro_name_hash) != 0;
                push_cond_incl(pp, !is_defined);
                if (is_defined) {
                    skip_cond_incl(pp, &tok);
                }
            } else if (string_eq(tok->str, WRAP_Z("line"))) {
                NOT_IMPL;
            } else if (string_eq(tok->str, WRAP_Z("pragma"))) {
                NOT_IMPL;
            } else if (string_eq(tok->str, WRAP_Z("error"))) {
                NOT_IMPL;
            } else if (string_eq(tok->str, WRAP_Z("include"))) {
                tok = tok->next;
                if (tok->at_line_start) {
                    NOT_IMPL;
                }

                if (tok->kind == PP_TOK_PUNCT && tok->punct_kind == '<') {
                    tok = tok->next;
                    char filename_buffer[4096];
                    char *buf_eof = filename_buffer + sizeof(filename_buffer);
                    char *cursor  = filename_buffer;
                    while (
                        (tok->kind != PP_TOK_PUNCT || tok->punct_kind != '>') &&
                        !tok->at_line_start) {
                        cursor += fmt_pp_tok(cursor, buf_eof - cursor, tok);
                        tok = tok->next;
                    }

                    if (tok->kind != PP_TOK_PUNCT || tok->punct_kind != '>') {
                        NOT_IMPL;
                    } else {
                        tok = tok->next;
                    }

                    string filename =
                        string(filename_buffer, cursor - filename_buffer);
                    include_file(pp, &tok, filename);
                } else if (tok->kind == PP_TOK_STR) {
                    string filename = tok->str;
                    tok             = tok->next;
                    include_file(pp, &tok, filename);
                } else {
                    NOT_IMPL;
                }
            } else {
                NOT_IMPL;
            }
        } else {
            NOT_IMPL;
        }

        while (!tok->at_line_start) {
            tok = tok->next;
        }
        result = true;
    }
    *tokp = tok;
    return result;
}

token *
do_pp(preprocessor *pp, string filename) {
    pp_token *tok = get_pp_tokens_for_file(pp, filename).first;
    linked_list_constructor converted = {0};
    while (tok) {
        if (expand_macro(pp, &tok)) {
            continue;
        }

        if (process_pp_directive(pp, &tok)) {
            continue;
        }

        token *c_tok = aalloc(pp->ea, sizeof(token));
        if (!convert_pp_token(tok, c_tok, pp->ea)) {
            NOT_IMPL;
        }
        LLISTC_ADD_LAST(&converted, c_tok);
        tok = tok->next;
#if HOLOC_DEBUG
        {
            char buffer[4096];
            uint32_t len     = fmt_token_verbose(buffer, sizeof(buffer), c_tok);
            char *debug_info = aalloc(get_debug_allocator(), len + 1);
            memcpy(debug_info, buffer, len + 1);
            c_tok->_debug_info = debug_info;
        }
#endif
    }
    return (token *)converted.first;
}
