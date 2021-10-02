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
    "/=",
    "*=",
    "%=",
    "&&",
    "||",
    "::"
};  

bool 
is_token_assign(u32 tok) {
    return tok == '=' || tok == TOKEN_IADD || tok == TOKEN_ISUB || tok == TOKEN_IMUL ||
        tok == TOKEN_IDIV || tok == TOKEN_IMOD || tok == TOKEN_IAND || tok == TOKEN_IOR ||
        tok == TOKEN_IXOR || tok == TOKEN_ILSHIFT || tok == TOKEN_IRSHIFT; 
}

Lexer *
create_lexer(CompilerCtx *ctx, InStream *stream, FileID file) {
    Lexer *lexer = arena_bootstrap(Lexer, arena);
    lexer->stream = stream;
    lexer->curr_loc.symb = 1;
    lexer->curr_loc.line = 1;
    lexer->curr_loc.file = file;
    lexer->scratch_buffer_size = TOKENIZER_DEFAULT_SCRATCH_BUFFER_SIZE;
    lexer->scratch_buffer = arena_alloc(&lexer->arena, lexer->scratch_buffer_size);
    lexer->ctx = ctx;
    for (u32 i = 0; i < ARRAY_SIZE(KEYWORD_STRS); ++i) {
        lexer->keyword_ids[i] = string_storage_add(lexer->ctx->ss, KEYWORD_STRS[i]);
    }
    return lexer;
}

void 
destroy_lexer(Lexer *lexer) {
    arena_clear(&lexer->arena);
}

// @TODO(hl): @SPEED:
// Do we always have to check for newlines?
static bool
advance(Lexer *lexer, u32 n) {
    u32 line_idx = lexer->curr_loc.symb;
    for (u32 i = 0; i < n; ++i) {
        if (in_stream_soft_peek_at(lexer->stream, i) == '\n') {
            ++lexer->curr_loc.line;
            line_idx = 0;
        } 
        ++line_idx;
    }
    uptr advanced = in_stream_advance(lexer->stream, n);
    lexer->curr_loc.symb = line_idx;
    return advanced == n;
}

static bool 
parse(Lexer *lexer, const char *str) {
    bool result = false;
    uptr len = str_len(str);
    assert(len < lexer->scratch_buffer_size);
    // @SPEED(hl): This can be done as direct compare with memory from 
    // stream, like in_stream_cmp(stream, bf, bf_sz)
    if (in_stream_peek(lexer->stream, lexer->scratch_buffer, len) == len) {
        result = mem_eq(lexer->scratch_buffer, str, len);
        if (result) {
            advance(lexer, len);
        }
    }
    
    return result;
}

static void 
begin_lit_write(Lexer *lexer) {
    lexer->scratch_buffer_used = 0;
    string_storage_begin_write(lexer->ctx->ss);
}

static void
lit_write(Lexer *lexer, u8 symb) {
    if (lexer->scratch_buffer_used + 1 > lexer->scratch_buffer_size) {
        string_storage_write(lexer->ctx->ss, lexer->scratch_buffer, lexer->scratch_buffer_size);
        lexer->scratch_buffer_used = 0;
    } 
    lexer->scratch_buffer[lexer->scratch_buffer_used++] = symb;
    advance(lexer, 1);
}

static StringID
end_lit_write(Lexer *lexer) {
    string_storage_write(lexer->ctx->ss, lexer->scratch_buffer, lexer->scratch_buffer_used);
    return string_storage_end_write(lexer->ctx->ss);
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
        u8 symb = in_stream_peek_b_or_zero(lexer->stream);
        if (!symb) {
            token->kind = TOKEN_EOS;
            break;
        }
        
        if (is_space(symb)) {
            advance(lexer, 1);
            continue;
        } else if (parse(lexer, "//")) {
            for (;;) {
                u8 peeked = in_stream_peek_b_or_zero(lexer->stream);
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
                report_error(lexer->ctx->er, comment_start_loc, "*/ expected to end comment");
            }
            continue;
        }
        
        token->src_loc = lexer->curr_loc;
        if (is_digit(symb)) {
            begin_lit_write(lexer);
            bool is_real_lit = false;
            for (;;) {
                symb = in_stream_peek_b_or_zero(lexer->stream);
                if (is_int(symb)) {
                    // nop
                } else if (is_real(symb)) {
                    is_real_lit = true;
                } else {
                    break;
                }
                
                lit_write(lexer, symb);
            }
            StringID str_id = end_lit_write(lexer);
            // @NOTE(hl): This is stupid, just because we decided that strings can have arbitrary size
            string_storage_get(lexer->ctx->ss, str_id, lexer->scratch_buffer, lexer->scratch_buffer_size);
            if (is_real_lit) {
                f64 real = str_to_f64((char *)lexer->scratch_buffer);
                token->value_real = real;
                token->kind = TOKEN_REAL;
            } else {
                i64 iv = str_to_i64((char *)lexer->scratch_buffer);
                token->value_int = iv;
                token->kind = TOKEN_INT;
            }
            break;
            
        } else if (is_ident_start(symb)) {
            begin_lit_write(lexer);
            lit_write(lexer, symb);
            for (;;) {
                symb = in_stream_peek_b_or_zero(lexer->stream);
                if (!is_ident(symb)) {
                    break;
                }
                
                lit_write(lexer, symb);
            }
            StringID str_id = end_lit_write(lexer);
            for (u32 i = 0; i < MAX_KEYWORD_TOKEN_COUNT; ++i) {
                if (str_id.value == lexer->keyword_ids[i].value) {
                    token->kind = TOKEN_KEYWORD + i;
                }
            }
            
            if (!token->kind) {
                token->kind = TOKEN_IDENT;
                token->value_str = str_id;
            }
            break;
        } else if (symb == '\"') {
            advance(lexer, 1);
            begin_lit_write(lexer);
            for (;;) {
                symb = in_stream_peek_b_or_zero(lexer->stream);
                if (symb == '\"') {
                    break;
                }
                
                lit_write(lexer, symb);
            }
            StringID str_id = end_lit_write(lexer);
            
            token->kind = TOKEN_STR;
            token->value_str = str_id;
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
            report_error_tok(lexer->ctx->er, token, "Unexpected character");
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
fmt_tok(CompilerCtx *ctx, char *buf, uptr buf_sz, Token *token) {
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
            result = fmt(buf, buf_sz, "<ident>");
            result += string_storage_get(ctx->ss, token->value_str, (u8 *)buf + result, buf_sz - result);
        } break;
        case TOKEN_STR: {
            result = fmt(buf, buf_sz, "<str>");
            result += string_storage_get(ctx->ss, token->value_str, (u8 *)buf + result, buf_sz - result);
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
