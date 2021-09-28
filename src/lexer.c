#include "lexer.h"

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

Lexer *
create_tokenizer(ErrorReporter *er, StringStorage *ss, InStream *st, FileID file) {
    Lexer *lexer = arena_bootstrap(Lexer, arena);
    lexer->st = st;
    lexer->curr_loc.symb = 1;
    lexer->curr_loc.line = 1;
    lexer->curr_loc.file = file;
    lexer->scratch_buffer_size = TOKENIZER_DEFAULT_SCRATCH_BUFFER_SIZE;
    lexer->scratch_buffer = arena_alloc(&lexer->arena, lexer->scratch_buffer_size);
    lexer->ss = ss;
    lexer->er = er;
    // create hashes
    lexer->keyword_count = ARRAY_SIZE(KEYWORD_STRS);
    for (u32 i = 0; i < lexer->keyword_count; ++i) {
        lexer->keyword_hashes[i] = hash_string(KEYWORD_STRS[i]);
    }
    return lexer;
}

void 
destroy_tokenizer(Lexer *lexer) {
    arena_clear(&lexer->arena);
}

// @TODO(hl): @SPEED:
// Do we always have to check for newlines?
static b32
advance(Lexer *lexer, u32 n) {
    u32 line_idx = n;
    for (u32 i = 0; i < n; ++i) {
        if (in_stream_soft_peek_at(lexer->st, i) == '\n') {
            ++lexer->curr_loc.line;
            line_idx = 0;
        } 
        ++line_idx;
    }
    uptr advanced = in_stream_advance(lexer->st, n);
    lexer->curr_loc.symb = line_idx;
    return advanced == n;
}

static b32 
parse(Lexer *lexer, const char *str) {
    b32 result = FALSE;
    uptr len = str_len(str);
    assert(len < lexer->scratch_buffer_size);
    // @SPEED(hl): This can be done as direct compare with memory from 
    // stream, like in_stream_cmp(st, bf, bf_sz)
    if (in_stream_peek(lexer->st, lexer->scratch_buffer, len) == len) {
        result = mem_eq(lexer->scratch_buffer, str, len);
        if (result) {
            advance(lexer, len);
        }
    }
    
    return result;
}

static void 
begin_scratch_buffer_write(Lexer *lexer) {
    lexer->scratch_buffer_used = 0;
}

static b32 
scratch_buffer_write_and_advance(Lexer *lexer, u8 symb) {
    b32 result = FALSE;
    // @NOTE(hl): Reserve ony byte for null-termination
    if (lexer->scratch_buffer_used + 1 < lexer->scratch_buffer_size - 1) {
        lexer->scratch_buffer[lexer->scratch_buffer_used++] = symb;
        advance(lexer, 1);
        result = TRUE;
    } 
    return result;
}

static const char *
end_scratch_buffer_write(Lexer *lexer) {
    if (lexer->scratch_buffer_used + 1 < lexer->scratch_buffer_size) {
        lexer->scratch_buffer[lexer->scratch_buffer_used++] = 0;
    }
    return (char *)lexer->scratch_buffer;
}

