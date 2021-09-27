#include "tokenizer.h"

#include "lib/strings.h"

static const char *KEYWORD_STRS[] = {
    "print",
    "while",
    "return",
    "if",
    "else",
    "int",
    "float"
};

static const char *MULTISYMB_STRS[] = {
    "<<=",
    ">>=",
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
    "%=",
    "&&",
    "||"
};  

b32 is_token_assign(u32 tok) {
    return tok == '=' || tok == TOKEN_IADD || tok == TOKEN_ISUB || tok == TOKEN_IMUL ||
        tok == TOKEN_IDIV || tok == TOKEN_IMOD || tok == TOKEN_IAND || tok == TOKEN_IOR ||
        tok == TOKEN_IXOR || tok == TOKEN_ILSHIFT || tok == TOKEN_IRSHIFT; 
}

Tokenizer *
create_tokenizer(ErrorReporter *er, StringStorage *ss, InStream *st, FileID file) {
    Tokenizer *tr = arena_bootstrap(Tokenizer, arena);
    tr->st = st;
    tr->curr_loc.symb = 1;
    tr->curr_loc.line = 1;
    tr->curr_loc.file = file;
    tr->scratch_buffer_size = TOKENIZER_DEFAULT_SCRATCH_BUFFER_SIZE;
    tr->scratch_buffer = arena_alloc(&tr->arena, tr->scratch_buffer_size);
    tr->ss = ss;
    tr->er = er;
    return tr;
}

void destroy_tokenizer(Tokenizer *tr) {
    arena_clear(&tr->arena);
}

// Safely advance cursor. If end of buffer is reached, 0 is set to tr->symb and  
// false is returned
static b32 advance(Tokenizer *tr, u32 n) {
    b32 result = in_stream_advance(tr->st, n);
    tr->curr_loc.symb += n;
    return result;
}

static b32 parse(Tokenizer *tr, const char *str) {
    b32 result = FALSE;
    uptr len = str_len(str);
    assert(len < tr->scratch_buffer_size);
    if (in_stream_peek(tr->st, tr->scratch_buffer, len) == len) {
        result = mem_eq(tr->scratch_buffer, str, len);
        if (result) {
            advance(tr, len);
        }
    }
    
    return result;
}

