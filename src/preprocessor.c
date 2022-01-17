#include "preprocessor.h"

#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "allocator.h"
#include "buffer_writer.h"
#include "bump_allocator.h"
#include "c_types.h"
#include "hashing.h"
#include "llist.h"
#include "pp_lexer.h"
#include "str.h"
#include "unicode.h"

static string KEYWORD_STRINGS[] = {
    WRAP_Z("(unknown)"),
    WRAP_Z("auto"),
    WRAP_Z("break"),
    WRAP_Z("case"),
    WRAP_Z("char"),
    WRAP_Z("const"),
    WRAP_Z("continue"),
    WRAP_Z("default"),
    WRAP_Z("do"),
    WRAP_Z("double"),
    WRAP_Z("else"),
    WRAP_Z("enum"),
    WRAP_Z("extern"),
    WRAP_Z("float"),
    WRAP_Z("for"),
    WRAP_Z("goto"),
    WRAP_Z("if"),
    WRAP_Z("inline"),
    WRAP_Z("int"),
    WRAP_Z("long"),
    WRAP_Z("register"),
    WRAP_Z("restrict"),
    WRAP_Z("return"),
    WRAP_Z("short"),
    WRAP_Z("signed"),
    WRAP_Z("unsigned"),
    WRAP_Z("static"),
    WRAP_Z("struct"),
    WRAP_Z("switch"),
    WRAP_Z("typedef"),
    WRAP_Z("union"),
    WRAP_Z("unsigned"),
    WRAP_Z("void"),
    WRAP_Z("volatile"),
    WRAP_Z("while"),
    WRAP_Z("_Alignas"),
    WRAP_Z("_Alignof"),
    WRAP_Z("_Atomic"),
    WRAP_Z("_Bool"),
    WRAP_Z("_Complex"),
    WRAP_Z("_Decimal128"),
    WRAP_Z("_Decimal32"),
    WRAP_Z("_Decimal64"),
    WRAP_Z("_Generic"),
    WRAP_Z("_Imaginary"),
    WRAP_Z("_Noreturn"),
    WRAP_Z("_Static_assert"),
    WRAP_Z("_Thread_local"),
    WRAP_Z("_Pragma"),
};

static string PUNCTUATOR_STRINGS[] = {
    WRAP_Z("(unknown)"), WRAP_Z(">>="), WRAP_Z("<<="), WRAP_Z("+="),
    WRAP_Z("-="),        WRAP_Z("*="),  WRAP_Z("/="),  WRAP_Z("%="),
    WRAP_Z("&="),        WRAP_Z("|="),  WRAP_Z("^="),  WRAP_Z("++"),
    WRAP_Z("--"),        WRAP_Z(">>"),  WRAP_Z("<<"),  WRAP_Z("&&"),
    WRAP_Z("||"),        WRAP_Z("=="),  WRAP_Z("!="),  WRAP_Z("<="),
    WRAP_Z(">="),
};