Token *
peek_tok(Lexer *lexer) {
    Token *token = lexer->active_token;
    if (token) {
        return token;
    }
    
    token = arena_alloc_struct(&lexer->arena, Token);
    lexer->active_token = token;

    for (;;) {
        u8 symb = in_stream_peek_b_or_zero(lexer->st);
        if (!symb) {
            token->kind = TOKEN_EOS;
            break;
        }
        
        if (is_space(symb)) {
            advance(lexer, 1);
            continue;
        } else if (parse(lexer, "//")) {
            for (;;) {
                u8 peeked = in_stream_peek_b_or_zero(lexer->st);
                if (!peeked || peeked == '\n') {
                    break;
                }
                advance(lexer, 1);
            }
            continue;
        } else if (parse(lexer, "/*")) {
            SrcLoc comment_start_loc = lexer->curr_loc;
            u32 depth = 1;
            while (depth) {
                if (parse(lexer, "/*")) {
                    ++depth; 
                } else if (parse(lexer, "*/")) {
                    --depth;
                } else if (!advance(lexer, 1)) {
                    break;
                }
            }
            
            if (depth) {
                report_error(lexer->er, comment_start_loc, "*/ expected to end comment");
            }
            continue;
        }
        
        token->src_loc = lexer->curr_loc;
        if (is_digit(symb)) {
            begin_scratch_buffer_write(lexer);
            b32 is_real_lit = FALSE;
            for (;;) {
                u8 symb = in_stream_peek_b_or_zero(lexer->st);
                if (is_int(symb)) {
                    // nop
                } else if (is_real(symb)) {
                    is_real_lit = TRUE;
                } else {
                    break;
                }
                
                if (!scratch_buffer_write_and_advance(lexer, symb)) {
                    report_error_tok(lexer->er, token, "Too long number literal. Maximum length is %u", lexer->scratch_buffer_size - 1);
                    break;
                }
            }
            const char *lit = end_scratch_buffer_write(lexer);
            
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
            begin_scratch_buffer_write(lexer);
            scratch_buffer_write_and_advance(lexer, symb);
            for (;;) {
                u8 symb = in_stream_peek_b_or_zero(lexer->st);
                if (!is_ident(symb)) {
                    break;
                }
                
                if (!scratch_buffer_write_and_advance(lexer, symb)) {
                    report_error_tok(lexer->er, token, "Too long ident name. Maximum length is %u", lexer->scratch_buffer_size - 1);
                    break;
                }
            }
            const char *lit = end_scratch_buffer_write(lexer);
            u64 lit_hash = hash_string(lit);  // @SPEED(hl): This can be futher optimized, if we bake in 
            // hashing into parsing
            for (u32 kind = TOKEN_KEYWORD, i = 0; i < lexer->keyword_count; ++i, ++kind) {
                if (lit_hash == lexer->keyword_hashes[i]) {
                    token->kind = kind;
                    break;
                }
            }
            
            if (!token->kind) {
                StringID id = string_storage_add(lexer->ss, lit);
                token->kind = TOKEN_IDENT;
                token->value_str = id;
            }
            break;
        } else if (symb == '\"') {
            advance(lexer, 1);
            begin_scratch_buffer_write(lexer);
            for (;;) {
                u8 symb = in_stream_peek_b_or_zero(lexer->st);
                if (symb == '\"') {
                    break;
                }
                
                if (!scratch_buffer_write_and_advance(lexer, symb)) {
                    report_error_tok(lexer->er, token, "Too long string name. Maximum length is %u", lexer->scratch_buffer_size - 1);
                    break;
                }
            }
            const char *lit = end_scratch_buffer_write(lexer);
            
            StringID id = string_storage_add(lexer->ss, lit);
            token->kind = TOKEN_STR;
            token->value_str = id;
            break;
        } else if (is_punct(symb)) {
            // Because muttiple operators can be put together (+=-2),
            // check by descending length
            for (u32 kind = TOKEN_MULTISYMB, i = 0; i < ARRAY_SIZE(MULTISYMB_STRS); ++i, ++kind) {
                if (parse(lexer, MULTISYMB_STRS[i])) {
                    token->kind = kind;
                    break;
                }
            }
            
            // All unhandled cases before - single character 
            if (!token->kind) {
                token->kind = symb;
                advance(lexer, 1);
            }
            break;
        } else {
            report_error_tok(lexer->er, token, "Unexpected character");
            token->kind = TOKEN_ERROR;
            advance(lexer, 1);
            break;
        }
    }
    return token;
}

void 
eat_tok(Lexer *lexer) {
    if (lexer->active_token) {
        lexer->active_token = 0;
    }
}

Token *
peek_next_tok(Lexer *lexer) {
    eat_tok(lexer);
    return peek_tok(lexer);    
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
