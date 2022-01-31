#include "pp_token_iter.h"

#include <assert.h>
#include <stddef.h>
#include <string.h>

#include "allocator.h"
#include "file_storage.h"
#include "freelist.h"
#include "llist.h"
#include "pp_lexer.h"
#include "str.h"

#define NEW_LEXER(_it) FREELIST_ALLOC((_it)->lexer_freelist, (_it)->a)
#define DEL_LEXER(_it, _lex) FREELIST_FREE(*((_it)->lexer_freelist), _lex)

#define NEW_ENTRY(_it) FREELIST_ALLOC(&(_it)->it_freelist, (_it)->a)
#define DEL_ENTRY(_it, _entry) FREELIST_FREE(*((_it)->it_freelist), _entry)

#define NEW_PP_TOKEN(_it) FREELIST_ALLOC(((_it)->token_freelist), (_it)->a)
#define DEL_PP_TOKEN(_it, _tok) FREELIST_FREE(*((_it)->token_freelist), _tok)

void
ppti_include_file(pp_token_iter *it, file_storage *fs, string filename) {
    file *current_file = NULL;
    for (ppti_entry *e = it->it; e; e = e->next) {
        if (e->f) {
            current_file = e->f;
            break;
        }
    }

    file *f = get_file(fs, filename, current_file);
    if (!f) {
        NOT_IMPL;
    }

    ppti_entry *entry = NEW_ENTRY(it);
    entry->f          = f;
    entry->lexer      = NEW_LEXER(it);
    pp_lexer_init(entry->lexer, f->contents.data, STRING_END(f->contents),
                  it->tok_buf, it->tok_buf_capacity);
    LLIST_ADD(it->it, entry);
}

void
ppti_insert_tok_list(pp_token_iter *it, pp_token *first, pp_token *last) {
    ppti_entry *e = it->it;
    if (!e) {
        e = NEW_ENTRY(it);
        LLIST_ADD(it->it, e);
    }

    last->next    = e->token_list;
    e->token_list = first;
}

pp_token *
ppti_peek_forward(pp_token_iter *it, uint32_t count) {
    pp_token *tok = NULL;
    ppti_entry *e = it->it;
    if (e) {
        uint32_t idx    = 0;
        pp_token **tokp = &e->token_list;
        while (idx != count) {
            // First handle the cases where some lookahead tokens are present
            if (*tokp && (*tokp)->next) {
                tokp = &(*tokp)->next;
                ++idx;
                continue;
            }

            if (!e || !e->lexer) {
                break;
            }

            pp_token local_tok = {0};
            bool not_eof       = pp_lexer_parse(e->lexer, &local_tok);
            if (!not_eof) {
                e = e->next;
                continue;
            }

            pp_token *new_tok = NEW_PP_TOKEN(it);
            memcpy(new_tok, &local_tok, sizeof(pp_token));
            (*tokp)->next = new_tok;
            tokp          = &(*tokp)->next;
            ++idx;
        }

        if (idx == count) {
            tok = *tokp;
        }
    }
    return tok;
}

pp_token *
ppti_peek(pp_token_iter *it) {
    return ppti_peek_forward(it, 0);
}

void
ppti_eat_multiple(pp_token_iter *it, uint32_t count) {
    for (uint32_t i = 0; i < count; ++i) {
        ppti_eat(it);
    }
}

void
ppti_eat(pp_token_iter *it) {
    // TODO: Do we want to return success status here?
    ppti_entry *e = it->it;
    if (e) {
        pp_token *tok = e->token_list;
        if (tok) {
            LLIST_POP(e->token_list);
            DEL_PP_TOKEN(it, tok);
        }
    }
}

pp_token *
ppti_eat_peek(pp_token_iter *it) {
    ppti_eat(it);
    return ppti_peek(it);
}

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
            uint32_t len     = fmt_it_tok_verbose(buffer, sizeof(buffer), tok);
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
    fmt_it_tok_verbose(buffer, sizeof(buffer), tok);
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
