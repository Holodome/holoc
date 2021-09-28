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

b32 
is_token_assign(u32 tok) {
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
    // create hashes
    tr->keyword_count = ARRAY_SIZE(KEYWORD_STRS);
    for (u32 i = 0; i < tr->keyword_count; ++i) {
        tr->keyword_hashes[i] = hash_string(KEYWORD_STRS[i]);
    }
    return tr;
}

void 
destroy_tokenizer(Tokenizer *tr) {
    arena_clear(&tr->arena);
}

// @TODO(hl): @SPEED:
// Do we always have to check for newlines?
static b32
advance(Tokenizer *tr, u32 n) {
    u32 line_idx = n;
    for (u32 i = 0; i < n; ++i) {
        if (in_stream_soft_peek_at(tr->st, i) == '\n') {
            ++tr->curr_loc.line;
            line_idx = 0;
        } 
        ++line_idx;
    }
    uptr advanced = in_stream_advance(tr->st, n);
    tr->curr_loc.symb = line_idx;
    return advanced == n;
}

static b32 
parse(Tokenizer *tr, const char *str) {
    b32 result = FALSE;
    uptr len = str_len(str);
    assert(len < tr->scratch_buffer_size);
    // @SPEED(hl): This can be done as direct compare with memory from 
    // stream, like in_stream_cmp(st, bf, bf_sz)
    if (in_stream_peek(tr->st, tr->scratch_buffer, len) == len) {
        result = mem_eq(tr->scratch_buffer, str, len);
        if (result) {
            advance(tr, len);
        }
    }
    
    return result;
}

static void 
begin_scratch_buffer_write(Tokenizer *tokenizer) {
    tokenizer->scratch_buffer_used = 0;
}

static b32 
scratch_buffer_write_and_advance(Tokenizer *tokenizer, u8 symb) {
    b32 result = FALSE;
    // @NOTE(hl): Reserve ony byte for null-termination
    if (tokenizer->scratch_buffer_used + 1 < tokenizer->scratch_buffer_size - 1) {
        tokenizer->scratch_buffer[tokenizer->scratch_buffer_used++] = symb;
        advance(tokenizer, 1);
        result = TRUE;
    } 
    return result;
}

static const char *
end_scratch_buffer_write(Tokenizer *tokenizer) {
    if (tokenizer->scratch_buffer_used + 1 < tokenizer->scratch_buffer_size) {
        tokenizer->scratch_buffer[tokenizer->scratch_buffer_used++] = 0;
    }
    return (char *)tokenizer->scratch_buffer;
}

Token *
peek_tok(Tokenizer *tr) {
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
        
        if (is_space(symb)) {
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
            SrcLoc comment_start_loc = tr->curr_loc;
            u32 depth = 1;
            while (depth) {
                if (parse(tr, "/*")) {
                    ++depth; 
                } else if (parse(tr, "*/")) {
                    --depth;
                } else if (!advance(tr, 1)) {
                    break;
                }
            }
            
            if (depth) {
                report_error(tr->er, comment_start_loc, "*/ expected to end comment");
            }
            continue;
        }
        
        token->src_loc = tr->curr_loc;
        if (is_digit(symb)) {
            begin_scratch_buffer_write(tr);
            b32 is_real_lit = FALSE;
            for (;;) {
                u8 symb = in_stream_peek_b_or_zero(tr->st);
                if (is_int(symb)) {
                    // nop
                } else if (is_real(symb)) {
                    is_real_lit = TRUE;
                } else {
                    break;
                }
                
                if (!scratch_buffer_write_and_advance(tr, symb)) {
                    report_error_tok(tr->er, token, "Too long number literal. Maximum length is %u", tr->scratch_buffer_size - 1);
                    break;
                }
            }
            const char *lit = end_scratch_buffer_write(tr);
            
            if (is_real_lit) {
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
            begin_scratch_buffer_write(tr);
            scratch_buffer_write_and_advance(tr, symb);
            for (;;) {
                u8 symb = in_stream_peek_b_or_zero(tr->st);
                if (!is_ident(symb)) {
                    break;
                }
                
                if (!scratch_buffer_write_and_advance(tr, symb)) {
                    report_error_tok(tr->er, token, "Too long ident name. Maximum length is %u", tr->scratch_buffer_size - 1);
                    break;
                }
            }
            const char *lit = end_scratch_buffer_write(tr);
            u64 lit_hash = hash_string(lit);  // @SPEED(hl): This can be futher optimized, if we bake in 
            // hashing into parsing
            for (u32 kind = TOKEN_KEYWORD, i = 0; i < tr->keyword_count; ++i, ++kind) {
                if (lit_hash == tr->keyword_hashes[i]) {
                    token->kind = kind;
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
            begin_scratch_buffer_write(tr);
            for (;;) {
                u8 symb = in_stream_peek_b_or_zero(tr->st);
                if (symb == '\"') {
                    break;
                }
                
                if (!scratch_buffer_write_and_advance(tr, symb)) {
                    report_error_tok(tr->er, token, "Too long string name. Maximum length is %u", tr->scratch_buffer_size - 1);
                    break;
                }
            }
            const char *lit = end_scratch_buffer_write(tr);
            
            StringID id = string_storage_add(tr->ss, lit);
            token->kind = TOKEN_STR;
            token->value_str = id;
            break;
        } else if (is_punct(symb)) {
            // Because muttiple operators can be put together (+=-2),
            // check by descending length
            for (u32 kind = TOKEN_MULTISYMB, i = 0; i < ARRAY_SIZE(MULTISYMB_STRS); ++i, ++kind) {
                if (parse(tr, MULTISYMB_STRS[i])) {
                    token->kind = kind;
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
            report_error_tok(tr->er, token, "Unexpected character");
            token->kind = TOKEN_ERROR;
            advance(tr, 1);
            break;
        }
    }
    return token;
}

void 
eat_tok(Tokenizer *tr) {
    if (tr->active_token) {
        tr->active_token = 0;
    }
}

Token *
peek_next_tok(Tokenizer *tr) {
    eat_tok(tr);
    return peek_tok(tr);    
}

uptr 
fmt_tok_kind(char *buf, uptr buf_sz, u32 kind) {
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

uptr 
fmt_tok(char *buf, uptr buf_sz, StringStorage *ss, Token *token) {
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
