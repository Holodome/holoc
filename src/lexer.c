#include "lexer.h"

#include "hashing.h"
#include "my_assert.h"
#include "string_storage.h"

#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <stdio.h>

typedef enum { 
    CHAR_LIT_REG  = 0x1, // '
    CHAR_LIT_UTF8 = 0x2, // u8'
    CHAR_LIT_U16  = 0x3, // u'
    CHAR_LIT_U32  = 0x4, // U'
    CHAR_LIT_WIDE = 0x5  // L'
} char_lit_kind;

typedef enum {
    STRING_LIT_REG  = 0x1, // "
    STRING_LIT_UTF8 = 0x2, // u8"
    STRING_LIT_U16  = 0x3, // u"
    STRING_LIT_U32  = 0x4, // U"
    STRING_LIT_WIDE = 0x5, // L"
} string_lit_kind;

typedef enum {
    PP_TOKEN_EOF  = 0x1,
    PP_TOKEN_INT  = 0x2,
    PP_TOKEN_REAL = 0x3,
    PP_TOKEN_CHAR = 0x4,
    PP_TOKEN_STR  = 0x5
} pp_token_kind;

typedef struct {
    pp_token_kind kind;
    union {
        char_lit_kind   char_kind;
        string_lit_kind string_kind;
    };
} pp_token;

static c_pp_macro *
get_macro_internal(c_preprocessor *pp, str_hash name_hash, bool or_zero) {
    c_pp_macro *macro = hash_table_oa_get_u32(pp->macro_hash, pp->hash_size,
        sizeof(c_pp_macro), offsetof(c_pp_macro, hash),
        name_hash.value, or_zero);
    return macro;
}

static pp_token
generate_pp_token(c_lexer *lex) {
    pp_token token = {0};
    
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

c_pp_macro *
c_pp_define(c_preprocessor *pp, string name) {
    str_hash name_hash = hash_string(name);
    c_pp_macro *macro = get_macro_internal(pp, name_hash, true);
    if (macro) {
        memset(macro, 0, sizeof(*macro));
        macro->name = name;
        macro->hash = name_hash;
    } else {
        NOT_IMPLEMENTED;
    }
    return macro;
}

void        
c_pp_undef(c_preprocessor *pp, string name) {
    str_hash name_hash = hash_string(name);
    c_pp_macro *macro = get_macro_internal(pp, name_hash, false);
    if (macro) {
        memset(&macro->hash, 0, sizeof(macro->hash));
    } else {
        NOT_IMPLEMENTED;
    }
}

c_pp_macro *
c_pp_get(c_preprocessor *pp, string name) {
    str_hash name_hash = hash_string(name);
    c_pp_macro *macro = get_macro_internal(pp, name_hash, false);
    return macro;
}

void 
init_c_lexbuf(c_lexbuf *buf, void *buffer_init, uintptr_t buffer_size, uint8_t kind) {
    char *buffer = (uint8_t *)buffer_init;
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
init_c_lexer(struct file_registry *fr, string filename) {
    c_lexer *lex = calloc(1, sizeof(c_lexer));
    init_c_lexbuf_storage(&lex->buffers);
    c_pp_init(&lex->pp, 8192);
    
    return lex;
}

c_token *
c_lexer_peek_forward(c_lexer *lex, uint32_t forward) {
    
}

c_token *
c_lexer_peek(c_lexer *lex) {
    
}

void 
c_lexer_eat(c_lexer *lex) {
    
}