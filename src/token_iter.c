#include "token_iter.h"

#include <assert.h>

#include "allocator.h"
#include "c_lang.h"
#include "preprocessor.h"

static char tok_buf[4096];

void
ti_init(token_iter *it, string filename) {
    it->pp    = aalloc(it->a, sizeof(preprocessor));
    it->pp->a = it->a;
    pp_init(it->pp, filename, tok_buf, sizeof(tok_buf));
}

token *
ti_peek_forward(token_iter *it, uint32_t count) {
    token **tokp = &it->token_list;
    uint32_t idx = 0;
    while (idx < count && *tokp && (*tokp)->next) {
        tokp = &(*tokp)->next;
        ++idx;
    }

    while (idx != count || !*tokp) {
        token *tok = aalloc(it->a, sizeof(token));
        pp_parse(it->pp, tok);
        if (*tokp) {
            (*tokp)->next = tok;
            tokp          = &(*tokp)->next;
            ++idx;
        } else {
            *tokp = tok;
        }
    }

    assert(*tokp);
    return *tokp;
}

token *
ti_peek(token_iter *it) {
    return ti_peek_forward(it, 0);
}

void
ti_eat_multiple(token_iter *it, uint32_t count) {
    for (uint32_t i = 0; i < count; ++i) {
        ti_eat(it);
    }
}

void
ti_eat(token_iter *it) {
    token *tok = it->token_list;
    if (tok) {
        it->token_list = tok->next;
    }
}

token *
ti_eat_peek(token_iter *it) {
    ti_eat(it);
    return ti_peek(it);
}
