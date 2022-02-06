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
#include "cond_incl_ast.h"
#include "error_reporter.h"
#include "file_storage.h"
#include "filepath.h"
#include "hashing.h"
#include "llist.h"
#include "pp_lexer.h"
#include "pp_token_iter.h"
#include "str.h"

// NOTE: These can be separate functions, but since they are really wrappers for
// another function let's not make them that way and keep as macros.
#define GET_MACROP(_pp, _hash)                                                           \
    (pp_macro **)hash_table_sc_get_u32((_pp)->macro_hash, ARRAY_SIZE((_pp)->macro_hash), \
                                       pp_macro, next, name_hash, (_hash))
#define GET_MACRO(_pp, _hash) (*GET_MACROP(_pp, _hash))

// Frees macro arguments and adds them to freelist.
// TODO: Free tokens too.
static void
free_macro_data(preprocessor *pp, pp_macro *macro) {
    while (macro->args) {
        pp_macro_arg *arg = macro->args;
        macro->args       = arg->next;
        afree(pp->a, arg, sizeof(pp_macro_arg));
    }
}

// Returns new token and writes memory contained in given to it, effectively
// making a copy.
static pp_token *
copy_pp_token(preprocessor *pp, pp_token *tok) {
    pp_token *new = aalloc(pp->a, sizeof(pp_token));
    memcpy(new, tok, sizeof(pp_token));
    new->next = 0;
    return new;
}

