#include "tokenizer.h"

Tokenizer create_tokenizer(const void *buffer, uptr buffer_size) {
    Tokenizer tokenizer = {0};
    tokenizer.buffer = arena_copy(&tokenizer.arena, buffer, buffer_size);
    tokenizer.buffer_size = buffer_size;
    tokenizer.symb = *tokenizer.buffer;
    tokenizer.cursor = tokenizer.buffer;
    return tokenizer;
}

void delete_tokenizer(Tokenizer *tokenizer) {
    arena_clear(&tokenizer->arena);
}

static void advance(Tokenizer *tokenizer) {
    ++tokenizer->symb_number;
    ++tokenizer->cursor;
    tokenizer->symb = *tokenizer->cursor;
}

Token *peek_tok(Tokenizer *tokenizer) {
    Token *token = tokenizer->active_token;
    if (!token) {
        token = arena_alloc_struct(&tokenizer->arena, Token);
        tokenizer->active_token = token;

        for (;;) {
            if (tokenizer->cursor - tokenizer->buffer >= tokenizer->buffer_size || 
                    !tokenizer->symb) {
                token->kind = TOKEN_EOS;
            }
        }
    }
    return token;
}

void eat_tok(Tokenizer *tokenizer) {

}
