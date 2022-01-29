#include "preprocessor.h"

#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "allocator.h"
#include "ast.h"
#include "buffer_writer.h"
#include "c_lang.h"
#include "c_types.h"
#include "file_storage.h"
#include "filepath.h"
#include "hashing.h"
#include "llist.h"
#include "pp_lexer.h"
#include "str.h"

// NOTE: These can be separate functions, but since they are really wrappers for
// another function let's not make them that way and keep as macros.
#define GET_MACROP(_pp, _hash)                                        \
    (pp_macro **)hash_table_sc_get_u32((_pp)->macro_hash,             \
                                       ARRAY_SIZE((_pp)->macro_hash), \
                                       pp_macro, next, name_hash, (_hash))
#define GET_MACRO(_pp, _hash) (*GET_MACROP(_pp, _hash))

#define FREELIST_ALLOC_(_fl, _a)                                \
    freelist_alloc_impl((void **)&(_fl),                        \
                        ((char *)&(_fl)->next - (char *)(_fl)), \
                        sizeof(*(_fl)), (_a))
#define FREELIST_ALLOC(_pp, _fl_name) FREELIST_ALLOC_((_pp)->_fl_name, (_pp)->a)

static void *
freelist_alloc_impl(void **flp, uintptr_t next_offset, uintptr_t size,
                    allocator *a) {
    void *result = *flp;
    if (!result) {
        result = aalloc(a, size);
    } else {
        *flp = *(void **)((char *)result + next_offset);
        memset(result, 0, size);
    }
    return result;
}

#define NEW_MACRO_ARG(_pp) FREELIST_ALLOC(_pp, macro_arg_freelist)
#define NEW_MACRO(_pp) FREELIST_ALLOC(_pp, macro_freelist)
#define NEW_COND_INCL(_pp) FREELIST_ALLOC(_pp, cond_incl_freelist)

#define NEW_PP_TOKEN(_pp) aalloc((_pp)->a, sizeof(pp_token))

// Frees macro arguments and adds them to freelist.
// TODO: Free tokens too.
static void
free_macro_data(preprocessor *pp, pp_macro *macro) {
    while (macro->args) {
        pp_macro_arg *arg      = macro->args;
        macro->args            = arg->next;
        arg->next              = pp->macro_arg_freelist;
        pp->macro_arg_freelist = arg;
    }
}

// Returns new token and writes memory contained in given to it, effectively
// making a copy.
static pp_token *
copy_pp_token(preprocessor *pp, pp_token *tok) {
    pp_token *new = NEW_PP_TOKEN(pp);
    memcpy(new, tok, sizeof(pp_token));
    new->next = 0;
    return new;
}

static void
copy_pp_token_loc(pp_token *dst, pp_token *src) {
    dst->loc = src->loc;
}

// Function used when #define'ing a function-like macro. Parses given token list
// and forms arguments, that are written to the given macro.
// Doesn't eat closing paren.
static void
get_function_like_macro_arguments(preprocessor *pp, pp_token **tokp,
                                  pp_macro *macro) {
    pp_token *tok = *tokp;
    // If next token is closing parens, don't collect arguments.
    if (macro->arg_count == 0 && !macro->is_variadic &&
        !PP_TOK_IS_PUNCT(tok, ')')) {
        NOT_IMPL;
    } else {
        // Collect arguments.
        // arg_idx needed for variadic argument checking and validating number
        // of arguments.
        uint32_t arg_idx  = 0;
        pp_macro_arg *arg = macro->args;

        for (;;) {
            // Commans inclosed in parens are not treated as argument
            // separators.
            uint32_t parens_depth = 0;
            // Construct list of copied tokens that form tokens of argument.
            // Varidic arguments are formed in a hacky way - they all are
            // threated as single argument
            linked_list_constructor macro_tokens = {0};
            for (;;) {
                if (PP_TOK_IS_PUNCT(tok, ')') && parens_depth == 0) {
                    break;
                } else if (PP_TOK_IS_PUNCT(tok, ',') && parens_depth == 0) {
                    tok = tok->next;
                    if (!macro->is_variadic) {
                        break;
                    }
                } else if (PP_TOK_IS_PUNCT(tok, '(')) {
                    ++parens_depth;
                } else if (PP_TOK_IS_PUNCT(tok, ')')) {
                    assert(parens_depth);
                    --parens_depth;
                }
                pp_token *new_token = copy_pp_token(pp, tok);
                LLISTC_ADD_LAST(&macro_tokens, new_token);
                tok = tok->next;
            }
            // Add eof to the end
            pp_token *eof = NEW_PP_TOKEN(pp);
            eof->kind     = PP_TOK_EOF;
            LLISTC_ADD_LAST(&macro_tokens, eof);

            arg->toks = macro_tokens.first;

            ++arg_idx;
            arg = arg->next;
            // Check if we are finished
            if (PP_TOK_IS_PUNCT(tok, ')')) {
                if (arg_idx != macro->arg_count && !macro->is_variadic) {
                    NOT_IMPL;
                }
                break;
            }
        }
    }

    *tokp = tok;
}

