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

static pp_token *
copy_pp_token(preprocessor *pp, pp_token *tok) {
    pp_token *new = bump_alloc(pp->a, sizeof(tok));
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
    if (macro->arg_count == 0 && !macro->is_variadic &&
        (tok->kind != PP_TOK_PUNCT || tok->punct_kind != ')')) {
        NOT_IMPL;
    } else {
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
                    if (!macro->is_variadic) {
                        break;
                    }
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
                if (arg_idx != macro->arg_count && !macro->is_variadic) {
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
    file *f = get_file(pp->fs, filename, current_file);

    char lexer_buffer[4096];

    pp_lexer *lex = bump_alloc(pp->a, sizeof(pp_lexer));
    init_pp_lexer(lex, f->contents.data, STRING_END(f->contents),
                  lexer_buffer, sizeof(lexer_buffer));

    for (;;) {
        pp_token *tok = bump_alloc(pp->a, sizeof(pp_token));
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

static int64_t
evaluate_constant_expression(ast *node) {
    int64_t result = 0;
    switch (node->kind) {
        INVALID_DEFAULT_CASE;
    case AST_TER: {
        ast_ternary *ter = (void *)node;

        result = evaluate_constant_expression(ter->cond)
                     ? evaluate_constant_expression(ter->cond_true)
                     : evaluate_constant_expression(ter->cond_false);
    } break;
    case AST_BIN: {
        ast_binary *bin = (void *)node;

        int64_t left  = evaluate_constant_expression(bin->left);
        int64_t right = evaluate_constant_expression(bin->right);

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

        int64_t expr = evaluate_constant_expression(un->expr);

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
        assert(c_type_is_int(num->type->kind));
        result = (int64_t)num->uint_value;
    } break;
    }

    return result;
}

static int64_t
eval_pp_expr(preprocessor *pp, pp_token **tokp) {
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
            pp_token *new_token = bump_alloc(pp->a, sizeof(pp_token));
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
            tok = tok->next;
        }
    }

    // So now in tok we have a list of tokens that form constant expression.
    // Convert these tokens to C ones.
    tok                               = modified.first;
    linked_list_constructor converted = {0};
    while (tok) {
        if (expand_macro(pp, &tok)) {
            continue;
        }

        if (tok->kind == PP_TOK_ID) {
            pp_macro *macro = *get_macro(pp, hash_string(tok->str));
            if (!macro) {
                allocator a      = bump_get_allocator(pp->a);
                char value_buf[] = "0";

                tok->kind = PP_TOK_NUM;
                tok->str  = string_memdup(&a, value_buf);
            }
        }

        allocator a  = bump_get_allocator(pp->a);
        token *c_tok = aalloc(&a, sizeof(token));
        if (!convert_pp_token(tok, c_tok, &a)) {
            NOT_IMPL;
        }
        LLISTC_ADD_LAST(&converted, c_tok);
        tok = tok->next;

        if (!tok) {
            c_tok       = aalloc(&a, sizeof(token));
            c_tok->kind = TOK_EOF;
            LLISTC_ADD_LAST(&converted, c_tok);
        }
    }

    allocator a        = bump_get_allocator(pp->a);
    token *expr_tokens = converted.first;
    ast *expr_ast      = ast_cond_incl_expr_ternary(&a, &expr_tokens);
    int64_t result     = evaluate_constant_expression(expr_ast);

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
                // TODO:
            } else if (string_eq(tok->str, WRAP_Z("pragma"))) {
                NOT_IMPL;
            } else if (string_eq(tok->str, WRAP_Z("error"))) {
                // TODO:
                /* NOT_IMPL; */
            } else if (string_eq(tok->str, WRAP_Z("warning"))) {
                // TODO:
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

void
init_pp(preprocessor *pp, string filename) {
    pp->toks = get_pp_tokens_for_file(pp, filename).first;
}

bool
pp_parse(preprocessor *pp, struct token *tok) {
    bool result = false;
    while (pp->toks) {
        if (expand_macro(pp, &pp->toks)) {
            continue;
        }

        if (process_pp_directive(pp, &pp->toks)) {
            continue;
        }

        if (!convert_pp_token(pp->toks, tok, pp->ea)) {
            NOT_IMPL;
        }
#if HOLOC_DEBUG
        {
            char buffer[4096];
            uint32_t len     = fmt_token_verbose(buffer, sizeof(buffer), tok);
            char *debug_info = aalloc(get_debug_allocator(), len + 1);
            memcpy(debug_info, buffer, len + 1);
            tok->_debug_info = debug_info;
        }
#endif
        result = tok->kind != TOK_EOF;
        break;
    }

    // Make sure to always return EOF after stream end
    if (!pp->toks && !result) {
        tok->kind = TOK_EOF;
    }
    return result;
}