// FIXME: All copies from lex->string_buf act as if it always was an 1-byte
// aligned array, which in reality it may be not

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

    while (macro->definition) {
        pp_token *tok     = macro->definition;
        macro->definition = tok;
        tok->next         = pp->tok_freelist;
        pp->tok_freelist  = tok;
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

#if 0
static pp_conditional_include *
get_new_cond_incl(preprocessor *pp) {
    pp_conditional_include *incl = pp->incl_freelist;
    if (incl) {
        pp->incl_freelist = incl->next;
        memset(incl, 0, sizeof(*incl));
    } else {
        incl = bump_alloc(pp->a, sizeof(*incl));
    }
    return incl;
}

static pp_macro_expansion_arg *
get_new_expansion_arg(preprocessor *pp) {
    pp_macro_expansion_arg *arg = pp->macro_expansion_arg_freelist;
    if (arg) {
        pp->macro_expansion_arg_freelist = arg->next;
        memset(arg, 0, sizeof(*arg));
    } else {
        arg = bump_alloc(pp->a, sizeof(*arg));
    }
    return arg;
}

static token_stack_entry *
get_new_token_stack_entry(preprocessor *pp) {
    token_stack_entry *entry = pp->token_stack_freelist;
    if (entry) {
        LLIST_POP(pp->token_stack_freelist);
        memset(entry, 0, sizeof(*entry));
    } else {
        entry = bump_alloc(pp->a, sizeof(*entry));
    }
    return entry;
}

static void
define_macro(preprocessor *pp) {
    pp_lexer *lex = pp->lex;
    if (lex->tok_kind != PP_TOK_ID) {
        // TODO: Diagnostic
        assert(false);
    }

    string ident_name = string(lex->tok_buf, lex->tok_buf_size);
    uint32_t hash     = hash_string(ident_name);
    pp_macro **macrop = get_macro(pp, hash);
    if (*macrop) {
        ident_name = (*macrop)->name;
        // Macro is already present
        // TODO: Diagnostic
        assert(false);
        // Free all args and tokens macro is holding
        free_macro_data(pp, *macrop);
        // Delete macro data but keep its next pointer for hash table
        pp_macro *next = (*macrop)->next;
        memset(*macrop, 0, sizeof(**macrop));
        (*macrop)->next = next;
    } else {
        allocator a = bump_get_allocator(pp->a);
        ident_name  = string_dup(&a, ident_name);

        pp_macro *new_macro = get_new_macro(pp);
        new_macro->next     = *macrop;
        *macrop             = new_macro;
    }

    pp_macro *macro = *macrop;
    assert(macro);

    macro->name      = ident_name;
    macro->name_hash = hash;

    pp_lexer_parse(lex);
    if (lex->tok_kind == PP_TOK_PUNCT && lex->tok_punct_kind == '(' &&
        !lex->tok_has_whitespace) {
        pp_macro_arg *args = 0;

        pp_lexer_parse(lex);
        // TODO: This condition can be checked inside the loop body
        while (lex->tok_kind != PP_TOK_PUNCT || lex->tok_punct_kind != ')') {
            if (lex->tok_kind == PP_TOK_PUNCT &&
                lex->tok_punct_kind == PP_TOK_PUNCT_VARARGS) {
                string arg_name   = WRAP_Z("__VA_ARGS__");
                pp_macro_arg *arg = get_new_macro_arg(pp);
                arg->name         = arg_name;
                LLIST_ADD_LAST(args, arg);

                pp_lexer_parse(lex);
                if (lex->tok_kind != ')') {
                    // TODO: Diagnostic
                    assert(false);
                    break;
                }
            } else if (lex->tok_kind == PP_TOK_ID) {
                allocator a       = bump_get_allocator(pp->a);
                string arg_name   = string_memdup(&a, lex->tok_buf);
                pp_macro_arg *arg = get_new_macro_arg(pp);
                arg->name         = arg_name;
                LLIST_ADD_LAST(args, arg);

                pp_lexer_parse(lex);
                if (lex->tok_kind == PP_TOK_PUNCT) {
                    if (lex->tok_punct_kind == ',') {
                        pp_lexer_parse(lex);
                    } else if (lex->tok_punct_kind != ')') {
                        // TODO: Diagnostic
                        assert(false);
                        break;
                    }
                }
            }
        }

        if (lex->tok_kind == PP_TOK_PUNCT && lex->tok_punct_kind == ')') {
            pp_lexer_parse(lex);
        }

        macro->args             = args;
        macro->is_function_like = true;
    }

    while (!lex->tok_at_line_start && lex->tok_kind != PP_TOK_EOF) {
        pp_token *tok = get_new_token(pp);
        init_pp_token_from_lexer(pp, tok);
        LLIST_ADD_LAST(macro->definition, tok);
        tok->next         = macro->definition;
        macro->definition = tok;
    }
}

static void
undef_macro(preprocessor *pp) {
    pp_lexer *lex = pp->lex;
    if (lex->tok_kind != PP_TOK_ID) {
        // TODO: Diagnostic
        assert(false);
    }

    string ident_name = string(lex->tok_buf, lex->tok_buf_size);
    uint32_t hash     = hash_string(ident_name);
    pp_macro **macrop = get_macro(pp, hash);
    if (*macrop) {
        pp_macro *macro = *macrop;
        free_macro_data(pp, macro);
        *macrop            = macro->next;
        macro->next        = pp->macro_freelist;
        pp->macro_freelist = macro;
    } else {
        // TODO: Diagnostic
        assert(false);
    }
}

static void
push_cond_incl(preprocessor *pp, bool is_included) {
    pp_conditional_include *incl = get_new_cond_incl(pp);
    incl->is_included            = is_included;

    incl->next          = pp->cond_incl_stack;
    pp->cond_incl_stack = incl;
}

static void
skip_cond_incl(preprocessor *pp) {
    uint32_t depth = 0;
    pp_lexer *lex  = pp->lex;

    while (lex->tok_kind != PP_TOK_EOF) {
        if (lex->tok_kind == PP_TOK_PUNCT && lex->tok_punct_kind == '#' &&
            lex->tok_at_line_start) {
            pp_lexer_parse(lex);
            if (lex->tok_kind == PP_TOK_ID) {
                string directive = string(lex->tok_buf, lex->tok_buf_size);
                if (string_eq(directive, WRAP_Z("if")) ||
                    string_eq(directive, WRAP_Z("ifdef")) ||
                    string_eq(directive, WRAP_Z("ifndef"))) {
                    ++depth;
                } else if (string_eq(directive, WRAP_Z("elif")) ||
                           string_eq(directive, WRAP_Z("else")) ||
                           string_eq(directive, WRAP_Z("endif"))) {
                    if (!depth) {
                        break;
                    }
                    if (string_eq(directive, WRAP_Z("endif"))) {
                        --depth;
                    }
                }
            }
        } else {
            pp_lexer_parse(lex);
        }
    }

    if (depth) {
        // TODO: Diagnostic
    }
}

static int64_t
eval_if_expr(preprocessor *pp) {
    int64_t result = 0;

    return result;
}

static void
process_pragma(preprocessor *pp) {
    pp_lexer *lex = pp->lex;
    while (!lex->tok_at_line_start) {
        if (lex->tok_kind == TOK_ID && strcmp(lex->tok_buf, "once")) {
            pp_guarded_file *guarded =
                bump_alloc(pp->a, sizeof(pp_guarded_file));
            // TODO:
            (void)guarded;
        }
    }
}

static void
process_pp_directive(preprocessor *pp) {
    pp_lexer *lex = pp->lex;
    // Looping is for conditional includes
    for (;;) {
        // Null directive is a single hash symbol followed by line break
        if (lex->tok_at_line_start) {
            break;
        }

        if (lex->tok_kind != PP_TOK_ID) {
            // TODO: Diagnostic
            assert(false);
            break;
        }

        string directive = string(lex->tok_buf, lex->tok_buf_size);
        if (string_eq(directive, WRAP_Z("include"))) {
            // TODO:
            break;
        } else if (string_eq(directive, WRAP_Z("define"))) {
            pp_lexer_parse(lex);
            define_macro(pp);
            break;
        } else if (string_eq(directive, WRAP_Z("undef"))) {
            pp_lexer_parse(lex);
            undef_macro(pp);
            break;
        } else if (string_eq(directive, WRAP_Z("if"))) {
            pp_lexer_parse(lex);
            int64_t expr = eval_if_expr(pp);
            push_cond_incl(pp, expr != 0);
            if (expr) {
                break;
            } else {
                skip_cond_incl(pp);
            }
        } else if (string_eq(directive, WRAP_Z("elif"))) {
            pp_lexer_parse(lex);
            int64_t expr                 = eval_if_expr(pp);
            pp_conditional_include *incl = pp->cond_incl_stack;
            if (!incl) {
                // TODO: Diagnostic
            } else {
                if (!incl->is_included && expr) {
                    incl->is_included = true;
                } else {
                    skip_cond_incl(pp);
                }
            }
        } else if (string_eq(directive, WRAP_Z("else"))) {
            pp_conditional_include *incl = pp->cond_incl_stack;
            if (!incl) {
                // TODO: Diagnostic
            } else {
                if (incl->is_after_else) {
                    // TODO: Diagnostic
                }
                incl->is_after_else = true;
                if (incl->is_included) {
                    skip_cond_incl(pp);
                    continue;
                }
                break;
            }
        } else if (string_eq(directive, WRAP_Z("endif"))) {
            pp_lexer_parse(lex);
            pp_conditional_include *incl = pp->cond_incl_stack;
            if (!incl) {
                // TODO: Diagnostic
            } else {
                pp->cond_incl_stack = incl->next;
                incl->next          = pp->incl_freelist;
                pp->incl_freelist   = incl;
            }
            break;
        } else if (string_eq(directive, WRAP_Z("ifdef"))) {
            pp_lexer_parse(lex);
            if (lex->tok_kind != PP_TOK_ID) {
                // TODO: Diagnostic
                assert(false);
            }
            uint32_t macro_name_hash =
                hash_string(string(lex->tok_buf, lex->tok_buf_size));
            bool is_defined = (*get_macro(pp, macro_name_hash) != 0);
            push_cond_incl(pp, is_defined);
            if (!is_defined) {
                skip_cond_incl(pp);
                continue;
            }
        } else if (string_eq(directive, WRAP_Z("ifndef"))) {
            pp_lexer_parse(lex);
            if (lex->tok_kind != PP_TOK_ID) {
                // TODO: Diagnostic
                assert(false);
            }
            uint32_t macro_name_hash =
                hash_string(string(lex->tok_buf, lex->tok_buf_size));
            bool is_defined = (*get_macro(pp, macro_name_hash) != 0);
            push_cond_incl(pp, !is_defined);
            if (is_defined) {
                skip_cond_incl(pp);
                continue;
            }
        } else if (string_eq(directive, WRAP_Z("line"))) {
            // Ignored
        } else if (string_eq(directive, WRAP_Z("pragma"))) {
            pp_lexer_parse(lex);
            process_pragma(pp);
        } else if (string_eq(directive, WRAP_Z("error"))) {
        } else {
            // TODO: Diagnostic
        }
    }
}
static void
push_token_to_stack(preprocessor *pp, token tok) {
    token_stack_entry *entry = get_new_token_stack_entry(pp);
    entry->tok               = tok;
    LLIST_ADD_LAST(pp->token_stack, entry);
}

static void
free_expansion_args(preprocessor *pp, pp_macro_expansion_arg *args) {
    for (pp_macro_expansion_arg *arg = args; arg; arg = arg->next) {
        for (pp_token *token = arg->tokens; token; token = token->next) {
            if (!token->next) {
                token->next      = pp->tok_freelist;
                pp->tok_freelist = arg->tokens;
            }
        }

        if (!arg->next) {
            arg->next                        = pp->macro_expansion_arg_freelist;
            pp->macro_expansion_arg_freelist = args;
        }
    }
}

static pp_macro_expansion_arg *
collect_macro_call_arguments(preprocessor *pp, pp_token **tokp) {
    pp_macro_expansion_arg *new_args = 0;
    pp_token *tok                    = *tokp;

    uint32_t parens_depth               = 0;
    pp_macro_expansion_arg *current_arg = 0;
    for (;;) {
        if (tok->kind == PP_TOK_PUNCT && tok->punct_kind == ')' &&
            parens_depth == 0) {
            LLIST_ADD_LAST(new_args, current_arg);
            break;
        }

        if (!current_arg) {
            current_arg = get_new_expansion_arg(pp);
        }

        if (tok->kind == PP_TOK_PUNCT && tok->punct_kind == ',') {
            LLIST_ADD_LAST(new_args, current_arg);
            parens_depth = 0;
            tok          = tok->next;
            continue;
        }

        if (tok->kind == PP_TOK_PUNCT) {
            if (tok->punct_kind == '(') {
                ++parens_depth;
            } else if (tok->punct_kind == ')') {
                if (parens_depth) {
                    --parens_depth;
                } else {
                    // TODO: Diagnostic
                }
            }
        }

        pp_token *token = get_new_token(pp);
        init_pp_token_from_lexer(pp, token);
        LLIST_ADD_LAST(current_arg->tokens, token);

        tok = tok->next;
    }

    *tokp = tok;
    return new_args;
}

static void expand_macro_internal(preprocessor *pp, pp_macro *macro,
                                  pp_macro_expansion_arg *arg_values);

static void
process_pp_token(preprocessor *pp, pp_token **tokp) {
    pp_token *tok = *tokp;

    if (tok->kind == PP_TOK_ID) {
        string name           = tok->str;
        uint32_t name_hash    = hash_string(name);
        pp_macro **new_macrop = get_macro(pp, name_hash);
        if (*new_macrop) {
            pp_macro *new_macro = *new_macrop;
            if (new_macro->is_function_like && tok->next &&
                tok->next->kind == TOK_PUNCT && tok->next->punct_kind == '(' &&
                !tok->next->has_spaces) {
                pp_macro_expansion_arg *new_args =
                    collect_macro_call_arguments(pp, &tok);
                expand_macro_internal(pp, new_macro, new_args);
                free_expansion_args(pp, new_args);
                goto out;
            } else if (!new_macro->is_function_like) {
                expand_macro_internal(pp, new_macro, 0);
                goto out;
            }
        }
    }

    // If we reached this point, identifier is just a regular token
    push_token_to_stack(pp, convert_pp_token(pp, tok));
out:
    *tokp = tok;
}

static void
expand_macro_internal(preprocessor *pp, pp_macro *macro,
                      pp_macro_expansion_arg *arg_values) {
    for (pp_token *tok = macro->definition; tok; tok = tok->next) {
        string name = tok->str;
        // First, check if identifier corresponds to some argument
        bool is_arg      = false;
        uint32_t arg_idx = 0;
        for (pp_macro_arg *arg = macro->args; arg; arg = arg->next, ++arg_idx) {
            if (string_eq(arg->name, name)) {
                is_arg = true;
                break;
            }
        }

        if (is_arg) {
            uint32_t idx = 0;
            for (pp_macro_expansion_arg *arg = arg_values; arg;
                 arg                         = arg->next, ++idx) {
                if (idx == arg_idx) {
                    pp_token *temp_tok = arg->tokens;
                    while (temp_tok) {
                        process_pp_token(pp, &temp_tok);
                    }
                }
            }
            continue;
        }

        process_pp_token(pp, &tok);
    }
}

static bool
expand_macro(preprocessor *pp) {
    bool result = false;

    pp_lexer *lex = pp->lex;
    if (lex->tok_kind == PP_TOK_ID) {
        string name        = string(lex->tok_buf, lex->tok_buf_size);
        uint32_t name_hash = hash_string(name);
        pp_macro **macrop  = get_macro(pp, name_hash);
        if (*macrop) {
            pp_macro *macro = *macrop;
            if (macro->is_function_like) {
                pp_lexer_parse(lex);
                if (lex->tok_kind == PP_TOK_PUNCT &&
                    lex->tok_punct_kind == '(' && !lex->tok_has_whitespace) {
                    pp_lexer_parse(lex);
                    pp_macro_expansion_arg *args = 0;

                    uint32_t parens_depth               = 0;
                    pp_macro_expansion_arg *current_arg = 0;
                    for (;;) {
                        if (lex->tok_kind == PP_TOK_PUNCT &&
                            lex->tok_punct_kind == ')' && parens_depth == 0) {
                            current_arg->next = args;
                            args              = current_arg;
                            break;
                        }

                        if (!current_arg) {
                            current_arg = get_new_expansion_arg(pp);
                        }

                        if (lex->tok_kind == PP_TOK_PUNCT &&
                            lex->tok_punct_kind == ',') {
                            current_arg->next = args;
                            args              = current_arg;
                            pp_lexer_parse(lex);
                            parens_depth = 0;
                            continue;
                        }

                        if (lex->tok_kind == PP_TOK_PUNCT) {
                            if (lex->tok_punct_kind == '(') {
                                ++parens_depth;
                            } else if (lex->tok_punct_kind == ')') {
                                if (parens_depth) {
                                    --parens_depth;
                                } else {
                                    // TODO: Diagnostic
                                }
                            }
                        }

                        pp_token *token = get_new_token(pp);
                        init_pp_token_from_lexer(pp, token);
                        token->next         = current_arg->tokens;
                        current_arg->tokens = token;

                        pp_lexer_parse(lex);
                    }

                    expand_macro_internal(pp, macro, args);
                    free_expansion_args(pp, args);
                    result = true;
                } else {
                    push_token_to_stack(
                        pp, (token){.kind = TOK_ID,
                                    .str  = string_dup(pp->ea, name)});
                }
            } else {
                expand_macro_internal(pp, macro, 0);
                result = true;
            }
        }
    }

    return result;
}

token
pp_get_token(preprocessor *pp) {
    token tok     = {0};
    pp_lexer *lex = pp->lex;

    for (;;) {
        if (pp->token_stack) {
            token_stack_entry *entry = pp->token_stack;
            tok                      = entry->tok;
            LLIST_POP(pp->token_stack);
            LLIST_ADD(pp->token_stack_freelist, entry);
            break;
        }

        pp_lexer_parse(lex);
        if (lex->tok_kind == PP_TOK_PUNCT && lex->tok_punct_kind == '#' &&
            lex->tok_at_line_start) {
            pp_lexer_parse(lex);
            process_pp_directive(pp);
            continue;
        }
        printf("here\n");

        if (expand_macro(pp)) {
            continue;
        }

        pp_token pp_tok = {0};
        init_pp_token_from_lexer(pp, &pp_tok);
        tok = convert_pp_token(pp, &pp_tok);
        break;
    }

    return tok;
}

#endif

static token
convert_pp_token(preprocessor *pp, pp_token *pp_tok) {
    token tok = {0};
    switch (pp_tok->kind) {
    case PP_TOK_EOF: {
        tok.kind = TOK_EOF;
    } break;
    case PP_TOK_ID: {
        tok.kind = TOK_ID;
        tok.str  = string_dup(pp->ea, pp_tok->str);
    } break;
    case PP_TOK_NUM: {
        tok.kind = TOK_NUM;
    } break;
    case PP_TOK_STR: {
        // First, deduce the type
        switch (pp_tok->str_kind) {
        default:
            assert(false);
            break;
        case PP_TOK_STR_SCHAR:
            tok.type = make_array_type(get_standard_type(C_TYPE_CHAR),
                                       pp_tok->str.len + 1, pp->ea);
            break;
        case PP_TOK_STR_SUTF8:
            tok.type = make_array_type(get_standard_type(C_TYPE_UCHAR),
                                       pp_tok->str.len + 1, pp->ea);
            break;
        case PP_TOK_STR_SUTF16:
            // FIXME: Array size
            tok.type = make_array_type(get_standard_type(C_TYPE_CHAR16),
                                       pp_tok->str.len + 1, pp->ea);
            break;
        case PP_TOK_STR_SUTF32:
            // FIXME: Array size
            tok.type = make_array_type(get_standard_type(C_TYPE_CHAR32),
                                       pp_tok->str.len + 1, pp->ea);
            break;
        case PP_TOK_STR_SWIDE:
            // FIXME: Array size
            tok.type = make_array_type(get_standard_type(C_TYPE_WCHAR),
                                       pp_tok->str.len + 1, pp->ea);
            break;
        case PP_TOK_STR_CCHAR:
            tok.type = get_standard_type(C_TYPE_CHAR);
            break;
        case PP_TOK_STR_CUTF8:
            tok.type = get_standard_type(C_TYPE_UCHAR);
            break;
        case PP_TOK_STR_CUTF16:
            tok.type = get_standard_type(C_TYPE_CHAR16);
            break;
        case PP_TOK_STR_CUTF32:
            tok.type = get_standard_type(C_TYPE_CHAR32);
            break;
        case PP_TOK_STR_CWIDE:
            tok.type = get_standard_type(C_TYPE_WCHAR);
            break;
        }

        switch (pp_tok->str_kind) {
        default:
            assert(false);
            break;
        case PP_TOK_STR_SCHAR:
        case PP_TOK_STR_SUTF8:
        case PP_TOK_STR_SUTF16:
        case PP_TOK_STR_SUTF32:
        case PP_TOK_STR_SWIDE: {
            uint32_t byte_stride = tok.type->size;

            uint32_t len = 0;
            char *cursor = pp_tok->str.data;
            while (*cursor) {
                uint32_t cp;
                cursor = utf8_decode(cursor, &cp);
                ++len;
            }
            void *buffer       = aalloc(pp->ea, byte_stride * (len + 1));
            void *write_cursor = buffer;
            cursor             = pp_tok->str.data;
            while (*cursor) {
                uint32_t cp;
                cursor = utf8_decode(cursor, &cp);
                if (byte_stride == 1) {
                    // TODO: If resulting string is utf8 we can skip decodind
                    // and encoding
                    write_cursor = utf8_encode(write_cursor, cp);
                } else if (byte_stride == 2) {
                    write_cursor = utf16_encode(write_cursor, cp);
                } else if (byte_stride == 4) {
                    memcpy(write_cursor, &cp, 4);
                    write_cursor = (char *)write_cursor + 4;
                } else {
                    assert(false);
                }
            }
            uint32_t zero = 0;
            memcpy(write_cursor, &zero, byte_stride);

            tok.kind = TOK_STR;
            tok.str  = string(buffer, byte_stride * len);
        } break;
        case PP_TOK_STR_CCHAR:
        case PP_TOK_STR_CUTF8:
        case PP_TOK_STR_CUTF16:
        case PP_TOK_STR_CUTF32:
        case PP_TOK_STR_CWIDE: {
            // Treat all character literals as mutltibyte.
            // Convert it to number
            uint64_t value       = 0;
            char *cursor         = pp_tok->str.data;
            uint32_t byte_stride = tok.type->size;
            while (*cursor) {
                uint32_t cp;
                cursor = utf8_decode(cursor, &cp);
                if (cp >= 1 << (8 * byte_stride)) {
                    // TODO: Diagnostic
                    break;
                }
                value = (value << (8 * byte_stride)) | cp;
            }
            // Make sure the value is in expected range
            value &= (1 << (8 * byte_stride)) - 1;
            tok.kind       = TOK_NUM;
            tok.uint_value = value;
        } break;
        }
    } break;
    case PP_TOK_PUNCT: {
        tok.kind = TOK_PUNCT;
        if (pp_tok->punct_kind < 0x100) {
            tok.punct = (uint32_t)pp_tok->punct_kind;
        } else {
            switch (pp_tok->punct_kind) {
            default:
                assert(false);
            case PP_TOK_PUNCT_IRSHIFT:
                tok.punct = C_PUNCT_IRSHIFT;
                break;
            case PP_TOK_PUNCT_ILSHIFT:
                tok.punct = C_PUNCT_ILSHIFT;
                break;
            case PP_TOK_PUNCT_IADD:
                tok.punct = C_PUNCT_IADD;
                break;
            case PP_TOK_PUNCT_ISUB:
                tok.punct = C_PUNCT_ISUB;
                break;
            case PP_TOK_PUNCT_IMUL:
                tok.punct = C_PUNCT_IMUL;
                break;
            case PP_TOK_PUNCT_IDIV:
                tok.punct = C_PUNCT_IDIV;
                break;
            case PP_TOK_PUNCT_IMOD:
                tok.punct = C_PUNCT_IMOD;
                break;
            case PP_TOK_PUNCT_IAND:
                tok.punct = C_PUNCT_IAND;
                break;
            case PP_TOK_PUNCT_IOR:
                tok.punct = C_PUNCT_IOR;
                break;
            case PP_TOK_PUNCT_IXOR:
                tok.punct = C_PUNCT_IXOR;
                break;
            case PP_TOK_PUNCT_INC:
                tok.punct = C_PUNCT_INC;
                break;
            case PP_TOK_PUNCT_DEC:
                tok.punct = C_PUNCT_DEC;
                break;
            case PP_TOK_PUNCT_RSHIFT:
                tok.punct = C_PUNCT_RSHIFT;
                break;
            case PP_TOK_PUNCT_LSHIFT:
                tok.punct = C_PUNCT_LSHIFT;
                break;
            case PP_TOK_PUNCT_LAND:
                tok.punct = C_PUNCT_LAND;
                break;
            case PP_TOK_PUNCT_LOR:
                tok.punct = C_PUNCT_LOR;
                break;
            case PP_TOK_PUNCT_EQ:
                tok.punct = C_PUNCT_EQ;
                break;
            case PP_TOK_PUNCT_NEQ:
                tok.punct = C_PUNCT_NEQ;
                break;
            case PP_TOK_PUNCT_LEQ:
                tok.punct = C_PUNCT_LEQ;
                break;
            case PP_TOK_PUNCT_GEQ:
                tok.punct = C_PUNCT_GEQ;
                break;
            }
        }
    } break;
    case PP_TOK_OTHER: {
        assert(false);
    } break;
    }
    return tok;
}

void
do_pp(preprocessor *pp) {
    linked_list_constructor tokens = {0};

    pp_token *tok = 0;
    for (;;) {
        pp_lexer_parse(pp->lex);
        tok  = get_new_token(pp);
        *tok = pp->lex->tok;
        if (tok->str.data) {
            allocator a = bump_get_allocator(pp->a);
            tok->str    = string_dup(&a, tok->str);
        }

        LLISTC_ADD_LAST(&tokens, tok);
        if (tok->kind == PP_TOK_EOF) {
            break;
        }
    }
}

static string
get_kw_str(c_keyword_kind kind) {
    assert(kind < (sizeof(KEYWORD_STRINGS) / sizeof(*KEYWORD_STRINGS)));
    return KEYWORD_STRINGS[kind];
}

// NOTE: Only called for multisymbol punctuators (ot ASCII ones)
static string
get_punct_str(c_punct_kind kind) {
    assert(kind > 0x100);
    assert((kind - 0x100) <
           (sizeof(PUNCTUATOR_STRINGS) / sizeof(*PUNCTUATOR_STRINGS)));
    return PUNCTUATOR_STRINGS[kind - 0x100];
}

uint32_t
fmt_token(token *tok, char *buf, uint32_t buf_size) {
    buffer_writer w = {buf, buf + buf_size};
    switch (tok->kind) {
    case TOK_EOF:
        break;
    case TOK_ID:
        buf_write(&w, "%.*s", tok->str.len, tok->str.data);
        break;
    case TOK_NUM:
        if (c_type_is_int(tok->type->kind)) {
            buf_write(&w, "%llu", tok->uint_value);
            switch (tok->type->kind) {
            default:
                break;
            case C_TYPE_ULLINT:
            case C_TYPE_SLLINT:
                buf_write(&w, "ll");
                break;
            case C_TYPE_ULINT:
            case C_TYPE_SLINT:
                buf_write(&w, "l");
                break;
            }
            if (c_type_is_int_unsigned(tok->type->kind)) {
                buf_write(&w, "u");
            }
        } else {
            buf_write(&w, "%Lg", tok->float_value);
            switch (tok->type->kind) {
            default:
                break;
            case C_TYPE_FLOAT:
                buf_write(&w, "f");
                break;
            case C_TYPE_LDOUBLE:
                buf_write(&w, "l");
                break;
            }
        }
        break;
    case TOK_STR:
        assert(tok->type->kind == C_TYPE_ARRAY);
        switch (tok->type->ptr_to->kind) {
        default:
            buf_write(&w, "\"");
            break;
        case C_TYPE_WCHAR:
            buf_write(&w, "L\"");
            break;
        case C_TYPE_UCHAR:
            buf_write(&w, "u8\"");
            break;
        case C_TYPE_CHAR16:
            buf_write(&w, "u\"");
            break;
        case C_TYPE_CHAR32:
            buf_write(&w, "U\"");
            break;
        }
        buf_write(&w, "%.*s", tok->str.len, tok->str.data);
        buf_write(&w, "\"");
        break;
    case TOK_PUNCT:
        if (tok->punct < 0x100) {
            buf_write(&w, "%c", tok->punct);
        } else {
            string punct_str = get_punct_str(tok->punct);
            buf_write(&w, "%.*s", punct_str.len, punct_str.data);
        }
        break;
    case TOK_KW: {
        string kw_str = get_kw_str(tok->kw);
        buf_write(&w, "%.*s", kw_str.len, kw_str.data);
    } break;
    }
    return w.cursor - buf;
}

uint32_t
fmt_tok_verbose(token *tok, char *buf, uint32_t buf_size) {
    uint32_t result = 0;
    switch (tok->kind) {
    case TOK_EOF:
        result = snprintf(buf, buf_size, "<EOF>");
        break;
    case TOK_ID:
        result = snprintf(buf, buf_size, "<ID>");
        break;
    case TOK_NUM:
        result = snprintf(buf, buf_size, "<Num>");
        break;
    case TOK_STR:
        result = snprintf(buf, buf_size, "<Str>");
        break;
    case TOK_PUNCT:
        result = snprintf(buf, buf_size, "<Punct>");
        break;
    case TOK_KW:
        result = snprintf(buf, buf_size, "<Kw>");
        break;
    }
    buf += result;
    buf_size -= result;
    result += fmt_token(tok, buf, buf_size);
    return result;
}
