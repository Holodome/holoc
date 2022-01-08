#include "preprocessor.h"

#include <stddef.h>
#include <string.h>
#include <ctype.h>

#include "my_assert.h"
#include "str.h"
#include "hashing.h"
#include "bump_allocator.h"
#include "string_storage.h"

typedef struct {
    bool is_parsed;
    bool is_newline_skipped;
} parse_whitespace_result;

#define get_macro_internal(_pp, _name_hash, _or_zero) \
    hash_table_oa_get_u32((_pp)->macro_hash, (_pp)->macro_hash_size, \
            sizeof(c_pp_macro), offsetof(c_pp_macro, hash), \
            (_name_hash).value, _or_zero)

c_preprocessor *
c_pp_init(uint32_t hash_size) { 
    c_preprocessor *pp = bump_bootstrap(c_preprocessor, allocator);
    pp->ss = string_storage_init(pp->allocator);
}

void 
c_pp_push_if(c_preprocessor *pp, bool is_handled) {

}

void 
c_pp_pop_if(c_preprocessor *pp) {

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


void init_pp_lexbuf(pp_lexbuf *buf, void *buffer, uintptr_t buffer_size, uint8_t kind);
uint8_t pp_lexbuf_peek(pp_lexbuf *buffer);
uint32_t pp_lexbuf_advance(pp_lexbuf *buffer, uint32_t n);
bool pp_lexbuf_next_eq(pp_lexbuf *buffer, string lit);
bool pp_lexbuf_parse(pp_lexbuf *buffer, string lit);


void init_pp_lexbuf_storage(pp_lexbuf_storage *bs);
pp_lexbuf *pp_lexbuf_storage_add(pp_lexbuf_storage *bs);
void pp_lexbuf_storage_remove(pp_lexbuf_storage *bs);

// Returns next character while advancing buffers
// Returns 0 if all buffers are empty
static uint8_t 
peek_cp(c_preprocessor *pp) {
    uint8_t symb = 0;
    c_lexbuf *buf = lex->buffers.current_buffer;
    while (buf && !symb) {
        symb = c_lexbuf_peek(buf);
        if (!symb) {
        }
    }

    return symb;
}

static void 
eat_cp(c_preprocessor *pp) {

}

static uint8_t
peek_next_cp(c_preprocessor *pp) {

}

// Does cascade parsing of whitespace symbols
// Skips whitespaces and comments
static parse_whitespace_result
parse_whitespace(c_preprocessor *pp) {
    bool is_parsed = false;
    uint8_t cp = peek_cp(pp);
    for (;;) {
        if (isspace(cp)) {
            is_parsed = true;
            cp = peek_next_cp(pp);
        } else if (parse(pp, WRAP_Z("//"))) {
            is_parsed = true;
            cp = peek_next_cp(pp);
            while (cp != '\n' && cp) {
                cp = peek_next_cp(pp);
            }
        } else if (parse(pp, WRAP_Z("/*"))) {
            is_parsed = true;
            cp = peek_cp(pp);
            for (;;) {
                if (!cp) {
                    pp->error_message = "Unterminated multiline comment";
                    break;
                } else if (cp == '*' && parse(pp, WRAP_Z("*/"))) {
                    break;
                } else {
                    cp = peek_next_cp(pp);
                }
            }
        } else {
            break;
        }
    }
    return is_parsed;
}

static bool 
parse_string_literal(c_preprocessor *pp) {
    uint32_t string_lit_kind = 0;
    if (parse(pp, WRAP_Z("u8'"))) {
        string_lit_kind = C_PP_CHAR_LIT_UTF8;
    } else if (parse(pp, WRAP_Z("u'"))) {
        string_lit_kind = C_PP_CHAR_LIT_UTF16;
    } else if (parse(pp, WRAP_Z("U'"))) {\
        string_lit_kind = C_PP_CHAR_LIT_UTF32;
    } else if (parse(pp, WRAP_Z("L'"))) {
        string_lit_kind = C_PP_CHAR_LIT_WIDE;
    } else if (parse(pp, WRAP_Z("'"))) {
        string_lit_kind = C_PP_CHAR_LIT_REG;
    } else if (parse(pp, WRAP_Z("u8\""))) {
        string_lit_kind = C_PP_STRING_LIT_UTF8;
    } else if (parse(pp, WRAP_Z("u\""))) {
        string_lit_kind = C_PP_STRING_LIT_UTF16;
    } else if (parse(pp, WRAP_Z("U\""))) {\
        string_lit_kind = C_PP_STRING_LIT_UTF32;
    } else if (parse(pp, WRAP_Z("L\""))) {
        string_lit_kind = C_PP_STRING_LIT_WIDE;
    } else if (parse(pp, WRAP_Z("\""))) {
        string_lit_kind = C_PP_STRING_LIT_REG;
    }
        
    if (string_lit_kind) {
        uint8_t quote = 0;
        if (string_lit_kind & C_PP_STRING_LIT_CHAR_MASK) {
            quote = '\'';
        } else if (string_lit_kind & C_PP_STRING_LIT_STRING_MASK) {
            quote = '\"';
        }
        assert(quote);
   
        uint8_t cp = peek_cp(pp);
        for (;;) {
            if (!cp) {
                pp->error_message = "Unterminated string literal";
                break;
            } else if (cp == '\\') {
                if (parse(lexer, WRAP_Z("\\\n"))) {
                
                }
            } else if (cp == '\n') {
                pp->error_message = "Unexpected EOL in string literal";
                break;
            }
        }
    }
    
    return string_lit_kind != 0; 
}

bool 
c_pp_parse(c_preprocessor *pp) {
    // Signal that we have parsed the token corectly. 
    // EOF is considered false
    for (;;) {
        // Special handling of individual condepoints
        {
            uint8_t cp = peek_cp(pp);
            if (!cp) { // All buffers are empty 
                pp->token_kind = C_PP_TOKEN_EOF;
                break;
            } else if (cp & 0x80) { // A non-ascii character
                pp->token_kind = C_PP_TOKEN_ERROR;
                pp->error_message = "Unexpected non-ASCII character";
                eat_cp(pp);
                break;
            } 
        } 

        // Parse whitespace and comment, which is also coutned as
        // whitespace
        if (parse_whitespace(pp)) {
            continue;
        }

        // Check if this is a preprocessor directive start
        // If inside conditional code compilation, skip until mathching #if
        if (handle_preprocessor_directives(pp)) {
            continue;
        }

        // String literal is either string constant or character constant
        if (parse_string_literal(pp)) {
            break;
        }

        if (parse_number(pp)) {
            break;
        }

        if (parse_ident(pp)) {
            break;
        }

        if (parse_punct(pp)) {
            break;
        }
        is_generated = true;
    }

    return pp->token_kind != C_PP_TOKEN_EOF;
}