Token *peek_tok(Tokenizer *tr) {
    Token *token = tr->active_token;
    if (token) {
        return token;
    }
    
    token = arena_alloc_struct(&tr->arena, Token);
    tr->active_token = token;

    for (;;) {
        u8 symb = in_stream_peek_b_or_zero(tr->st);
        if (!symb) {
            token->kind = TOKEN_EOS;
            break;
        }
        
        if (parse(tr, "\n\r") || parse(tr, "\n")) {
            ++tr->curr_loc.line;
            tr->curr_loc.symb = 1;
            continue;
        } else if (is_space(symb)) {
            advance(tr, 1);
            continue;
        } else if (parse(tr, "//")) {
            for (;;) {
                u8 peeked = in_stream_peek_b_or_zero(tr->st);
                if (!peeked || peeked == '\n') {
                    break;
                }
                advance(tr, 1);
            }
            continue;
        } else if (parse(tr, "/*")) {
            for (;;) {
                for (;;) {
                    u8 peeked = in_stream_peek_b_or_zero(tr->st);
                    if (!peeked || peeked == '*') {
                        break;
                    }
                    advance(tr, 1);
                }
                // @NOTE(hl): This is rarely hit path in case other than correct comment end,
                //  so additional peeking is not a problem here
                if (parse(tr, "*/") || !(in_stream_peek_b_or_zero(tr->st))) {
                    break;
                }
            }
            continue;
        }
        
        token->src_loc = tr->curr_loc;
        if (is_digit(symb)) {
            // @TODO warn on large ident lengths
            char *lit = (char *)tr->scratch_buffer;
            u32 lit_len = 0;
            b32 is_real = FALSE;
            for (;;) {
                // @TODO Provide handling for scientific notation
                u8 peeked = 0;
                if (!in_stream_peek(tr->st, &peeked, 1)) {
                    break;
                }
                if (!is_digit(peeked) || !peeked) {
                    break;
                }
                if (peeked == '.') {
                    is_real = TRUE;
                }
                assert(lit_len < sizeof(lit));
                lit[lit_len++] = peeked;
                
                b32 advanced = advance(tr, 1);
                assert(advanced);
            }
            assert(lit_len < tr->scratch_buffer_size);
            lit[lit_len] = 0;
            
            if (is_real) {
                f64 real = str_to_f64(lit);
                token->value_real = real;
                token->kind = TOKEN_REAL;
            } else {
                i64 iv = str_to_i64(lit);
                token->value_int = iv;
                token->kind = TOKEN_INT;
            }
            break;
        } else if (is_ident_start(symb)) {
            char *lit = (char *)tr->scratch_buffer;
            u32 lit_len = 0;
            for (;;) {
                u8 peeked = 0;
                if (!in_stream_peek(tr->st, &peeked, 1)) {
                    break;
                }
                if (!is_ident(peeked) || !peeked) {
                    break;
                }
                assert(lit_len < tr->scratch_buffer_size);
                lit[lit_len++] = peeked;
                b32 advanced = advance(tr, 1);
                assert(advanced);
            }
            assert(lit_len < tr->scratch_buffer_size);
            lit[lit_len] = 0;
                        
            for (u32 i = TOKEN_KEYWORD, local_i = 0; IS_TOKEN_KEYWORD(i) && local_i < ARRAY_SIZE(KEYWORD_STRS); ++i, ++local_i) {
                if (str_eq(KEYWORD_STRS[local_i], lit)) {
                    token->kind = i;
                    break;
                }
            }
            
            if (!token->kind) {
                StringID id = string_storage_add(tr->ss, lit);
                token->kind = TOKEN_IDENT;
                token->value_str = id;
            }
            break;
        } else if (symb == '\"') {
            advance(tr, 1);
            char *lit = (char *)tr->scratch_buffer;
            u32 lit_len = 0;
            for (;;) {
                u8 peeked = 0;
                if (!in_stream_peek(tr->st, &peeked, 1)) {
                    break;
                }
                if (peeked == '\"' || !peeked) {
                    break;
                }
                assert(lit_len < sizeof(lit));
                lit[lit_len++] = peeked;
                b32 advanced = advance(tr, 1);
                assert(advanced);
            }
            assert(lit_len < tr->scratch_buffer_size);
            lit[lit_len] = 0;
            
            StringID id = string_storage_add(tr->ss, lit);
            token->kind = TOKEN_STR;
            token->value_str = id;
            break;
        } else if (is_punct(symb)) {
            // Because muttiple operators can be put together (+=-2),
            // check by descending length
            for (u32 i = TOKEN_ILSHIFT, local_i = 0; IS_TOKEN_MULTISYMB(i); ++i, ++local_i) {
                if (parse(tr, MULTISYMB_STRS[local_i])) {
                    token->kind = i;
                    break;
                }
            }
            
            // All unhandled cases before - single character 
            if (!token->kind) {
                token->kind = symb;
                advance(tr, 1);
            }
            break;
        } else {
            report_error_tok(tr->er, token, "Unexpected token");
            token->kind = TOKEN_ERROR;
            advance(tr, 1);
            break;
        }
    }
    return token;
}

void eat_tok(Tokenizer *tr) {
    if (tr->active_token) {
        tr->active_token = 0;
    }
}

Token *peek_next_tok(Tokenizer *tr) {
    eat_tok(tr);
    return peek_tok(tr);    
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

uptr fmt_tok(char *buf, uptr buf_sz, StringStorage *ss, Token *token) {
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
                result = fmt(buf, buf_sz, "<ident>%s", string_storage_get(ss, token->value_str));
            } break;
            case TOKEN_STR: {
                result = fmt(buf, buf_sz, "<str>%s", string_storage_get(ss, token->value_str));
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
