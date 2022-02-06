// Defines tool used for iterating tokens in parser. Tokens are generated with preprocessor.
// Tokens are processed on demand of peek() function. To peek next token, last one must be eaten
// with eat() function.
// Tokens that are eaten have their memory freed and become unavailable for future use.
// If any information about token must persist, it should be read and stored elsewhere before eating.
#ifndef TOKEN_ITER_H
#define TOKEN_ITER_H

#include "general.h"

struct token;
struct allocator;
struct preprocessor;

typedef struct token_iter {
    struct allocator *a;

    struct token *token_list;
    struct preprocessor *pp;

    string filename;
} token_iter;

// Initializes iterator to process the 'filename' file.
void ti_init(token_iter *it, string filename);
// Peeks 'nth' token.
struct token *ti_peek_forward(token_iter *it, uint32_t nth);
// Peeks next token
struct token *ti_peek(token_iter *it);
// Eats 'count' tokens
void ti_eat_multiple(token_iter *it, uint32_t count);
// Eats one token
void ti_eat(token_iter *it);
// Wrapper for consecutive calls of eat and peek.
struct token *ti_eat_peek(token_iter *it);

#endif
