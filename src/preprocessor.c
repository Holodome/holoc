#include "preprocessor.h"

#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "allocator.h"
#include "bump_allocator.h"
#include "c_lang.h"
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

static pp_token *
copy_pp_token(preprocessor *pp, pp_token *tok) {
    pp_token *new = get_new_token(pp);
    *new          = *tok;
    new->next     = 0;
    return new;
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
        default:
            assert(false);
            break;
        case PP_MACRO_OBJ: {
            pp_token *initial = tok;
            tok               = tok->next;

            uint32_t idx                = 0;
            linked_list_constructor def = {0};
            for (pp_token *temp                    = macro->definition;
                 idx < macro->definition_len; temp = temp->next, ++idx) {
                pp_token *new_token = copy_pp_token(pp, temp);
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

                uint32_t arg_idx  = 0;
                pp_macro_arg *arg = macro->args;
                for (;;) {
                    pp_token *first_arg_token = tok;
                    uint32_t parens_depth     = 0;
                    uint32_t tok_count        = 0;
                    for (;;) {
                        if (tok->kind == PP_TOK_PUNCT &&
                            tok->punct_kind == ')' && parens_depth == 0) {
                            break;
                        } else if (tok->kind == PP_TOK_PUNCT &&
                                   tok->punct_kind == ',' &&
                                   parens_depth == 0) {
                            tok           = tok->next;
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

                if (tok->kind == PP_TOK_PUNCT && tok->punct_kind == ')') {
                    tok = tok->next;
                } else {
                    NOT_IMPL;
                }

                uint32_t idx                = 0;
                linked_list_constructor def = {0};
                for (pp_token *temp                    = macro->definition;
                     idx < macro->definition_len; temp = temp->next, ++idx) {
                    bool is_arg = false;
                    if (temp->kind == PP_TOK_ID) {
                        for (arg = macro->args; arg;
                             arg               = arg->next) {
                            if (string_eq(arg->name, temp->str)) {
                                uint32_t arg_tok_idx = 0;
                                for (pp_token *arg_tok = arg->toks;
                                     arg_tok_idx < arg->tok_count;
                                     arg_tok = arg_tok->next, ++arg_tok_idx) {
                                    pp_token *new_token =
                                        copy_pp_token(pp, arg_tok);
                                    LLISTC_ADD_LAST(&def, new_token);
                                }
                                is_arg = true;
                                break;
                            }
                        }
                    }

                    if (is_arg) {
                        continue;
                    }

                    pp_token *new_token = copy_pp_token(pp, temp);
                    LLISTC_ADD_LAST(&def, new_token);
                }
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
        tok                          = tok->next;
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

    pp_lexer *lex         = bump_alloc(pp->a, sizeof(pp_lexer));
    lex->tok_buf          = pp->lexer_buffer;
    lex->tok_buf_capacity = sizeof(pp->lexer_buffer);
    lex->data             = f->contents.data;
    lex->eof              = STRING_END(f->contents);
    lex->cursor           = lex->data;

    pp_token *tok;
    for (;;) {
        pp_lexer_parse(lex);
        tok  = get_new_token(pp);
        *tok = lex->tok;
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
eval_pp_expr(preprocessor *pp, pp_token **tokp) {
    int64_t result = 0;

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
                /* NOT_IMPL; */
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
                        cursor += fmt_pp_tok(tok, cursor, buf_eof - cursor);
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
                    include_file(pp, &tok, tok->str);
                    tok = tok->next;
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
    while (tok->next) {
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
            uint32_t len     = fmt_token_verbose(c_tok, buffer, sizeof(buffer));
            char *debug_info = aalloc(get_debug_allocator(), len + 1);
            memcpy(debug_info, buffer, len + 1);
            tok->_debug_info = debug_info;
            printf("%s\n", buffer);
        }
#endif
    }
    return (token *)converted.first;
}
