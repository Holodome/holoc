#include "token_iter.h"

#include <assert.h>

#include "c_lang.h"

token *
ti_peek_forward(token_iter *it, uint32_t count) {
    (void)it;
    (void)count;
    token *tok = 0;
    NOT_IMPL;
    return tok;
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
    (void)it;
    NOT_IMPL;
}

token *
ti_eat_peek(token_iter *it) {
    ti_eat(it);
    return ti_peek(it);
}