// Function used when #define'ing a function-like macro. Parses given token list
// and forms arguments, that are written to the given macro.
// Doesn't eat closing paren.
static void
get_function_like_macro_arguments(preprocessor *pp, pp_token_iter *it, pp_macro *macro) {
    pp_token *tok = ppti_peek(it);
    // If next token is closing parens, don't collect arguments.
    if (macro->arg_count == 0 && !macro->is_variadic && !PP_TOK_IS_PUNCT(tok, ')')) {
        report_error_pp_token(tok, "Unexpected macro arguments");
    } else {
        // Collect arguments.
        // arg_idx needed for variadic argument checking and validating number
        // of arguments.
        uint32_t arg_idx  = 0;
        pp_macro_arg *arg = macro->args;

        while (tok->kind != PP_TOK_EOF) {
            // Commans inclosed in parens are not treated as argument
            // separators.
            uint32_t parens_depth = 0;
            // Construct list of copied tokens that form tokens of argument.
            // Varidic arguments are formed in a hacky way - they all are
            // threated as single argument
            linked_list_constructor macro_tokens = {0};
            while (tok->kind != PP_TOK_EOF) {
                if (PP_TOK_IS_PUNCT(tok, ')') && parens_depth == 0) {
                    break;
                } else if (PP_TOK_IS_PUNCT(tok, ',') && parens_depth == 0) {
                    tok = ppti_eat_peek(it);
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
                tok = ppti_eat_peek(it);
            }
            // Add eof to the end
            pp_token *eof = aalloc(pp->a, sizeof(pp_token));
            eof->kind     = PP_TOK_EOF;
            LLISTC_ADD_LAST(&macro_tokens, eof);

            arg->toks = macro_tokens.first;

            ++arg_idx;
            arg = arg->next;
            // Check if we are finished
            if (PP_TOK_IS_PUNCT(tok, ')')) {
                if (arg_idx != macro->arg_count && !macro->is_variadic) {
                    report_error_pp_token(tok,
                                          "Incorrect number of arguments in macro invocation "
                                          "(expected %u, got %u)",
                                          macro->arg_count, arg_idx + 1);
                }
                break;
            }
        }
    }
}

// Used internally in expand_function_like_macro.
// Finds argument by name in macro argument list.
static pp_macro_arg *
get_argument(pp_macro *macro, string name) {
    pp_macro_arg *arg = NULL;
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
expand_function_like_macro(preprocessor *pp, pp_token_iter *it, pp_macro *macro,
                           source_loc initial_loc) {
    // Collect macro arguments.
    get_function_like_macro_arguments(pp, it, macro);
    pp_token *tok = ppti_peek(it);
    if (!PP_TOK_IS_PUNCT(tok, ')')) {
        report_error_pp_token(tok, "Missing closing paren in function-like macro invocation");
    } else {
        ppti_eat(it);
    }

    linked_list_constructor def = {0};
    for (pp_token *temp = macro->definition; temp->kind != PP_TOK_EOF; temp = temp->next) {
        // Check if given token is a macro argument
        if (temp->kind == PP_TOK_ID) {
            pp_macro_arg *arg = get_argument(macro, temp->str);
            if (arg) {
                for (pp_token *arg_tok = arg->toks; arg_tok->kind != PP_TOK_EOF;
                     arg_tok           = arg_tok->next) {
                    pp_token *new_token = copy_pp_token(pp, arg_tok);
                    new_token->loc      = initial_loc;
                    LLISTC_ADD_LAST(&def, new_token);
                }
                continue;
            }
        } else if (PP_TOK_IS_PUNCT(temp, '#')) {
            temp              = temp->next;
            pp_macro_arg *arg = get_argument(macro, temp->str);
            if (!arg) {
                report_error_pp_token(tok,
                                      "Expected macro argument after "
                                      "stringification operator '#'");
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
        new_token->loc      = initial_loc;
        LLISTC_ADD_LAST(&def, new_token);
    }

    if (def.first) {
        ppti_insert_tok_list(it, def.first, def.last);
    }
}

static bool
expand_macro(preprocessor *pp, pp_token_iter *it) {
    bool result     = false;
    pp_macro *macro = NULL;
    pp_token *tok   = ppti_peek(it);
    if (tok->kind == PP_TOK_ID) {
        uint32_t name_hash = hash_string(tok->str);
        macro              = GET_MACRO(pp, name_hash);
    }

    if (macro) {
        switch (macro->kind) {
            INVALID_DEFAULT_CASE;
        case PP_MACRO_OBJ: {
            source_loc initial_loc = tok->loc;

            linked_list_constructor def = {0};
            for (pp_token *temp = macro->definition; temp->kind != PP_TOK_EOF;
                 temp           = temp->next) {
                pp_token *new_token = copy_pp_token(pp, temp);
                new_token->loc      = initial_loc;
                LLISTC_ADD_LAST(&def, new_token);
            }
            // Eat the identifier. We do it here because we need it for copying
            // source information, like location to new tokens.
            ppti_eat(it);
            if (def.first) {
                ppti_insert_tok_list(it, def.first, def.last);
            }

            result = true;
        } break;
        case PP_MACRO_FUNC: {
            source_loc initial_loc = tok->loc;

            pp_token *next = ppti_peek_forward(it, 1);
            if (next && !next->has_whitespace && PP_TOK_IS_PUNCT(next, '(')) {
                ppti_eat_multiple(it, 2);
                expand_function_like_macro(pp, it, macro, initial_loc);
                result = true;
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
    return result;
}

static void
define_macro_function_like_args(preprocessor *pp, pp_macro *macro) {
    pp_token *tok = ppti_peek(pp->it);

    linked_list_constructor args = {0};
    while (tok->kind != PP_TOK_EOF) {
        if (tok->at_line_start) {
            break;
        }

        // Eat argument
        if (PP_TOK_IS_PUNCT(tok, PP_TOK_PUNCT_VARARGS)) {
            if (macro->is_variadic) {
                report_error_pp_token(tok, "Multiple varargs");
            }

            macro->is_variadic = true;
            pp_macro_arg *arg  = aalloc(pp->a, sizeof(pp_macro_arg));
            arg->name          = WRAPZ("__VA_ARGS__");
            LLISTC_ADD_LAST(&args, arg);

            tok = ppti_eat_peek(pp->it);
        } else if (tok->kind != PP_TOK_ID) {
            report_error_pp_token(tok, "Unexpected token");
            break;
        } else {
            if (macro->is_variadic) {
                report_error_pp_token(tok, "Argument after varargs");
            }

            pp_macro_arg *arg = aalloc(pp->a, sizeof(pp_macro_arg));
            arg->name         = tok->str;
            LLISTC_ADD_LAST(&args, arg);
            ++macro->arg_count;

            tok = ppti_eat_peek(pp->it);
        }

        // Parse post-argument
        if (!tok->at_line_start && PP_TOK_IS_PUNCT(tok, ',')) {
            tok = ppti_eat_peek(pp->it);
        } else {
            break;
        }
    }

    if (tok->at_line_start || !PP_TOK_IS_PUNCT(tok, ')')) {
        report_error_pp_token(tok, "Expected closing paren in function-like macro");
    } else {
        macro->args = args.first;
    }
}

static void
define_macro(preprocessor *pp) {
    pp_token *tok = ppti_peek(pp->it);

    if (tok->kind != PP_TOK_ID) {
        report_error_pp_token(tok, "Expected identifier after #define");
    }

    // Get macro
    string macro_name        = tok->str;
    uint32_t macro_name_hash = hash_string(macro_name);
    pp_macro **macrop        = GET_MACROP(pp, macro_name_hash);
    if (*macrop) {
        report_error_pp_token(tok, "#define on already defined macro");
    } else {
        pp_macro *macro = aalloc(pp->a, sizeof(pp_macro));
        LLIST_ADD(*macrop, macro);
    }

    pp_macro *macro = *macrop;
    assert(macro);

    macro->name      = macro_name;
    macro->name_hash = macro_name_hash;

    tok = ppti_eat_peek(pp->it);
    // Function-like macro
    if (PP_TOK_IS_PUNCT(tok, '(') && !tok->has_whitespace) {
        tok = ppti_eat_peek(pp->it);
        if (!PP_TOK_IS_PUNCT(tok, ')')) {
            define_macro_function_like_args(pp, macro);
        }

        tok = ppti_peek(pp->it);
        if (!PP_TOK_IS_PUNCT(tok, ')')) {
            report_error_pp_token(tok, "Missin ')' in macro parameter list");
        } else {
            tok = ppti_eat_peek(pp->it);
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
        tok = ppti_eat_peek(pp->it);
    }

    pp_token *eof = aalloc(pp->a, sizeof(pp_token));
    eof->kind     = PP_TOK_EOF;
    LLISTC_ADD_LAST(&def, eof);
    macro->definition = def.first;
}

static void
undef_macro(preprocessor *pp) {
    pp_token *tok = ppti_peek(pp->it);
    if (tok->kind != PP_TOK_ID) {
        report_error_pp_token(tok, "Expected identifier after #undef");
    }

    string macro_name        = tok->str;
    uint32_t macro_name_hash = hash_string(macro_name);
    pp_macro **macrop        = GET_MACROP(pp, macro_name_hash);
    if (*macrop) {
        pp_macro *macro = *macrop;
        free_macro_data(pp, macro);
        *macrop = macro->next;
        afree(pp->a, macro, sizeof(pp_macro));
    }
}

static void
push_cond_incl(preprocessor *pp, bool is_included) {
    pp_conditional_include *incl = aalloc(pp->a, sizeof(pp_conditional_include));
    incl->is_included            = is_included;
    LLIST_ADD(pp->cond_incl_stack, incl);
}

static void
skip_cond_incl(preprocessor *pp) {
    pp_token *tok  = ppti_peek(pp->it);
    uint32_t depth = 0;
    while (tok->kind != PP_TOK_EOF) {
        if (!PP_TOK_IS_PUNCT(tok, '#') || !tok->at_line_start) {
            tok = ppti_eat_peek(pp->it);
            continue;
        }
        // now the token is hash
        pp_token *next = ppti_peek_forward(pp->it, 1);

        if (next->kind != PP_TOK_ID) {
            ppti_eat_multiple(pp->it, 2);
            continue;
        }

        if (string_eq(next->str, WRAPZ("if")) || string_eq(next->str, WRAPZ("ifdef")) ||
            string_eq(next->str, WRAPZ("ifndef"))) {
            ++depth;
        } else if (string_eq(next->str, WRAPZ("elif")) ||
                   string_eq(next->str, WRAPZ("else")) ||
                   string_eq(next->str, WRAPZ("endif"))) {
            if (!depth) {
                break;
            }
            if (string_eq(next->str, WRAPZ("endif"))) {
                --depth;
            }
        }
        ppti_eat_multiple(pp->it, 2);
        tok = ppti_peek(pp->it);
    }

    if (depth) {
        NOT_IMPL;
    }
}

static int64_t
eval_pp_expr(preprocessor *pp) {
    // Copy all tokens from current line so we can process them
    // independently
    pp_token *tok                  = ppti_peek(pp->it);
    linked_list_constructor copied = {0};
    while (!tok->at_line_start) {
        pp_token *new_tok = copy_pp_token(pp, tok);
        LLISTC_ADD_LAST(&copied, new_tok);
        tok = ppti_eat_peek(pp->it);
    }
    pp_token *eof = aalloc(pp->a, sizeof(pp_token));
    eof->kind     = PP_TOK_EOF;
    LLISTC_ADD_LAST(&copied, eof);

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
                report_error_pp_token(tok, "Expected identifier");
                break;
            }

            uint32_t macro_name_hash = hash_string(tok->str);
            pp_macro *macro          = GET_MACRO(pp, macro_name_hash);

            pp_token *new_token = aalloc(pp->a, sizeof(pp_token));
            new_token->kind     = PP_TOK_NUM;
            new_token->str      = (macro != 0) ? WRAPZ("1") : WRAPZ("0");

            LLISTC_ADD_LAST(&modified, new_token);

            tok = tok->next;
            if (has_paren) {
                if (!PP_TOK_IS_PUNCT(tok, ')')) {
                    report_error_pp_token(tok, "Expected closing parens ')' for defined()");
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
    linked_list_constructor converted = {0};
    pp_token_iter iter                = {.a = pp->a};
    ppti_insert_tok_list(&iter, modified.first, modified.last);
    tok = ppti_peek(&iter);
    while (tok) {
        if (expand_macro(pp, &iter)) {
            tok = ppti_peek(&iter);
            continue;
        }

        if (tok->kind == PP_TOK_ID) {
            pp_macro *macro = GET_MACRO(pp, hash_string(tok->str));
            // Identifier can be present here if it is part of function-like
            // macro. In this case we can't do anything with it, so leave it be
            // and error will be reported when evaluating expression.
            if (macro) {
                report_error_pp_token(tok, "Unexpected identifier");
                break;
            } else {
                tok->kind = PP_TOK_NUM;
                tok->str  = WRAPZ("0");
            }
        }

        token *c_tok = aalloc(pp->a, sizeof(token));
        char buf[4096];
        uint32_t buf_len = 0;
        if (!convert_pp_token(tok, c_tok, buf, sizeof(buf), &buf_len, pp->a)) {
            report_error_pp_token(tok, "Unexpected token");
            break;
        }

        if (buf_len) {
            c_tok->str = string_dup(pp->a, c_tok->str);
        }
        LLISTC_ADD_LAST(&converted, c_tok);
        tok = ppti_eat_peek(&iter);
    }

    token *expr_tokens = converted.first;
#if 0
    for (token *t = expr_tokens; t; t = t->next) {
        char buffer[4096];
        fmt_token(buffer, sizeof(buffer), t);
        printf("%s", buffer);
    }
    printf("\n");
#endif
    ast *expr_ast  = ast_cond_incl_expr_ternary(pp->a, &expr_tokens);
    int64_t result = ast_cond_incl_eval(expr_ast);

    return result;
}

static bool
process_pp_directive(preprocessor *pp) {
    pp_token *tok = ppti_peek(pp->it);
    bool result   = false;
    if (PP_TOK_IS_PUNCT(tok, '#') && tok->at_line_start) {
        tok = ppti_eat_peek(pp->it);
        if (tok->kind == PP_TOK_ID) {
            if (string_eq(tok->str, WRAPZ("define"))) {
                tok = ppti_eat_peek(pp->it);
                define_macro(pp);
            } else if (string_eq(tok->str, WRAPZ("undef"))) {
                tok = ppti_eat_peek(pp->it);
                undef_macro(pp);
            } else if (string_eq(tok->str, WRAPZ("if"))) {
                /* report_error_pp_token(pp, tok, "DBG: IF"); */
                tok = ppti_eat_peek(pp->it);

                int64_t expr_result = eval_pp_expr(pp);
                push_cond_incl(pp, expr_result != 0);
                if (!expr_result) {
                    skip_cond_incl(pp);
                }
            } else if (string_eq(tok->str, WRAPZ("elif"))) {
                pp_conditional_include *incl = pp->cond_incl_stack;
                if (!incl) {
                    report_error_pp_token(tok, "Stray #elif");
                    tok = ppti_eat_peek(pp->it);
                } else {
                    if (incl->is_after_else) {
                        report_error_pp_token(tok, "#elif is after #else");
                    }

                    tok = ppti_eat_peek(pp->it);
                    if (!incl->is_included) {
                        int64_t expr_result = eval_pp_expr(pp);
                        if (expr_result) {
                            incl->is_included = true;
                        } else {
                            skip_cond_incl(pp);
                        }
                    } else {
                        skip_cond_incl(pp);
                    }
                }
            } else if (string_eq(tok->str, WRAPZ("else"))) {
                pp_conditional_include *incl = pp->cond_incl_stack;
                if (!incl) {
                    report_error_pp_token(tok, "Stray #else");
                    tok = ppti_eat_peek(pp->it);
                } else {
                    /* report_error_pp_token(pp, tok, "DBG: ELSE"); */
                    if (incl->is_after_else) {
                        report_error_pp_token(tok, "#else is after #else");
                    }
                    tok                 = ppti_eat_peek(pp->it);
                    incl->is_after_else = true;
                    if (incl->is_included) {
                        skip_cond_incl(pp);
                    }
                }
            } else if (string_eq(tok->str, WRAPZ("endif"))) {
                pp_conditional_include *incl = pp->cond_incl_stack;
                if (!incl) {
                    report_error_pp_token(tok, "Stray #endif");
                    tok = ppti_eat_peek(pp->it);
                } else {
                    /* report_error_pp_token(pp, tok, "DBG: ENDIF"); */
                    tok = ppti_eat_peek(pp->it);
                    LLIST_POP(pp->cond_incl_stack);
                    afree(pp->a, incl, sizeof(pp_conditional_include));
                }
            } else if (string_eq(tok->str, WRAPZ("ifdef"))) {
                tok = ppti_eat_peek(pp->it);
                if (tok->kind != PP_TOK_ID) {
                    report_error_pp_token(tok, "Expected identifier");
                }

                uint32_t macro_name_hash = hash_string(tok->str);
                bool is_defined          = GET_MACRO(pp, macro_name_hash) != 0;
                push_cond_incl(pp, is_defined);
                tok = ppti_eat_peek(pp->it);
                if (!is_defined) {
                    skip_cond_incl(pp);
                }
            } else if (string_eq(tok->str, WRAPZ("ifndef"))) {
                tok = ppti_eat_peek(pp->it);

                if (tok->kind != PP_TOK_ID) {
                    report_error_pp_token(tok, "Expected identifier");
                }

                uint32_t macro_name_hash = hash_string(tok->str);
                bool is_defined          = GET_MACRO(pp, macro_name_hash) != 0;
                push_cond_incl(pp, !is_defined);
                tok = ppti_eat_peek(pp->it);
                if (is_defined) {
                    skip_cond_incl(pp);
                }
            } else if (string_eq(tok->str, WRAPZ("line"))) {
                // TODO:
            } else if (string_eq(tok->str, WRAPZ("pragma"))) {
                NOT_IMPL;
            } else if (string_eq(tok->str, WRAPZ("error"))) {
                source_loc error_loc = tok->loc;

                tok = ppti_eat_peek(pp->it);
                char buffer[4096];
                buffer_writer w = {buffer, buffer + sizeof(buffer)};
                while (!tok->at_line_start) {
                    fmt_pp_tokw(&w, tok);
                    buf_write(&w, " ");
                    tok = ppti_eat_peek(pp->it);
                }
                report_error(error_loc, "%s", buffer);
            } else if (string_eq(tok->str, WRAPZ("warning"))) {
                source_loc error_loc = tok->loc;

                tok = ppti_eat_peek(pp->it);
                char buffer[4096];
                buffer_writer w = {buffer, buffer + sizeof(buffer)};
                while (!tok->at_line_start) {
                    fmt_pp_tokw(&w, tok);
                    buf_write(&w, " ");
                    tok = ppti_eat_peek(pp->it);
                }
                report_warning(error_loc, "%s", buffer);
            } else if (string_eq(tok->str, WRAPZ("include"))) {
                tok = ppti_eat_peek(pp->it);

                if (tok->at_line_start) {
                    NOT_IMPL;
                }

                if (PP_TOK_IS_PUNCT(tok, '<')) {
                    tok = ppti_eat_peek(pp->it);
                    char filename_buffer[4096];
                    char *buf_eof = filename_buffer + sizeof(filename_buffer);
                    char *cursor  = filename_buffer;
                    while (!PP_TOK_IS_PUNCT(tok, '>') && !tok->at_line_start) {
                        cursor += fmt_pp_tok(cursor, buf_eof - cursor, tok);
                        tok = ppti_eat_peek(pp->it);
                    }

                    if (!PP_TOK_IS_PUNCT(tok, '>')) {
                        report_error_pp_token(tok, "Expected '>'");
                    } else {
                        tok = ppti_eat_peek(pp->it);
                    }

                    string filename = string(filename_buffer, cursor - filename_buffer);
                    ppti_include_file(pp->it, filename);
                } else if (tok->kind == PP_TOK_STR) {
                    string filename = tok->str;

                    tok = ppti_eat_peek(pp->it);
                    ppti_include_file(pp->it, filename);
                } else {
                    report_error_pp_token(tok, "Unexpected token (expected filename)");
                }
            } else {
                report_error_pp_token(tok, "Unexpected preprocessor directive");
            }
        } else {
            NOT_IMPL;
        }

        tok = ppti_peek(pp->it);
        if (tok->kind != TOK_EOF && !tok->at_line_start) {
            report_error_pp_token(tok, "Unexpected tokens aftern preprocessor directive");
            while (tok->kind != TOK_EOF && !tok->at_line_start) {
                tok = ppti_eat_peek(pp->it);
            }
        }
        result = true;
    }
    return result;
}

static void
predefined_macro(preprocessor *pp, string name, string value) {
    pp_lexer lex = {0};
    pp_lexer_init(&lex, value.data, STRING_END(value));

    linked_list_constructor tokens = {0};
    for (;;) {
        char buf[4096];
        uint32_t buf_len = 0;

        pp_token *tok        = aalloc(pp->a, sizeof(pp_token));
        bool should_continue = pp_lexer_parse(&lex, tok, buf, sizeof(buf), &buf_len);
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
    pp_macro *macro = aalloc(pp->a, sizeof(pp_token));
    *macrop         = macro;

    macro->name       = name;
    macro->name_hash  = name_hash;
    macro->kind       = PP_MACRO_OBJ;
    macro->definition = tokens.first;
}

static void
define_common_predefined_macros(preprocessor *pp, string filename) {
    string file_onlyname = path_filename(filename);
    string file_name =
        string_memprintf(pp->a, "\"%.*s\"", file_onlyname.len, file_onlyname.data);
    predefined_macro(pp, WRAPZ("__FILE_NAME__"), file_name);

    string base_file = string_memprintf(pp->a, "\"%.*s\"", filename.len, filename.data);
    predefined_macro(pp, WRAPZ("__BASE_FILE__"), base_file);

    time_t now    = time(0);
    struct tm *tm = localtime(&now);

    static char *months[] = {
        "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec",
    };
    assert((unsigned)tm->tm_mon < ARRAY_SIZE(months));
    string date = string_memprintf(pp->a, "\"%s %2d %d\"", months[tm->tm_mon], tm->tm_mday,
                                   tm->tm_year + 1900);
    predefined_macro(pp, WRAPZ("__DATE__"), date);

    string time =
        string_memprintf(pp->a, "\"%02d:%02d:%02d\"", tm->tm_hour, tm->tm_min, tm->tm_sec);
    predefined_macro(pp, WRAPZ("__TIME__"), time);

    predefined_macro(pp, WRAPZ("__SIZE_TYPE__"), WRAPZ("unsigned long"));
    predefined_macro(pp, WRAPZ("__PTRDIFF_TYPE__"), WRAPZ("long long"));
    predefined_macro(pp, WRAPZ("__WCHAR_TYPE__"), WRAPZ("unsigned int"));
    predefined_macro(pp, WRAPZ("__WINT_TYPE__"), WRAPZ("unsigned int"));
    predefined_macro(pp, WRAPZ("__INTMAX_TYPE__"), WRAPZ("long long"));
    predefined_macro(pp, WRAPZ("__UINTMAX_TYPE__"), WRAPZ("unsigned long long"));
    /* predefined_macro(pp, WRAPZ("__SIG_ATOMIC_TYPE__"),
     * WRAPZ("sig_atomic_t")); */
    predefined_macro(pp, WRAPZ("__INT8_TYPE__"), WRAPZ("signed char"));
    predefined_macro(pp, WRAPZ("__INT16_TYPE__"), WRAPZ("short"));
    predefined_macro(pp, WRAPZ("__INT32_TYPE__"), WRAPZ("int"));
    predefined_macro(pp, WRAPZ("__INT64_TYPE__"), WRAPZ("long long"));
    predefined_macro(pp, WRAPZ("__UINT8_TYPE__"), WRAPZ("unsigned char"));
    predefined_macro(pp, WRAPZ("__UINT16_TYPE__"), WRAPZ("unsigned short"));
    predefined_macro(pp, WRAPZ("__UINT32_TYPE__"), WRAPZ("unsigned"));
    predefined_macro(pp, WRAPZ("__UINT64_TYPE__"), WRAPZ("unsigned long long"));
    predefined_macro(pp, WRAPZ("__INT_LEAST8_TYPE__"), WRAPZ("signed char"));
    predefined_macro(pp, WRAPZ("__INT_LEAST16_TYPE__"), WRAPZ("short"));
    predefined_macro(pp, WRAPZ("__INT_LEAST32_TYPE__"), WRAPZ("int"));
    predefined_macro(pp, WRAPZ("__INT_LEAST64_TYPE__"), WRAPZ("long long"));
    predefined_macro(pp, WRAPZ("__UINT_LEAST8_TYPE__"), WRAPZ("unsigned char"));
    predefined_macro(pp, WRAPZ("__UINT_LEAST16_TYPE__"), WRAPZ("unsigned short"));
    predefined_macro(pp, WRAPZ("__UINT_LEAST32_TYPE__"), WRAPZ("unsigned int"));
    predefined_macro(pp, WRAPZ("__UINT_LEAST64_TYPE__"), WRAPZ("unsigned long long"));
    predefined_macro(pp, WRAPZ("__INT_FAST8_TYPE__"), WRAPZ("signed char"));
    predefined_macro(pp, WRAPZ("__INT_FAST16_TYPE__"), WRAPZ("short"));
    predefined_macro(pp, WRAPZ("__INT_FAST32_TYPE__"), WRAPZ("int"));
    predefined_macro(pp, WRAPZ("__INT_FAST64_TYPE__"), WRAPZ("long long"));
    predefined_macro(pp, WRAPZ("__UINT_FAST8_TYPE__"), WRAPZ("unsigned char"));
    predefined_macro(pp, WRAPZ("__UINT_FAST16_TYPE__"), WRAPZ("unsigned short"));
    predefined_macro(pp, WRAPZ("__UINT_FAST32_TYPE__"), WRAPZ("unsigned int"));
    predefined_macro(pp, WRAPZ("__UINT_FAST64_TYPE__"), WRAPZ("unsigned long long"));
    predefined_macro(pp, WRAPZ("__INTPTR_TYPE__"), WRAPZ("long long"));
    predefined_macro(pp, WRAPZ("__UINTPTR_TYPE__"), WRAPZ("unsigned long long"));

    predefined_macro(pp, WRAPZ("__CHAR_BIT__"), WRAPZ("8"));

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
    predefined_macro(pp, WRAPZ("__SIZEOF_INT__"), WRAPZ("4"));
    predefined_macro(pp, WRAPZ("__SIZEOF_LONG"), WRAPZ("8"));
    predefined_macro(pp, WRAPZ("__SIZEOF_LONG_LONG__"), WRAPZ("8"));
    predefined_macro(pp, WRAPZ("__SIZEOF_SHORT__"), WRAPZ("2"));
    predefined_macro(pp, WRAPZ("__SIZEOF_POINTER__"), WRAPZ("8"));
    predefined_macro(pp, WRAPZ("__SIZEOF_FLOAT__"), WRAPZ("4"));
    predefined_macro(pp, WRAPZ("__SIZEOF_DOUBLE__"), WRAPZ("8"));
    predefined_macro(pp, WRAPZ("__SIZEOF_LONG_DOUBLE__"), WRAPZ("8"));
    predefined_macro(pp, WRAPZ("__SIZEOF_SIZE_T__"), WRAPZ("8"));
    predefined_macro(pp, WRAPZ("__SIZEOF_WCHAR_T__"), WRAPZ("4"));
    predefined_macro(pp, WRAPZ("__SIZEOF_WINT_T__"), WRAPZ("4"));
    predefined_macro(pp, WRAPZ("__SIZEOF_PTRDIFF_T__"), WRAPZ("8"));

    predefined_macro(pp, WRAPZ("__STDC_HOSTED__"), WRAPZ("1"));
    predefined_macro(pp, WRAPZ("__STDC_NO_COMPLEX__"), WRAPZ("1"));
    predefined_macro(pp, WRAPZ("__STDC_UTF_16__"), WRAPZ("1"));
    predefined_macro(pp, WRAPZ("__STDC_UTF_32__"), WRAPZ("1"));
    predefined_macro(pp, WRAPZ("__STDC__"), WRAPZ("1"));
    predefined_macro(pp, WRAPZ("__STDC_VERSION__"), WRAPZ("201112L"));

    predefined_macro(pp, WRAPZ("__LP64__"), WRAPZ("1"));
    predefined_macro(pp, WRAPZ("_LP64"), WRAPZ("1"));

    predefined_macro(pp, WRAPZ("__x86_64__"), WRAPZ("1"));
    predefined_macro(pp, WRAPZ("__x86_64"), WRAPZ("1"));
    predefined_macro(pp, WRAPZ("__amd64"), WRAPZ("1"));
    predefined_macro(pp, WRAPZ("__amd64__"), WRAPZ("1"));
    predefined_macro(pp, WRAPZ("__holoc__"), WRAPZ("1"));
    predefined_macro(pp, WRAPZ("__gnu_linux__"), WRAPZ("1"));
    predefined_macro(pp, WRAPZ("__linux__"), WRAPZ("1"));
    predefined_macro(pp, WRAPZ("__linux"), WRAPZ("1"));
    predefined_macro(pp, WRAPZ("linux"), WRAPZ("1"));
    predefined_macro(pp, WRAPZ("unix"), WRAPZ("1"));

    predefined_macro(pp, WRAPZ("__inline__"), WRAPZ("inline"));
    predefined_macro(pp, WRAPZ("__signed__"), WRAPZ("signed"));
    predefined_macro(pp, WRAPZ("__typeof__"), WRAPZ("typeof"));
    predefined_macro(pp, WRAPZ("__volatile__"), WRAPZ("volatile"));
    predefined_macro(pp, WRAPZ("__alignof__"), WRAPZ("_Alignof"));
}

void
pp_init(preprocessor *pp, string filename) {
    define_common_predefined_macros(pp, filename);

    pp->it                  = aalloc(pp->a, sizeof(pp_token_iter));
    pp->it->a               = pp->a;
    pp->it->eof_token       = aalloc(pp->a, sizeof(pp_token));
    pp->it->eof_token->kind = PP_TOK_EOF;
    ppti_include_file(pp->it, filename);
}

bool
pp_parse(preprocessor *pp, struct token *tok, char *buf, uint32_t buf_size,
         uint32_t *buf_writtenp) {
    for (;;) {
        if (expand_macro(pp, pp->it)) {
            continue;
        }

        if (process_pp_directive(pp)) {
            continue;
        }

        pp_token *pp_tok = ppti_peek(pp->it);
        if (!pp_tok) {
            break;
        }

        if (!convert_pp_token(pp_tok, tok, buf, buf_size, buf_writtenp, pp->a)) {
            report_error_pp_token(pp_tok, "Unexpected token");
            ppti_eat(pp->it);
            continue;
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
        ppti_eat(pp->it);
        break;
    }

    return tok->kind != TOK_EOF;
}
