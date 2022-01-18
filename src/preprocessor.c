#include "preprocessor.h"

#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "allocator.h"
#include "bump_allocator.h"
#include "c_lang.h"
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
            uint32_t idx      = 0;
            pp_token *initial = tok;
            for (pp_token *temp                    = macro->definition;
                 idx < macro->definition_len; temp = temp->next, ++idx) {
                pp_token *new_token = copy_pp_token(pp, temp);
                LLIST_ADD(tok, new_token);
            }
            tok->next           = initial->next;
            tok->has_whitespace = initial->has_whitespace;
            tok->at_line_start  = initial->at_line_start;
            result              = true;
        } break;
        case PP_MACRO_FUNC:
            assert(false);
            break;
        case PP_MACRO_FILE:
            assert(false);
            break;
        case PP_MACRO_LINE:
            assert(false);
            break;
        case PP_MACRO_COUNTER:
            assert(false);
            break;
        case PP_MACRO_TIMESTAMP:
            assert(false);
            break;
        case PP_MACRO_BASE_FILE:
            assert(false);
            break;
        case PP_MACRO_DATE:
            assert(false);
            break;
        case PP_MACRO_TIME:
            assert(false);
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
        assert(false);
    }

    string macro_name        = tok->str;
    uint32_t macro_name_hash = hash_string(macro_name);
    pp_macro **macrop        = get_macro(pp, macro_name_hash);
    if (*macrop) {
        // TODO: Diagnostic
        assert(false);
    } else {
        pp_macro *macro = get_new_macro(pp);
        LLIST_ADD(*macrop, macro);
    }

    pp_macro *macro = *macrop;
    assert(macro);

    macro->name      = macro_name;
    macro->name_hash = macro_name_hash;

    tok               = tok->next;
    macro->definition = tok;
    while (!tok->at_line_start) {
        ++macro->definition_len;
        tok = tok->next;
    }

    *tokp = tok;
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
            }
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
preprocess(preprocessor *pp, pp_token *tok) {
    while (tok->kind != PP_TOK_EOF) {
        if (expand_macro(pp, &tok)) {
            continue;
        }

        if (process_pp_directive(pp, &tok)) {
            continue;
        }

        token c_tok = {0};
        if (convert_pp_token(tok, &c_tok, pp->ea)) {
            char buffer[4096];
            fmt_token_verbose(&c_tok, buffer, sizeof(buffer));
            printf("%s\n", buffer);
        } else {
            assert(false);
        }
        tok = tok->next;
    }
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

    pp_token *token_list = tokens.first;
    preprocess(pp, token_list);
    /* for (tok = token_list; tok; tok = tok->next) { */
    /*     token c_tok = {0}; */
    /*     if (convert_pp_token(tok, &c_tok, pp->ea)) { */
    /*         char buffer[4096]; */
    /*         fmt_token_verbose(&c_tok, buffer, sizeof(buffer)); */
    /*         printf("%s\n", buffer); */
    /*     } */
    /* } */
}

