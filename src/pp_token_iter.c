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
    assert(first && last);

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
        for (;;) {
            if (*tokp && count == idx) {
                break;
            }

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
                if (e && e->token_list) {
                    if (*tokp) {
                        (*tokp)->next = e->token_list;
                        tokp          = &(*tokp)->next;
                    } else {
                        *tokp = e->token_list;
                    }
                    e->token_list = NULL;
                }
                continue;
            }

            pp_token *new_tok = NEW_PP_TOKEN(it);
            memcpy(new_tok, &local_tok, sizeof(pp_token));
            new_tok->loc.filename = e->f->name;
            if (new_tok->str.data) {
                new_tok->str = string_dup(it->a, new_tok->str);
            }
#if HOLOC_DEBUG
            {
                char buffer[4096];
                uint32_t len =
                    fmt_pp_tok_verbose(buffer, sizeof(buffer), new_tok);
                char *debug_info = aalloc(get_debug_allocator(), len + 1);
                memcpy(debug_info, buffer, len + 1);
                new_tok->_debug_info = debug_info;
            }
#endif

            if (*tokp) {
                assert(!(*tokp)->next);
                (*tokp)->next = new_tok;
                tokp          = &(*tokp)->next;
                ++idx;
            } else {
                *tokp = new_tok;
            }
        }

        if (idx == count) {
            tok = *tokp;
        }
    }

    if (!tok) {
        tok = it->eof_token;
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

