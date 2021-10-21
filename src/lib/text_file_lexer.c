#include "text_file_lexer.h"

#include "lib/strings.h"

Text_Lexer 
text_lexer(void *buf, uintptr_t buf_size, 
           void *string_buf, u32 string_buf_size) {
    Text_Lexer lexer = {0};
    lexer.buf = (u8 *)buf;
    lexer.buf_at = lexer.buf;                          
    lexer.buf_eof = lexer.buf + buf_size;
    lexer.line = 1;
    lexer.symb = 1;
    lexer.string_buf = string_buf;
    lexer.string_buf_size = string_buf_size;
    return lexer;
}
                      
static u32 
peek_forward(Text_Lexer *lexer, u32 forward) {
    u32 codepoint = 0;
    const void *local_at = lexer->buf_at;
    while (forward-- && lexer->buf_at < lexer->buf_eof) {
        local_at = utf8_decode(local_at, &codepoint);   
    }
    if (lexer->buf_at >= lexer->buf_eof) {
        codepoint = 0;
    }
    return codepoint;
}
                      
static u32 
peek_codepoint(Text_Lexer *lexer) {
    return peek_forward(lexer, 1);
}

static void 
advance(Text_Lexer *lexer) {
    if (lexer->buf_at < lexer->buf_eof) {
        u32 codepoint = 0;
        lexer->buf_at = (u8 *)utf8_decode(lexer->buf_at, &codepoint);
        if (codepoint == '\n') {
            lexer->line += 1;
            lexer->symb = 1;
        } else if (is_printable(codepoint)) {
            ++lexer->symb;
        }
    }
}
                      
bool 
text_lexer_token(Text_Lexer *lexer) {
    bool result = false;
    
    u32 codepoint;
start:
    codepoint = peek_codepoint(lexer);
    if (!codepoint) {
        lexer->token_kind = TEXT_FILE_TOKEN_EOF;
        goto end;    
    }
    
    bool is_space_advanced = false;
    while (is_space(codepoint)) {
        advance(lexer);
        codepoint = peek_codepoint(lexer);
    }
    if (is_space_advanced) {
        goto start;
    }
    
    if (is_ident_start(codepoint)) {
        u32 length = 0;
        while (length + 1 < lexer->string_buf_size) {
            codepoint = peek_codepoint(lexer);
            if (is_ident(codepoint)) {
                lexer->string_buf[length++] = (char)codepoint;
            } else {
                break;
            }
            advance(lexer);
        }
        assert(length < lexer->string_buf_size);
        lexer->string_buf[length] = 0;
        
        lexer->token_kind = TEXT_FILE_TOKEN_IDENT;
        goto end;    
    } 
    
    if (is_digit(codepoint) || 
        ((codepoint == '+' || codepoint == '-') && is_digit(peek_forward(lexer, 2)) )) {
        u32 length = 0;
        bool is_real_lit = false;
        while (length + 1 < lexer->string_buf_size) {
            codepoint = peek_codepoint(lexer);
            if (is_real(codepoint)) {
                is_real_lit = true;
                lexer->string_buf[length++] = (u8)codepoint;
            } else if (is_digit(codepoint)) {
                lexer->string_buf[length++] = (u8)codepoint;
            } else {
                break;
            }
            advance(lexer);
        }
        assert(length < lexer->string_buf_size);
        lexer->string_buf[length] = 0;
        
        if (is_real_lit) {
           lexer->token_kind = TEXT_FILE_TOKEN_REAL;
           lexer->real_value = z2f64((const char *)lexer->string_buf); 
        } else {
            lexer->token_kind = TEXT_FILE_TOKEN_INT;
            lexer->int_valie = z2i64((const char *)lexer->string_buf);
        }
        goto end;       
    }
    
    if (codepoint == '\"') {
        advance(lexer);
        u32 length = 0;
        // @NOTE(hl): Reserve 4 here in case of wrong utf8 decoding
        while (length + 4 < lexer->string_buf_size) {
            codepoint = peek_codepoint(lexer);
            if (codepoint == '\"') {
                break;
            } else {
                // @TODO(hl): This is kinda stupid
                length += utf8_encode(codepoint, lexer->string_buf + length);
            }
        }
        
        assert(length < lexer->string_buf_size);
        lexer->string_buf[length] = 0;
        lexer->token_kind = TEXT_FILE_TOKEN_STR;
        goto end;
    }
    
    if (is_punct(codepoint)) {
        advance(lexer);
        lexer->token_kind = codepoint;
        goto end;
    }
end:
    return result;
}
