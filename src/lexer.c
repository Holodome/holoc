#include "lexer.h"

#include "hashing.h"
#include "my_assert.h"
#include "string_storage.h"
#include "hashing.h"

#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <stdio.h>

static c_pp_macro *
get_macro_internal(c_preprocessor *pp, str_hash name_hash, bool or_zero) {
    c_pp_macro *macro = hash_table_oa_get_u32(pp->macro_hash, pp->hash_size,
        sizeof(c_pp_macro), offsetof(c_pp_macro, hash),
        name_hash.value, or_zero);
    return macro;
}

static bool 
generate_character_literal(c_lexer *lexer, c_lexbuf *buffer, c_pp_token *token) {
    
}

static bool 
generate_string_literal(c_lexer *lexer, c_lexbuf *buffer, c_pp_token *token) {
    
}

static bool 
generate_number_literal(c_lexer *lexer, c_lexbuf *buffer, c_pp_token *token) {
    
}

static bool 
generate_identifier_literal(c_lexer *lexer, c_lexbuf *buffer, pp_token *token) {
    
}

static bool 
generate_punctuator_literal(c_lexer *lexer, c_lexbuf *buffer, pp_token *token) {
    
}

static pp_token
generate_pp_token(c_lexer *lex) {
    pp_token token = {0};
   
    for (;;) {
        uint8_t symb = peek_next_symb(lex);
        if (!symb) {
            token.kind = PP_TOKEN_EOF;
            break;
        }

        while (isspace(symb)) {
        
        }
        c_lexbuf *buffer = lex->buffer->current_buffer;
    }

    return token;
}

void 
c_pp_init(c_preprocessor *pp, uint32_t hash_size) {
    pp->hash_size = hash_size;
    pp->macro_hash = calloc(hash_size, sizeof(*pp->macro_hash));
    pp->ss = init_string_storage(8096);
}

void 
c_pp_push_if(c_preprocessor *pp, bool is_handled) {
    assert(!pp->is_current_if_handled);
    ++pp->if_depth;
    pp->is_current_if_handled = is_handled;
}

void 
c_pp_pop_if(c_preprocessor *pp) {
    assert(pp->if_depth);
    pp->is_current_if_handled = false;
}
void 
init_c_lexbuf(c_lexbuf *buf, void *buffer_init, uintptr_t buffer_size, uint8_t kind) {
    uint8_t *buffer = (uint8_t *)buffer_init;
    buf->buf = buffer;
    buf->at = buf->buf;
    buf->eof = buf->at + buffer_size;
    buf->kind = kind;
}

uint8_t 
c_lexbuf_peek(c_lexbuf *buffer) {
#if 0
    uint8_t result = 0;
    if (buffer->at < buffer->eof) {
        result = *buffer->at;
    }
    return result;
#else 
    assert(buffer->at <= buffer->eof);
    uint8_t result = *buffer->at;
    return result;
#endif 
}

uint32_t 
c_lexbuf_advance(c_lexbuf *buffer, uint32_t n) {
    assert(buffer->at <= buffer->eof);
    uintptr_t space_remaining = buffer->eof - buffer->at;
    if (n > space_remaining) {
        n = space_remaining;
    }
    
    uint8_t *new_at = buffer->at + n;
    if (new_at < buffer->eof) {
        buffer-> at = new_at;
    }
    return n;
}

bool 
c_lexbuf_next_eq(c_lexbuf *buffer, string lit) {
    bool result = false;
    uint32_t n = lit.len;
    assert(buffer->at <= buffer->eof);
    uintptr_t space_remaining = buffer->eof - buffer->at;
    if (n <= space_remaining) {
        result = memcmp(buffer->at, lit.data, lit.len) == 0;
    }
    return result;
}

bool 
c_lexbuf_parse(c_lexbuf *buffer, string lit) {
    bool result = c_lexbuf_next_eq(buffer, lit);
    if (result) {
        uint32_t advanced = c_lexbuf_advance(buffer, lit.len);
        assert(advanced == lit.len);
    }
    return result;
}

void 
init_c_lexbuf_storage(c_lexbuf_storage *bs) {
    
}

c_lexer *
c_lexer_init(struct file_registry *fr, string_storage *ss, string filename) {
    c_lexer *lex = calloc(1, sizeof(c_lexer));
    init_c_lexbuf_storage(&lex->buffers);
    c_pp_init(&lex->pp, 8192);
    
    return lex;
}

c_token *
c_lexer_peek_forward(c_lexer *lex, uint32_t forward) {
    return 0;
}

c_token *
c_lexer_peek(c_lexer *lex) {
    return 0;
}

void 
c_lexer_eat(c_lexer *lex) {
    
}
