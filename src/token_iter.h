#ifndef TOKEN_ITER_H
#define TOKEN_ITER_H

#include "types.h"

struct token;
struct allocator;
struct preprocessor;

typedef struct token_iter {
    struct allocator *a;
    struct token *token_list;
    struct preprocessor *pp;
} token_iter;

struct token *ti_peek_forward(token_iter *it, uint32_t count);
struct token *ti_peek(token_iter *it);
void ti_eat_multiple(token_iter *it, uint32_t count);
void ti_eat(token_iter *it);
struct token *ti_eat_peek(token_iter *it);

#endif
