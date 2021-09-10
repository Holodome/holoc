#include "tokenizer.h"

#include "strings.h"

static const char *TOKEN_BASIC_STRS[] = {
    "EOS",
    "Ident",
    "Int",
    "Real",
    "Str"
};  

static const char *KEYWORD_STRS[] = {
    "print",
    "while",
    "return",
    "if",
    "else"
};

static const char *MULTISYMB_STRS[] = {
    "<<=",
    ">>=",
    "::",
    ":=",
    "->",
    "<=",
    ">=",
    "==",
    "!=",
    "<<",
    ">>",
    "+=",
    "-=",
    "&=",
    "|=",
    "^=",
    "%=",
    "/=",
    "*=",
};  


Tokenizer *create_tokenizer(FileID id) {
    const FileData *file_data = get_file_data(id);
    
    Tokenizer *tokenizer = arena_bootstrap(Tokenizer, arena);
    tokenizer->file_id = id;
    tokenizer->buffer = arena_copy(&tokenizer->arena, file_data->data, file_data->size);
    tokenizer->buffer_size = file_data->size;
    tokenizer->line_number = 1;
    tokenizer->symb = *tokenizer->buffer;
    tokenizer->cursor = tokenizer->buffer;
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

static b32 next_eq(Tokenizer *tokenizer, const char *str, uptr *out_len) {
    b32 result = FALSE;
    uptr len = str_len(str);
    if (has_space_for(tokenizer, len)) {
        result = str_eqn((const char *)tokenizer->cursor, str, len);
    }
    if (out_len) {
        *out_len = len;
    }
    return result;
}

static b32 parse(Tokenizer *tokenizer, const char *str) {
    b32 result = FALSE;
    uptr len = 0;
    if (next_eq(tokenizer, str, &len)) {
        while (len--) {
            b32 temp = advance(tokenizer);
            assert(temp);
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
            if (!tokenizer->symb) {
                token->kind = TOKEN_EOS;
                break;
            }
            
            if (parse(tokenizer, "\n") || parse(tokenizer, "\n\r")) {
                ++tokenizer->line_number;
                tokenizer->symb_number = 0;
                continue;
            } else if (is_space(tokenizer->symb)) {
                advance(tokenizer);
                continue;
            } else if (parse(tokenizer, "//")) {
                while (tokenizer->symb != '\n') {
                    advance(tokenizer);
                }
                continue;
            } else if (parse(tokenizer, "/*")) {
                for (;;) {
                    do {
                        advance(tokenizer);
                    } while (tokenizer->symb != '*');
                    if (parse(tokenizer, "*/")) {
                        break;
                    }
                }
                continue;
            }
            
            SourceLocation source_loc;
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
                
                for (u32 i = TOKEN_KW_PRINT, local_i = 0; i <= TOKEN_KW_ELSE; ++i, ++local_i) {
                    if (str_eq(KEYWORD_STRS[local_i], str)) {
                        token->kind = i;
                        break;
                    }
                }
                
                if (!token->kind) {
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
                // check by descending length
                for (u32 i = TOKEN_ILSHIFT, local_i = 0; i <= TOKEN_IMUL; ++i, ++local_i) {
                    if (parse(tokenizer, MULTISYMB_STRS[local_i])) {
                        token->kind = i;
                        break;
                    }
                }
                
                // All unhandled cases before - single character 
                if (!token->kind) {
                    token->kind = tokenizer->symb;
                    advance(tokenizer);
                }
                break;
            } else {
                token->kind = TOKEN_ERROR;
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

uptr fmt_tok_kind(char *buf, uptr buf_sz, u32 kind) {
    uptr result = 0;
    if (IS_TOKEN_ASCII(kind)) {
        result = fmt(buf, buf_sz, "%c", kind);
    } else if (IS_TOKEN_MULTISYMB(kind)) {
        result = fmt(buf, buf_sz, "%s", MULTISYMB_STRS[kind - TOKEN_MULTISYMB]);
    } else if (IS_TOKEN_KEYWORD(kind)) {
        result = fmt(buf, buf_sz, "<kw>%s", KEYWORD_STRS[kind - TOKEN_KEYWORD]);
    } else if (IS_TOKEN_GENERAL(kind)) {
        switch (kind) {
            case TOKEN_EOS: {
                result = fmt(buf, buf_sz, "<EOS>");
            } break;
            case TOKEN_IDENT: {
                result = fmt(buf, buf_sz, "<ident>");
            } break;
            case TOKEN_STR: {
                result = fmt(buf, buf_sz, "<str>");
            } break;
            case TOKEN_INT: {
                result = fmt(buf, buf_sz, "<int>");
            } break;
            case TOKEN_REAL: {
                result = fmt(buf, buf_sz, "<real>");
            } break;
        }
    }
    return result;    
}

uptr fmt_tok(char *buf, uptr buf_sz, Token *token) {
    uptr result = 0;
    if (IS_TOKEN_ASCII(token->kind)) {
        result = fmt(buf, buf_sz, "%c", token->kind);
    } else if (IS_TOKEN_MULTISYMB(token->kind)) {
        result = fmt(buf, buf_sz, "%s", MULTISYMB_STRS[token->kind - TOKEN_MULTISYMB]);
    } else if (IS_TOKEN_KEYWORD(token->kind)) {
        result = fmt(buf, buf_sz, "<kw>%s", KEYWORD_STRS[token->kind - TOKEN_KEYWORD]);
    } else if (IS_TOKEN_GENERAL(token->kind)) {
        switch (token->kind) {
            case TOKEN_EOS: {
                result = fmt(buf, buf_sz, "<EOS>");
            } break;
            case TOKEN_IDENT: {
                result = fmt(buf, buf_sz, "<ident>%s", token->value_str);
            } break;
            case TOKEN_STR: {
                result = fmt(buf, buf_sz, "<str>%s", token->value_str);
            } break;
            case TOKEN_INT: {
                result = fmt(buf, buf_sz, "<int>%lld", token->value_int);
            } break;
            case TOKEN_REAL: {
                result = fmt(buf, buf_sz, "<real>%f", token->value_real);
            } break;
        }
    }
    return result;
}