// Used internally in expand_function_like_macro.
// Finds argument by name in macro argument list.
static pp_macro_arg *
get_argument(pp_macro *macro, string name) {
    pp_macro_arg *arg = 0;
    for (pp_macro_arg *test = macro->args; test; test = test->next) {
        if (string_eq(test->name, name)) {
            arg = test;
            break;
        }
    }
    return arg;
}

// Parses given function-like macro invocation. Arguments are stored in
// macro->args, with their values of invocation. This is possible because
// recursive macro invocation is not supported, and tokens of given macro
// invocation won't be changed doring it.
// Sets locations of new tokens to be the same as 'initial' param.
static void
expand_function_like_macro(preprocessor *pp, pp_token **tokp, pp_macro *macro,
                           pp_token *initial) {
    pp_token *tok = *tokp;

    // Collect macro arguments.
    get_function_like_macro_arguments(pp, &tok, macro);
    if (!PP_TOK_IS_PUNCT(tok, ')')) {
        NOT_IMPL;
    } else {
        tok = tok->next;
    }

    linked_list_constructor def = {0};
    for (pp_token *temp = macro->definition; temp->kind != PP_TOK_EOF;
         temp           = temp->next) {
        // Check if given token is a macro argument
        if (temp->kind == PP_TOK_ID) {
            pp_macro_arg *arg = get_argument(macro, temp->str);
            if (arg) {
                for (pp_token *arg_tok = arg->toks; arg_tok->kind != PP_TOK_EOF;
                     arg_tok           = arg_tok->next) {
                    pp_token *new_token = copy_pp_token(pp, arg_tok);
                    copy_pp_token_loc(new_token, initial);
                    LLISTC_ADD_LAST(&def, new_token);
                }
                continue;
            }
        } else if (PP_TOK_IS_PUNCT(temp, '#')) {
            temp              = temp->next;
            pp_macro_arg *arg = get_argument(macro, temp->str);
            if (!arg) {
                NOT_IMPL;
                continue;
            }

            char buffer[4096];
            buffer_writer w = {buffer, buffer + sizeof(buffer)};
            for (pp_token *arg_tok = arg->toks; arg_tok->kind != PP_TOK_EOF;
                 arg_tok           = arg_tok->next) {
                fmt_pp_tokw(&w, arg_tok);
            }
            pp_token *new_token = aalloc(pp->a, sizeof(pp_token));
            new_token->kind     = PP_TOK_STR;
            new_token->str      = string_strdup(pp->a, buffer);
            new_token->str_kind = PP_TOK_STR_SCHAR;
            LLISTC_ADD_LAST(&def, new_token);
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
        macro              = GET_MACRO(pp, name_hash);
    }

    if (macro) {
        switch (macro->kind) {
            INVALID_DEFAULT_CASE;
        case PP_MACRO_OBJ: {
            pp_token *initial = tok;
            tok               = tok->next;

            linked_list_constructor def = {0};
            for (pp_token *temp = macro->definition; temp->kind != PP_TOK_EOF;
                 temp           = temp->next) {
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
            if (!tok->has_whitespace && PP_TOK_IS_PUNCT(tok, '(')) {
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
        case PP_MACRO_INCLUDE_LEVEL:
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
        if (PP_TOK_IS_PUNCT(tok, PP_TOK_PUNCT_VARARGS)) {
            if (macro->is_variadic) {
                NOT_IMPL;
            }

            macro->is_variadic = true;
            pp_macro_arg *arg  = NEW_MACRO_ARG(pp);
            arg->name          = WRAPZ("__VA_ARGS__");
            LLISTC_ADD_LAST(&args, arg);

            tok = tok->next;
        } else if (tok->kind != PP_TOK_ID) {
            NOT_IMPL;
            break;
        } else {
            if (macro->is_variadic) {
                NOT_IMPL;
            }

            pp_macro_arg *arg = NEW_MACRO_ARG(pp);
            arg->name         = tok->str;
            LLISTC_ADD_LAST(&args, arg);
            ++macro->arg_count;

            tok = tok->next;
        }

        // Parse post-argument
        if (!tok->at_line_start && PP_TOK_IS_PUNCT(tok, ',')) {
            tok = tok->next;
        } else {
            break;
        }
    }

    if (tok->at_line_start || !PP_TOK_IS_PUNCT(tok, ')')) {
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
    pp_macro **macrop        = GET_MACROP(pp, macro_name_hash);
    if (*macrop) {
        NOT_IMPL;
    } else {
        pp_macro *macro = NEW_MACRO(pp);
        LLIST_ADD(*macrop, macro);
    }

    pp_macro *macro = *macrop;
    assert(macro);

    macro->name      = macro_name;
    macro->name_hash = macro_name_hash;

    tok = tok->next;
    // Function-like macro
    if (PP_TOK_IS_PUNCT(tok, '(') && !tok->has_whitespace) {
        tok = tok->next;
        if (!PP_TOK_IS_PUNCT(tok, ')')) {
            define_macro_function_like_args(pp, &tok, macro);
        }

        if (PP_TOK_IS_PUNCT(tok, ')')) {
            tok = tok->next;
        } else {
            NOT_IMPL;
        }
        macro->kind = PP_MACRO_FUNC;
    } else {
        macro->kind = PP_MACRO_OBJ;
    }

    // Store definition
    linked_list_constructor def = {0};
    while (!tok->at_line_start) {
        pp_token *new_token = copy_pp_token(pp, tok);
        LLISTC_ADD_LAST(&def, new_token);
        tok = tok->next;
    }
    pp_token *eof = NEW_PP_TOKEN(pp);
    eof->kind     = PP_TOK_EOF;
    LLISTC_ADD_LAST(&def, eof);
    macro->definition = def.first;

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
    pp_macro **macrop        = GET_MACROP(pp, macro_name_hash);
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
    pp_conditional_include *incl = NEW_COND_INCL(pp);
    incl->is_included            = is_included;
    LLIST_ADD(pp->cond_incl_stack, incl);
}

static void
skip_cond_incl(preprocessor *pp, pp_token **tokp) {
    uint32_t depth = 0;
    pp_token *tok  = *tokp;
    while (tok->kind != PP_TOK_EOF) {
        if (!PP_TOK_IS_PUNCT(tok, '#') || !tok->at_line_start) {
            tok = tok->next;
            continue;
        }

        pp_token *init = tok;
        tok            = tok->next;
        if (tok->kind != PP_TOK_ID) {
            continue;
        }

        if (string_eq(tok->str, WRAPZ("if")) ||
            string_eq(tok->str, WRAPZ("ifdef")) ||
            string_eq(tok->str, WRAPZ("ifndef"))) {
            ++depth;
        } else if (string_eq(tok->str, WRAPZ("elif")) ||
                   string_eq(tok->str, WRAPZ("else")) ||
                   string_eq(tok->str, WRAPZ("endif"))) {
            if (!depth) {
                tok = init;
                break;
            }
            if (string_eq(tok->str, WRAPZ("endif"))) {
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
    file *f            = get_file(pp->fs, filename, current_file);

    char lexer_buffer[4096];
    pp_lexer lex = {0};
    init_pp_lexer(&lex, f->contents.data, STRING_END(f->contents), lexer_buffer,
                  sizeof(lexer_buffer));

    for (;;) {
        pp_token tok = {0};
        if (!pp_lexer_parse(&lex, &tok)) {
            break;
        }

        pp_token *entry = NEW_PP_TOKEN(pp);
        memcpy(entry, &tok, sizeof(pp_token));
        entry->loc.filename = f->name;
#if HOLOC_DEBUG
        {
            char buffer[4096];
            uint32_t len = fmt_pp_tok_verbose(buffer, sizeof(buffer), entry);
            char *debug_info = aalloc(get_debug_allocator(), len + 1);
            memcpy(debug_info, buffer, len + 1);
            entry->_debug_info = debug_info;
        }
#endif
        if (entry->str.data) {
            entry->str = string_dup(pp->a, entry->str);
        }

        LLISTC_ADD_LAST(&tokens, entry);
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
    // Copy all tokens from current line so we can process them
    // independently
    linked_list_constructor copied = {0};
    pp_token *tok                  = *tokp;
    while (!tok->at_line_start) {
        pp_token *new_tok = copy_pp_token(pp, tok);
        LLISTC_ADD_LAST(&copied, new_tok);
        char buffer[4096];
        fmt_pp_tok(buffer, sizeof(buffer), new_tok);
        tok = tok->next;
    }
    *tokp = tok;

    // Replace 'defined(_id)' and 'defined _id' sequences.
    tok                              = copied.first;
    linked_list_constructor modified = {0};
    while (tok) {
        if (tok->kind == PP_TOK_ID && string_eq(tok->str, WRAPZ("defined"))) {
            bool has_paren = false;
            tok            = tok->next;
            if (PP_TOK_IS_PUNCT(tok, '(')) {
                tok       = tok->next;
                has_paren = true;
            }

            if (tok->kind != PP_TOK_ID) {
                NOT_IMPL;
            }

            uint32_t macro_name_hash = hash_string(tok->str);
            pp_macro *macro          = GET_MACRO(pp, macro_name_hash);

            string value = (macro != 0) ? WRAPZ("1") : WRAPZ("0");

            pp_token *new_token = NEW_PP_TOKEN(pp);
            new_token->kind     = PP_TOK_NUM;
            new_token->str      = value;

            LLISTC_ADD_LAST(&modified, new_token);

            tok = tok->next;
            if (has_paren) {
                if (!PP_TOK_IS_PUNCT(tok, ')')) {
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
            pp_macro *macro = GET_MACRO(pp, hash_string(tok->str));
            if (!macro) {
                tok->kind = PP_TOK_NUM;
                tok->str  = WRAPZ("0");
            }
        }

        token *c_tok = aalloc(pp->a, sizeof(token));
        if (!convert_pp_token(tok, c_tok, pp->a)) {
            NOT_IMPL;
        }
        LLISTC_ADD_LAST(&converted, c_tok);
        tok = tok->next;

        if (!tok) {
            c_tok       = aalloc(pp->a, sizeof(token));
            c_tok->kind = TOK_EOF;
            LLISTC_ADD_LAST(&converted, c_tok);
        }
    }

    token *expr_tokens = converted.first;
    ast *expr_ast      = ast_cond_incl_expr_ternary(pp->a, &expr_tokens);
    int64_t result     = evaluate_constant_expression(expr_ast);

    return result;
}

static bool
process_pp_directive(preprocessor *pp, pp_token **tokp) {
    bool result   = false;
    pp_token *tok = *tokp;
    if (PP_TOK_IS_PUNCT(tok, '#') && tok->at_line_start) {
        tok = tok->next;
        if (tok->kind == PP_TOK_ID) {
            if (string_eq(tok->str, WRAPZ("define"))) {
                tok = tok->next;
                define_macro(pp, &tok);
            } else if (string_eq(tok->str, WRAPZ("undef"))) {
                tok = tok->next;
                undef_macro(pp, &tok);
            } else if (string_eq(tok->str, WRAPZ("if"))) {
                tok                 = tok->next;
                int64_t expr_result = eval_pp_expr(pp, &tok);
                push_cond_incl(pp, expr_result != 0);
                if (!expr_result) {
                    skip_cond_incl(pp, &tok);
                }
            } else if (string_eq(tok->str, WRAPZ("elif"))) {
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
            } else if (string_eq(tok->str, WRAPZ("else"))) {
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
            } else if (string_eq(tok->str, WRAPZ("endif"))) {
                tok                          = tok->next;
                pp_conditional_include *incl = pp->cond_incl_stack;
                if (!incl) {
                    NOT_IMPL;
                } else {
                    LLIST_POP(pp->cond_incl_stack);
                    LLIST_ADD(pp->cond_incl_freelist, incl);
                }
            } else if (string_eq(tok->str, WRAPZ("ifdef"))) {
                tok = tok->next;
                if (tok->kind != PP_TOK_ID) {
                    NOT_IMPL;
                }
                uint32_t macro_name_hash = hash_string(tok->str);
                bool is_defined          = GET_MACRO(pp, macro_name_hash) != 0;
                push_cond_incl(pp, is_defined);
                if (!is_defined) {
                    skip_cond_incl(pp, &tok);
                }
            } else if (string_eq(tok->str, WRAPZ("ifndef"))) {
                tok = tok->next;
                if (tok->kind != PP_TOK_ID) {
                    NOT_IMPL;
                }
                uint32_t macro_name_hash = hash_string(tok->str);
                bool is_defined          = GET_MACRO(pp, macro_name_hash) != 0;
                push_cond_incl(pp, !is_defined);
                if (is_defined) {
                    skip_cond_incl(pp, &tok);
                }
            } else if (string_eq(tok->str, WRAPZ("line"))) {
                // TODO:
            } else if (string_eq(tok->str, WRAPZ("pragma"))) {
                NOT_IMPL;
            } else if (string_eq(tok->str, WRAPZ("error"))) {
                // TODO:
                /* NOT_IMPL; */
            } else if (string_eq(tok->str, WRAPZ("warning"))) {
                // TODO:
            } else if (string_eq(tok->str, WRAPZ("include"))) {
                tok = tok->next;
                if (tok->at_line_start) {
                    NOT_IMPL;
                }

                if (PP_TOK_IS_PUNCT(tok, '<')) {
                    tok = tok->next;
                    char filename_buffer[4096];
                    char *buf_eof = filename_buffer + sizeof(filename_buffer);
                    char *cursor  = filename_buffer;
                    while (!PP_TOK_IS_PUNCT(tok, '>') && !tok->at_line_start) {
                        cursor += fmt_pp_tok(cursor, buf_eof - cursor, tok);
                        tok = tok->next;
                    }

                    if (!PP_TOK_IS_PUNCT(tok, '>')) {
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

static void
predifined_macro(preprocessor *pp, string name, string value) {
    char lexer_buffer[4096];
    pp_lexer lex = {0};
    init_pp_lexer(&lex, value.data, STRING_END(value), lexer_buffer,
                  sizeof(lexer_buffer));

    linked_list_constructor tokens = {0};
    for (;;) {
        pp_token *tok        = NEW_PP_TOKEN(pp);
        bool should_continue = pp_lexer_parse(&lex, tok);
        tok->loc.filename    = WRAPZ("BUILTIN");
        if (tok->str.data) {
            tok->str = string_dup(pp->a, tok->str);
        }
        LLISTC_ADD_LAST(&tokens, tok);
        if (!should_continue) {
            break;
        }
    }

    uint32_t name_hash = hash_string(name);
    pp_macro **macrop  = GET_MACROP(pp, name_hash);
    assert(!*macrop);
    pp_macro *macro = NEW_MACRO(pp);
    *macrop         = macro;

    macro->name       = name;
    macro->name_hash  = name_hash;
    macro->kind       = PP_MACRO_OBJ;
    macro->definition = tokens.first;
}

static void
define_common_predifined_macros(preprocessor *pp, string filename) {
    string file_onlyname = path_filename(filename);
    string file_name = string_memprintf(pp->a, "\"%.*s\"", file_onlyname.len,
                                        file_onlyname.data);
    predifined_macro(pp, WRAPZ("__FILE_NAME__"), file_name);

    string base_file =
        string_memprintf(pp->a, "\"%.*s\"", filename.len, filename.data);
    predifined_macro(pp, WRAPZ("__BASE_FILE__"), base_file);

    time_t now    = time(0);
    struct tm *tm = localtime(&now);

    static char *months[] = {
        "Jan", "Feb", "Mar", "Apr", "May", "Jun",
        "Jul", "Aug", "Sep", "Oct", "Nov", "Dec",
    };
    assert((unsigned)tm->tm_mon < ARRAY_SIZE(months));
    string date = string_memprintf(pp->a, "\"%s %2d %d\"", months[tm->tm_mon],
                                   tm->tm_mday, tm->tm_year + 1900);
    predifined_macro(pp, WRAPZ("__DATE__"), date);

    string time = string_memprintf(pp->a, "\"%02d:%02d:%02d\"", tm->tm_hour,
                                   tm->tm_min, tm->tm_sec);
    predifined_macro(pp, WRAPZ("__TIME__"), time);

    predifined_macro(pp, WRAPZ("__SIZE_TYPE__"), WRAPZ("unsigned long"));
    predifined_macro(pp, WRAPZ("__PTRDIFF_TYPE__"), WRAPZ("long long"));
    predifined_macro(pp, WRAPZ("__WCHAR_TYPE__"), WRAPZ("unsigned int"));
    predifined_macro(pp, WRAPZ("__WINT_TYPE__"), WRAPZ("unsigned int"));
    predifined_macro(pp, WRAPZ("__INTMAX_TYPE__"), WRAPZ("long long"));
    predifined_macro(pp, WRAPZ("__UINTMAX_TYPE__"),
                     WRAPZ("unsigned long long"));
    /* predifined_macro(pp, WRAPZ("__SIG_ATOMIC_TYPE__"),
     * WRAPZ("sig_atomic_t")); */
    predifined_macro(pp, WRAPZ("__INT8_TYPE__"), WRAPZ("signed char"));
    predifined_macro(pp, WRAPZ("__INT16_TYPE__"), WRAPZ("short"));
    predifined_macro(pp, WRAPZ("__INT32_TYPE__"), WRAPZ("int"));
    predifined_macro(pp, WRAPZ("__INT64_TYPE__"), WRAPZ("long long"));
    predifined_macro(pp, WRAPZ("__UINT8_TYPE__"), WRAPZ("unsigned char"));
    predifined_macro(pp, WRAPZ("__UINT16_TYPE__"), WRAPZ("unsigned short"));
    predifined_macro(pp, WRAPZ("__UINT32_TYPE__"), WRAPZ("unsigned"));
    predifined_macro(pp, WRAPZ("__UINT64_TYPE__"), WRAPZ("unsigned long long"));
    predifined_macro(pp, WRAPZ("__INT_LEAST8_TYPE__"), WRAPZ("signed char"));
    predifined_macro(pp, WRAPZ("__INT_LEAST16_TYPE__"), WRAPZ("short"));
    predifined_macro(pp, WRAPZ("__INT_LEAST32_TYPE__"), WRAPZ("int"));
    predifined_macro(pp, WRAPZ("__INT_LEAST64_TYPE__"), WRAPZ("long long"));
    predifined_macro(pp, WRAPZ("__UINT_LEAST8_TYPE__"), WRAPZ("unsigned char"));
    predifined_macro(pp, WRAPZ("__UINT_LEAST16_TYPE__"),
                     WRAPZ("unsigned short"));
    predifined_macro(pp, WRAPZ("__UINT_LEAST32_TYPE__"), WRAPZ("unsigned int"));
    predifined_macro(pp, WRAPZ("__UINT_LEAST64_TYPE__"),
                     WRAPZ("unsigned long long"));
    predifined_macro(pp, WRAPZ("__INT_FAST8_TYPE__"), WRAPZ("signed char"));
    predifined_macro(pp, WRAPZ("__INT_FAST16_TYPE__"), WRAPZ("short"));
    predifined_macro(pp, WRAPZ("__INT_FAST32_TYPE__"), WRAPZ("int"));
    predifined_macro(pp, WRAPZ("__INT_FAST64_TYPE__"), WRAPZ("long long"));
    predifined_macro(pp, WRAPZ("__UINT_FAST8_TYPE__"), WRAPZ("unsigned char"));
    predifined_macro(pp, WRAPZ("__UINT_FAST16_TYPE__"),
                     WRAPZ("unsigned short"));
    predifined_macro(pp, WRAPZ("__UINT_FAST32_TYPE__"), WRAPZ("unsigned int"));
    predifined_macro(pp, WRAPZ("__UINT_FAST64_TYPE__"),
                     WRAPZ("unsigned long long"));
    predifined_macro(pp, WRAPZ("__INTPTR_TYPE__"), WRAPZ("long long"));
    predifined_macro(pp, WRAPZ("__UINTPTR_TYPE__"),
                     WRAPZ("unsigned long long"));

    predifined_macro(pp, WRAPZ("__CHAR_BIT__"), WRAPZ("8"));

    /* TODO:
     * __SCHAR_MAX__
__WCHAR_MAX__
__SHRT_MAX__
__INT_MAX__
__LONG_MAX__
__LONG_LONG_MAX__
__WINT_MAX__
__SIZE_MAX__
__PTRDIFF_MAX__
__INTMAX_MAX__
__UINTMAX_MAX__
__SIG_ATOMIC_MAX__
__INT8_MAX__
__INT16_MAX__
__INT32_MAX__
__INT64_MAX__
__UINT8_MAX__
__UINT16_MAX__
__UINT32_MAX__
__UINT64_MAX__
__INT_LEAST8_MAX__
__INT_LEAST16_MAX__
__INT_LEAST32_MAX__
__INT_LEAST64_MAX__
__UINT_LEAST8_MAX__
__UINT_LEAST16_MAX__
__UINT_LEAST32_MAX__
__UINT_LEAST64_MAX__
__INT_FAST8_MAX__
__INT_FAST16_MAX__
__INT_FAST32_MAX__
__INT_FAST64_MAX__
__UINT_FAST8_MAX__
__UINT_FAST16_MAX__
__UINT_FAST32_MAX__
__UINT_FAST64_MAX__
__INTPTR_MAX__
__UINTPTR_MAX__
__WCHAR_MIN__
__WINT_MIN__
__SIG_ATOMIC_MIN__


__INT8_C
__INT16_C
__INT32_C
__INT64_C
__UINT8_C
__UINT16_C
__UINT32_C
__UINT64_C
__INTMAX_C
__UINTMAX_C

__SCHAR_WIDTH__
__SHRT_WIDTH__
__INT_WIDTH__
__LONG_WIDTH__
__LONG_LONG_WIDTH__
__PTRDIFF_WIDTH__
__SIG_ATOMIC_WIDTH__
__SIZE_WIDTH__
__WCHAR_WIDTH__
__WINT_WIDTH__
__INT_LEAST8_WIDTH__
__INT_LEAST16_WIDTH__
__INT_LEAST32_WIDTH__
__INT_LEAST64_WIDTH__
__INT_FAST8_WIDTH__
__INT_FAST16_WIDTH__
__INT_FAST32_WIDTH__
__INT_FAST64_WIDTH__
__INTPTR_WIDTH__
__INTMAX_WIDTH__
*/
    predifined_macro(pp, WRAPZ("__SIZEOF_INT__"), WRAPZ("4"));
    predifined_macro(pp, WRAPZ("__SIZEOF_LONG"), WRAPZ("8"));
    predifined_macro(pp, WRAPZ("__SIZEOF_LONG_LONG__"), WRAPZ("8"));
    predifined_macro(pp, WRAPZ("__SIZEOF_SHORT__"), WRAPZ("2"));
    predifined_macro(pp, WRAPZ("__SIZEOF_POINTER__"), WRAPZ("8"));
    predifined_macro(pp, WRAPZ("__SIZEOF_FLOAT__"), WRAPZ("4"));
    predifined_macro(pp, WRAPZ("__SIZEOF_DOUBLE__"), WRAPZ("8"));
    predifined_macro(pp, WRAPZ("__SIZEOF_LONG_DOUBLE__"), WRAPZ("8"));
    predifined_macro(pp, WRAPZ("__SIZEOF_SIZE_T__"), WRAPZ("8"));
    predifined_macro(pp, WRAPZ("__SIZEOF_WCHAR_T__"), WRAPZ("4"));
    predifined_macro(pp, WRAPZ("__SIZEOF_WINT_T__"), WRAPZ("4"));
    predifined_macro(pp, WRAPZ("__SIZEOF_PTRDIFF_T__"), WRAPZ("8"));

    predifined_macro(pp, WRAPZ("__STDC_HOSTED__"), WRAPZ("1"));
    predifined_macro(pp, WRAPZ("__STDC_NO_COMPLEX__"), WRAPZ("1"));
    predifined_macro(pp, WRAPZ("__STDC_UTF_16__"), WRAPZ("1"));
    predifined_macro(pp, WRAPZ("__STDC_UTF_32__"), WRAPZ("1"));
    predifined_macro(pp, WRAPZ("__STDC__"), WRAPZ("1"));

    predifined_macro(pp, WRAPZ("__LP64__"), WRAPZ("1"));
    predifined_macro(pp, WRAPZ("_LP64"), WRAPZ("1"));

    predifined_macro(pp, WRAPZ("__x86_64__"), WRAPZ("1"));
    predifined_macro(pp, WRAPZ("__x86_64"), WRAPZ("1"));
    predifined_macro(pp, WRAPZ("__amd64"), WRAPZ("1"));
    predifined_macro(pp, WRAPZ("__amd64__"), WRAPZ("1"));
    predifined_macro(pp, WRAPZ("__holoc__"), WRAPZ("1"));
    predifined_macro(pp, WRAPZ("__gnu_linux__"), WRAPZ("1"));
    predifined_macro(pp, WRAPZ("__linux__"), WRAPZ("1"));
    predifined_macro(pp, WRAPZ("__linux"), WRAPZ("1"));
    predifined_macro(pp, WRAPZ("linux"), WRAPZ("1"));
    predifined_macro(pp, WRAPZ("unix"), WRAPZ("1"));

    predifined_macro(pp, WRAPZ("__inline__"), WRAPZ("inline"));
    predifined_macro(pp, WRAPZ("__signed__"), WRAPZ("signed"));
    predifined_macro(pp, WRAPZ("__typeof__"), WRAPZ("typeof"));
    predifined_macro(pp, WRAPZ("__volatile__"), WRAPZ("volatile"));
    predifined_macro(pp, WRAPZ("__alignof__"), WRAPZ("_Alignof"));
}

void
init_pp(preprocessor *pp, string filename, char *tok_buf,
        uint32_t tok_buf_size) {
    pp->tok_buf          = tok_buf;
    pp->tok_buf_capacity = tok_buf_size;

    linked_list_constructor base_file = get_pp_tokens_for_file(pp, filename);

    pp_token *eof = NEW_PP_TOKEN(pp);
    eof->kind     = PP_TOK_EOF;
    LLISTC_ADD_LAST(&base_file, eof);

    pp->toks = base_file.first;
    define_common_predifined_macros(pp, filename);
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

        if (!convert_pp_token(pp->toks, tok, pp->a)) {
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
        pp->toks = pp->toks->next;
        result   = pp->toks != 0;
        break;
    }

    return result;
}

