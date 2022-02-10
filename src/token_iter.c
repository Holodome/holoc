#include "token_iter.h"

#include <assert.h>
#include <stdlib.h>

#include "c_lang.h"
#include "preprocessor.h"
#include "str.h"

void
ti_init(token_iter *it, string filename) {
    it->pp = calloc(1, sizeof(preprocessor));
    pp_init(it->pp, filename);
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
        token *tok = calloc(1, sizeof(token));
        char buf[4096];
        uint32_t buf_size = 0;
        pp_parse(it->pp, tok, buf, sizeof(buf), &buf_size);
        if (buf_size) {
            tok->str = string_dup((string){buf, buf_size});
        }
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
        free(tok);
        it->token_list = tok->next;
    }
}

token *
ti_eat_peek(token_iter *it) {
    ti_eat(it);
    return ti_peek(it);
}
