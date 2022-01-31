#include "pp_token_iterator.h"

#if 0

// Peeks next token from parse stack.
static pp_token *
ps_peek(preprocessor *pp, pp_parse_stack **psp) {
    pp_token *tok = NULL;
    while (!tok && *psp) {
        tok = (*psp)->token_list;
        if (tok) {
            break;
        }

        pp_token local_tok = {0};
        if (!(*psp)->lexer || !pp_lexer_parse((*psp)->lexer, &local_tok)) {
            if ((*psp)->lexer) {
                LLIST_ADD(pp->lex_freelist, (*psp)->lexer);
            }
            pp_parse_stack *entry = *psp;
            LLIST_POP(*psp);
            LLIST_ADD(pp->parse_stack_freelist, entry);
            continue;
        } else {
            tok = NEW_PP_TOKEN(pp);
            memcpy(tok, &local_tok, sizeof(pp_token));
            tok->loc.filename = (*psp)->f->full_path;
            if (tok->str.data) {
                tok->str = string_dup(pp->a, tok->str);
            }
        }

#if HOLOC_DEBUG
        {
            char buffer[4096];
            uint32_t len     = fmt_pp_tok_verbose(buffer, sizeof(buffer), tok);
            char *debug_info = aalloc(get_debug_allocator(), len + 1);
            memcpy(debug_info, buffer, len + 1);
            tok->_debug_info = debug_info;
        }
#endif

        (*psp)->token_list = tok;
    }
    return tok;
}

// Eats current tokens from parse stack, forcing it to move to next one on next
// peek call.
static void
ps_eat(preprocessor *pp, pp_parse_stack *ps) {
    pp_token *tok = ps->token_list;
#if 0
    char buffer[4096];
    fmt_pp_tok_verbose(buffer, sizeof(buffer), tok);
    printf("%s\n", buffer);
#endif
    assert(tok);
    LLIST_POP(ps->token_list);
    DEL_PP_TOKEN(pp, tok);
}

// Wrapper for consequetive eat and peek.
static pp_token *
ps_eat_peek(preprocessor *pp, pp_parse_stack **psp) {
    ps_eat(pp, *psp);
    return ps_peek(pp, psp);
}

// Sets token list to be the next tokens for parse stack. Used when expanding a
// macro.
static void
ps_set_next_toks(preprocessor *pp, pp_parse_stack *ps, pp_token *toks) {
    pp_token *last_tok = 0;
    for (last_tok = toks; last_tok; last_tok = last_tok->next) {
        if (!last_tok->next) {
            break;
        }
    }

    if (last_tok) {
        last_tok->next = ps->token_list;
    }
    ps->token_list = toks;
}

// Peeks next n'th token.
static pp_token *
ps_peek_forward(preprocessor *pp, pp_parse_stack *ps, uint32_t n) {
    uint32_t idx    = 0;
    pp_token **tokp = &ps->token_list;
    while (idx != n) {
        if (*tokp && (*tokp)->next) {
            tokp = &(*tokp)->next;
            ++idx;
            continue;
        }

        pp_token *new_tok    = NEW_PP_TOKEN(pp);
        bool should_continue = pp_lexer_parse(ps->lexer, new_tok);
        (*tokp)->next        = new_tok;
        tokp                 = &(*tokp)->next;
        ++idx;
        if (!should_continue) {
            break;
        }
    }

    pp_token *tok = NULL;
    if (idx == n) {
        tok = *tokp;
    }
    return tok;
}

// Multiple consquetive eats.
static void
ps_eat_multiple(preprocessor *pp, pp_parse_stack *ps, uint32_t count) {
    for (uint32_t i = 0; i < count; ++i) {
        ps_eat(pp, ps);
    }
}
#endif 
