#include "tokenizer.h"

#include "strings.h"

Tokenizer create_tokenizer(const void *buffer, uptr buffer_size, const char *name) {
    Tokenizer tokenizer = {0};
    tokenizer.buffer = arena_copy(&tokenizer.arena, buffer, buffer_size);
    tokenizer.buffer_size = buffer_size;
    tokenizer.line_number = 1;
    tokenizer.symb = *tokenizer.buffer;
    tokenizer.cursor = tokenizer.buffer;
    tokenizer.source_name = arena_alloc_str(&tokenizer.arena, name);
    return tokenizer;
}

void delete_tokenizer(Tokenizer *tokenizer) {
    arena_clear(&tokenizer->arena);
}

// Returns current index in tokenizer.
// Same can be achieved using separate variable for counting,
// which is less preferable due to increased complexity and neglible perfomace change (single instruction)
static uptr get_buffer_idx(Tokenizer *tokenizer) {
    return tokenizer->cursor - tokenizer->buffer;
}

static b32 has_space_for(Tokenizer *tokenizer, u32 count) {
    return get_buffer_idx(tokenizer) + count < tokenizer->buffer_size;
}

// Safely advance cursor. If end of buffer is reached, 0 is set to tokenizer->symb and  
// false is returned
static b32 advance(Tokenizer *tokenizer) {
    b32 result = TRUE;
    if (has_space_for(tokenizer, 1)) {
        ++tokenizer->symb_number;
        ++tokenizer->cursor;
        tokenizer->symb = *tokenizer->cursor;
    } else {
        tokenizer->symb = 0;
        result = FALSE;
    }
    return result;
}

static b32 next_eq(Tokenizer *tokenizer, const char *str) {
    
}

#define check_multigraph(_tokenizer, _str) check_multigraph_(_tokenizer, _str, sizeof(_str) - 1) 
static void check_multigraph_(Tokenizer *tokenizer, const char *str, uptr len) {
    b32 result = FALSE;
    if (next_eq(tokenizer, str)) {
        while (len--) {
            advance(tokenizer);
        }
        result = TRUE;
    }
    return result;
}


Token *peek_tok(Tokenizer *tokenizer) {
    Token *token = tokenizer->active_token;
    if (!token) {
        token = arena_alloc_struct(&tokenizer->arena, Token);
        tokenizer->active_token = token;

        for (;;) {
            assert(has_space_for(tokenizer, 1));
            if (!tokenizer->symb) {
                token->kind = TOKEN_EOS;
                break;
            }
            
            if (tokenizer->symb == '\n') {
                ++tokenizer->line_number;
                tokenizer->symb_number = 0;
                advance(tokenizer);
                continue;
            } else if (is_space(tokenizer->symb)) {
                advance(tokenizer);
                continue;
            } else if (tokenizer->symb == '#') {
                while (tokenizer->symb != '\n') {
                    advance(tokenizer);
                }
                continue;
            } else if (next_eq)
            
            SourceLocation source_loc;
            source_loc.source_name = tokenizer->source_name;
            source_loc.line = tokenizer->line_number;
            source_loc.symb = tokenizer->symb_number;
            token->source_loc = source_loc;
            if (is_digit(tokenizer->symb)) {
                const u8 *start = tokenizer->cursor;
                b32 is_real = FALSE;
                do {
                    advance(tokenizer);
                    if (tokenizer->symb == '.') {
                        is_real = TRUE;
                    }
                } while (is_digit(tokenizer->symb) || tokenizer->symb == '.');
                uptr len = tokenizer->cursor - start;
                
                if (is_real) {
                    f64 real = str_to_f64((const char *)start, len);
                    token->value_real = real;
                    token->kind = TOKEN_REAL;
                } else {
                    i64 iv = str_to_i64((const char *)start, len);
                    token->value_int = iv;
                    token->kind = TOKEN_INT;
                }
                break;
            } else if (is_alpha(tokenizer->symb)) {
                const u8 *start = tokenizer->cursor;
                do {
                    advance(tokenizer);
                } while (is_ident(tokenizer->symb));
                uptr len = tokenizer->cursor - start;
                
                char *str = arena_alloc(&tokenizer->arena, len + 1);
                mem_copy(str, start, len);
                str[len] = 0;
                
                if (str_eq(str, "print")) {
                    token->kind = TOKEN_KW_PRINT;
                } else if (str_eq(str, "while")) {
                    token->kind = TOKEN_KW_WHILE;
                } else if (str_eq(str, "return")) {
                    token->kind = TOKEN_KW_RETURN;
                } else {
                    token->kind = TOKEN_IDENT;
                    token->value_str = str;
                }
                break;
            } else if (tokenizer->symb == '\"') {
                advance(tokenizer);
                const u8 *start = tokenizer->cursor;
                do {
                    advance(tokenizer);
                } while (tokenizer->symb != '\"');
                uptr len = tokenizer->cursor - start;
                
                char *str = arena_alloc(&tokenizer->arena, len + 1);
                mem_copy(str, start, len);
                str[len] = 0;
                token->kind = TOKEN_STR;
                token->value_str = str;
                break;
            } else if (is_punct(tokenizer->symb)) {
                // Because muttiple operators can be put together (+=-2),
                // first check by descending length
                if (next_eq(tokenizer, "<<=")) {
                    token->kind = TOKEN_ILSHIFT;
                    advance_multiple(tokenizer, 3);
                } else if (next_eq(tokenizer->))
                } else {
                    // All unhandled cases before - single character 
                    token->kind = tokenizer->symb;
                }
                advance(tokenizer);
                break;
            } else {
                token->kind = TOKEN_NONE;
                advance(tokenizer);
                break;
            }
        }
    }
    return token;
}

void eat_tok(Tokenizer *tokenizer) {
    if (tokenizer->active_token) {
        tokenizer->active_token = 0;
    }
}

Token *peek_next_tok(Tokenizer *tokenizer) {
    eat_tok(tokenizer);
    return peek_tok(tokenizer);    
}

void fmt_tok(char *buf, uptr buf_sz, Token *token) {
    if (token->kind < TOKEN_EOS) {
        fmt(buf, buf_sz, "[%c]", token->kind);
    } else {
        switch (token->kind) {
            case TOKEN_EOS: {
                fmt(buf, buf_sz, "[EOS]");
            } break;
            case TOKEN_IDENT: {
                fmt(buf, buf_sz, "[IDENT %s]", token->value_str);
            } break;
            case TOKEN_STR: {
                fmt(buf, buf_sz, "[STR %s]", token->value_str);
            } break;
            case TOKEN_INT: {
                fmt(buf, buf_sz, "[INT %lld]", token->value_int);
            } break;
            case TOKEN_REAL: {
                fmt(buf, buf_sz, "[REAL %f]", token->value_real);
            } break;
        }
    }
}