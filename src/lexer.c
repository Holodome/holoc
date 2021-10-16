#include "lexer.h"

#include "lib/memory.h"
#include "lib/lists.h"
#include "lib/strings.h"

#include "c_lang.h"
#include "ast.h"
#include "string_storage.h"
#include "compiler_ctx.h"
#include "file_registry.h"

#define LEX_STR_HASH_FUNC fnv64

#define ITER_KEYWORDS(_it) \
for ((_it) = KEYWORD_AUTO; (_it) < KEYWORD_SENTINEL; ++(_it))
#define ITER_PREPROCESSOR_KEYWORD(_it) \
for ((_it) = PP_KEYWORD_DEFINE; (_it) < PP_KEYWORD_SENTINEL; ++(_it))
#define ITER_PUNCTUATORS(_it) \
for ((_it) = PUNCTUATOR_IADD; (_it) < PUNCTUATOR_SENTINEL; ++(_it))

static const char *KEYWORD_STRINGS[] = {
    "(unknown)",
    "auto",
    "break",
    "case",
    "char",
    "const",
    "continue",
    "default",
    "do",
    "double",
    "else ",
    "enum ",
    "extern",
    "float",
    "for ",
    "goto",
    "if",
    "inline",
    "int",
    "long",
    "register",
    "restrict",
    "return",
    "short",
    "signed",
    "unsigned ",
    "static ",
    "struct",
    "switch ",
    "typedef",
    "union ",
    "unsigned",
    "void",
    "volatile",
    "while",
    "_Alignas",
    "_Alignof",
    "_Atomic",
    "_Bool",
    "_Complex",
    "_Decimal128 ",
    "_Decimal32",
    "_Decimal64",
    "_Generic",
    "_Imaginary",
    "_Noreturn",
    "_Static_assert",
    "_Thread_local",
    "_Pragma",
};

static const char *PREPROCESSOR_KEYWORD_STRINGS[] = {
    "(unknown)",
    "define",
    "undef",
    "include",
    "if",
    "ifdef",
    "ifndef",
    "else",
    "elifdef",
    "elifndef",
    "pragma",
    "error",
    "defined",
    "line",
    "elif",
    "endif"
};

static const char *PUNCTUATOR_STRINGS[] = {
    "(unknown)",
    ">>=",
    "<<=", 
    "...",
    "+=",
    "-=",
    "*=",
    "/=",
    "%=", 
    "&=",
    "|=",
    "^=",
    "++",
    "--",
    ">>",
    "<<",
    "&&",
    "||",
    "==",
    "!=", 
    "<=", 
    ">=", 
    "->",
};

static const char *ALPHABET       = "0123456789ABCDEF";
static const char *ALPHABET_LOWER = "0123456789abcdef";

u32 
fmt_token_kind(char *buf, u64 buf_sz, u32 kind) {
    u32 result = 0;
    switch (kind) {
    case TOKEN_EOS: {
        result = fmt(buf, buf_sz, "<eos>");
    } break;
    case TOKEN_IDENT: {
        result = fmt(buf, buf_sz, "<ident>");
    } break;
    case TOKEN_KEYWORD: {
        result = fmt(buf, buf_sz, "<kw>");
    } break;
    case TOKEN_STRING: {
        result = fmt(buf, buf_sz, "<str>");
    } break;
    case TOKEN_NUMBER: {
        result = fmt(buf, buf_sz, "<num>");
    } break;
    case TOKEN_PUNCTUATOR: {
        result = fmt(buf, buf_sz, "<punct>");
    } break;
    }    
    return result;
}

u32 
fmt_token(char *buf, u64 buf_sz, Token *token) {
    u32 result = 0;
    switch (token->kind) {
    case TOKEN_EOS: {
        result = fmt(buf, buf_sz, "<eos>");
    } break;
    case TOKEN_IDENT: {
        result = fmt(buf, buf_sz, "<ident>%s", token->ident.str);
    } break;
    case TOKEN_KEYWORD: {
        result = fmt(buf, buf_sz, "<kw>%s", KEYWORD_STRINGS[token->kw]);
    } break;
    case TOKEN_STRING: {
        result = fmt(buf, buf_sz, "<str>%s", token->str.str);
    } break;
    case TOKEN_NUMBER: {
        result = fmt(buf, buf_sz, "<num>%llu", 
            token->number.int_value);
    } break;
    case TOKEN_PUNCTUATOR: {
        result = fmt(buf, buf_sz, "<punct>");
        if (token->punct < 0x100) {
            result += fmt(buf + result, buf_sz - result, "%c", token->punct);
        } else {
            result += fmt(buf + result, buf_sz - result, "%s", 
                PUNCTUATOR_STRINGS[token->punct - 0x100]);
        }
    } break;
    }    
    return result;
}

u8 
lexbuf_peek(Lexer_Buffer *buffer) {
    u8 result = 0;
    if (buffer->at < buffer->buf + buffer->size) {
        result = *buffer->at;
    } 
    return result;
}

u32 
lexbuf_advance(Lexer_Buffer *buffer, u32 n) {
    u32 advanced = 0;
    if (buffer->at < buffer->buf + buffer->size && n--) {
        ++buffer->at;
        ++advanced;
    }
    return advanced;
}

bool 
lexbuf_parse(Lexer_Buffer *buffer, const char *lit) {
    u32 length = zlen(lit);
    bool result = false;
    if (buffer->at + length < buffer->buf + buffer->size) {
        result = mem_eq(buffer->at, lit, length);
        if (result) {
            lexbuf_advance(buffer, length);
        }
    }
    return result;
}

static bool
is_symb_base(u32 symb, u32 base) {
    bool result = false;
    if (base == 2) {
        result = (symb == '0' || symb == '1');
    } else if (base == 8) {
        result = ('0' <= symb && symb <= '7');
    } else if (base == 16) {
        result = ('0' <= symb && symb <= '9') || ('A' <= symb && symb <= 'F') || ('a' <= symb && symb <= 'f');
    } else if (base == 10) {
        result = ('0' <= symb && symb <= '9');
    } else {
        UNREACHABLE;
    }
    return result;
}

static u64 
symb_to_base(u32 symb, u32 base) {
    // 0110000 - 0111001 from 0 - 9
    // 0001111 mask 
    
    // 1100001 - 1111010 from  a - z
    // 1000001 - 1011010 to    A - Z 
    // 0011111 mask
    // -1 
    u64 result = 0;
    if (base == 2) {
        result = symb & 0xF;
    } else if (base == 8) {
        result = symb & 0xF;;
    } else if (base == 16) {
        if ('0' <= symb && symb <= '9') {
            result = symb & 0xF;;
        } else {
            result = (symb & 0x1F) - 1;
        }
    } else if (base == 10) {
        result = symb & 0xF;;
    } else {
        UNREACHABLE;
    }
    return result;
}

static u64 
str_to_u64_base(Lexer *lexer, u32 base) {
    u64 value = 0;
    u8 codepoint = peek_codepoint(lexer);
    while (codepoint && is_symb_base(codepoint, base)) {
        value = value * base + symb_to_base(codepoint, base);
        advance(lexer, 1);
        codepoint = peek_codepoint(lexer);
    }
    return value;
}

static void 
parse_number(Lexer *lexer, Token *token) {
    // https://en.cppreference.com/w/c/language/integer_constant
    u32 base = 10;
    if (parse(lexer, "0x")) {
        base = 16;
    } else if (parse(lexer, "0")) {
        base = 8;
    } else if (parse(lexer, "0b")) {
        base = 2;
    };
    
    u64 value = str_to_u64_base(lexer, base);
    u32 mask = 0;
    enum { MASK_L = 0x1, MASK_LL = 0x2, MASK_U = 0x4 };
    if (parse(lexer, "LLU") || parse(lexer, "LLu") ||
        parse(lexer, "llU") || parse(lexer, "llu") ||
        parse(lexer, "ULL") || parse(lexer, "Ull") ||
        parse(lexer, "uLL") || parse(lexer, "ull")) {
        mask = MASK_LL | MASK_U;
    } else if (parse(lexer, "ll") || parse(lexer, "LL")) {
        mask = MASK_LL;
    } else if (parse(lexer, "LU") || parse(lexer, "Lu") || 
               parse(lexer, "lU") || parse(lexer, "lu")) {
        mask = MASK_L | MASK_U;
    } else if (parse(lexer, "L") || parse(lexer, "l")) {
        mask = MASK_L;
    } else if (parse(lexer, "U") || parse(lexer, "u")) {
        mask = MASK_U;
    }
    
    // Infer type
    u32 type;
    if (base == 10) {
        if (mask & (MASK_LL | MASK_U)) {
            type = C_TYPE_ULLINT;;
        } else if (mask & MASK_LL) {
            type = C_TYPE_SLLINT;
        } else if (mask & (MASK_L & MASK_U)) {
            if (value <= ULONG_MAX) {
                type = C_TYPE_ULINT;
            } else {
                type = C_TYPE_ULLINT;
            }
        } else if (mask & MASK_L) {
            if (value <= LONG_MAX) {
                type = C_TYPE_SLINT;
            } else {
                type = C_TYPE_SLLINT;
            }
        } else if (mask & MASK_U) {
            if (value <= UINT_MAX) {
                type = C_TYPE_UINT;
            } else if (value <= ULONG_MAX) {
                type = C_TYPE_ULINT;
            } else {
                type = C_TYPE_ULLINT;
            }
        } else {
            if (value <= INT_MAX) {
                type = C_TYPE_SINT;
            } else if (value <= LONG_MAX) {
                type = C_TYPE_SLINT;
            } else {
                type = C_TYPE_SLLINT;
            }
        }
    } else {
        if (mask & (MASK_LL | MASK_U)) {
            type = C_TYPE_ULLINT;;
        } else if (mask & MASK_LL) {
            if (value <= LLONG_MAX) {
                type = C_TYPE_SLLINT;    
            } else {
                type = C_TYPE_ULLINT;
            }
        } else if (mask & (MASK_L & MASK_U)) {
            if (value <= ULONG_MAX) {
                type = C_TYPE_ULINT;
            } else {
                type = C_TYPE_ULLINT;
            }
        } else if (mask & MASK_L) {
            if (value <= LONG_MAX) {
                type = C_TYPE_SLINT;
            } else if (value <= ULONG_MAX) {
                type = C_TYPE_ULINT;
            } else if (value <= LLONG_MAX) {
                type = C_TYPE_SLLINT;    
            } else {
                type = C_TYPE_ULLINT;
            }
        } else if (mask & MASK_U) {
            if (value <= UINT_MAX) {
                type = C_TYPE_UINT;
            } else if (value <= ULONG_MAX) {
                type = C_TYPE_ULINT;
            } else {
                type = C_TYPE_ULLINT;
            }
        } else {
            if (value <= INT_MAX) {
                type = C_TYPE_SINT;
            } else if (value <= UINT_MAX) {
                type = C_TYPE_UINT;
            }else if (value <= LONG_MAX) {
                type = C_TYPE_SLINT;
            } else if (value <= ULONG_MAX) {
                type = C_TYPE_ULINT;
            } else if (value <= LLONG_MAX) {
                type = C_TYPE_SLLINT;
            } else {
                type = C_TYPE_ULLINT;
            }
        }
    }
    
    token->kind = TOKEN_NUMBER;
    token->number.type = type;
    token->number.int_value = value;
}

static void
parse_character_literal(Lexer *lexer, Token *token) {
    
}

static Token_Stack_Entry *
get_new_stack_entry(Lexer *lexer) {
    Token_Stack_Entry *entry = lexer->token_stack;
    if (!entry) {
        entry = arena_alloc_struct(lexer->arena, Token_Stack_Entry);
    } else {
        entry->token = 0;
    }
    return entry;
}

void 
add_token_to_stack(Lexer *lexer, Token *token) {
    Token_Stack_Entry *stack_entry = get_new_stack_entry(lexer);
    stack_entry->token = token;
    STACK_ADD(lexer->token_stack, stack_entry);
    ++lexer->token_stack_size;
}

void 
pop_token_from_stack(Lexer *lexer) {
    assert(lexer->token_stack_size);
    --lexer->token_stack_size;
    Token_Stack_Entry *stack_entry = lexer->token_stack;
    STACK_POP(lexer->token_stack);
    STACK_ADD(lexer->token_stack_freelist, stack_entry);
}

Token *
get_current_token(Lexer *lexer) {
    Token *result = 0;
    if (lexer->token_stack_size) {
        result = lexer->token_stack->token;
    }
    return result;
}

static Lexer_Buffer *
get_new_buffer_stack_entry(Lexer *lexer) {
    Lexer_Buffer *result = lexer->buffer_freelist;
    if (result) {
        STACK_POP(lexer->buffer_freelist);
        mem_zero(result, sizeof(*result));
    } else {
        result = arena_alloc_struct(lexer->arena, Lexer_Buffer);
    }
    return result;
}

void 
add_buffer_to_stack(Lexer *lexer, Lexer_Buffer *entry) {
    STACK_ADD(lexer->buffer_stack, entry);
}

void 
add_buffer_to_stack_file(Lexer *lexer, const char *filename) {
    File_ID parent_id = {0};
    {
        Lexer_Buffer *entry = get_current_buf(lexer);
        if (entry) {
            parent_id = entry->file_id;
        }    
    }
    File_ID new_id = register_file(lexer->ctx->fr, filename, parent_id);
    File_Data_Get_Result get_result = get_file_data(lexer->ctx->fr, new_id);
    if (get_result.is_valid) {
        File_Data data = get_result.data;
    
        Lexer_Buffer *entry = get_new_buffer_stack_entry(lexer);
        entry->buf = entry->at = data.data;
        entry->size = data.data_size;
        entry->file_id = new_id;
        STACK_ADD(lexer->buffer_stack, entry);       
        ++lexer->include_nesting;
    } else {
        NOT_IMPLEMENTED;
    }
}

void 
pop_buffer_from_stack(Lexer *lexer) {
    Lexer_Buffer *entry = lexer->buffer_stack;
    if (!entry->is_pp) {
        // assert(lexer->include_nesting);
        --lexer->include_nesting;
    }
    STACK_POP(lexer->buffer_stack);
    STACK_ADD(lexer->buffer_freelist, entry);
}

Lexer_Buffer *
get_current_buf(Lexer *lexer) {
    return lexer->buffer_stack;
}

PP_Macro *
pp_get(Lexer *lexer, const char *name) {
    PP_Macro *result = 0;
    u32 name_length = zlen(name);
    u64 name_hash = LEX_STR_HASH_FUNC(name, name_length);
    Hash_Table64_Get_Result get_result = hash_table64_get(&lexer->macro_hash, name_hash);
    if (get_result.is_valid) {
        u32 slot_idx = get_result.value;
        PP_Macro *slot = lexer->macros + slot_idx;
        result = slot;
        assert(result->id == name_hash);
    } 
    return result;
}

PP_Macro * 
pp_define(Lexer *lexer, const char *name) {
    PP_Macro *result = 0;
    u32 name_length = zlen(name);
    u64 name_hash = LEX_STR_HASH_FUNC(name, name_length);
    Hash_Table64_Get_Result get_result = hash_table64_get(&lexer->macro_hash, name_hash);
    u32 slot_idx = (u32)-1;
    if (get_result.is_valid) {
        slot_idx = get_result.value;
        assert(lexer->macros[slot_idx].id == name_hash);
    } else {
        slot_idx = lexer->next_macro_slot++;
        hash_table64_set(&lexer->macro_hash, name_hash, slot_idx);
    }
    
    assert(slot_idx != (u32)-1);
    
    result = lexer->macros + slot_idx;
    mem_zero(result, sizeof(*result));
    result->name = name;
    result->id = name_hash;
    
    return result;
}

void 
pp_undef(Lexer *lexer, const char *name) {
    u32 name_length = zlen(name);
    u64 name_hash = LEX_STR_HASH_FUNC(name, name_length);
    hash_table64_delete(&lexer->macro_hash, name_hash);
}


void 
pp_push_nested_if(Lexer *lexer, bool is_handled) {
    assert(lexer->nested_if_cursor + 1 < MAX_NESTED_IFS);
    PP_Nested_If *ifs = lexer->nested_ifs + lexer->nested_if_cursor++;
    ifs->is_handled = is_handled;
}

bool 
pp_get_nested_if_handled(Lexer *lexer) {
    assert(lexer->nested_if_cursor);
    return lexer->nested_ifs[lexer->nested_if_cursor - 1].is_handled;
}

void 
pp_set_nested_if_handled(Lexer *lexer) {
    assert(lexer->nested_if_cursor);
    lexer->nested_ifs[lexer->nested_if_cursor - 1].is_handled = true;
}

void 
pp_pop_nested_if(Lexer *lexer)  {
    assert(lexer->nested_if_cursor);
    --lexer->nested_if_cursor;
}

u8 
peek_codepoint(Lexer *lexer) {
    u8 result = 0;
    Lexer_Buffer *buffer = get_current_buf(lexer);
    while (buffer && !result) {
        result = lexbuf_peek(buffer);
        if (!result) {
            assert(buffer != buffer->next); // detect infinite loop
            pop_buffer_from_stack(lexer);
            buffer = get_current_buf(lexer);
        }
    }
    return result;
}
void 
advance(Lexer *lexer, u32 n) {
    Lexer_Buffer *buffer = get_current_buf(lexer);
    while (n && buffer) {
        n -= lexbuf_advance(buffer, n);
        buffer = get_current_buf(lexer);
    }
}

bool 
parse(Lexer *lexer, const char *lit) {
    bool result = false;
    Lexer_Buffer *buffer = get_current_buf(lexer);
    if (buffer) {
        result = lexbuf_parse(buffer, lit);
    }
    return result;
}

bool 
skip_spaces(Lexer *lexer) {
    bool skipped = false;
    while (is_space(peek_codepoint(lexer))) {
        advance(lexer, 1);
        skipped = true;
    }
    return skipped;
}

bool
pp_skip_spaces(Lexer *lexer) {
    bool skipped = false;
    for (;;) {
        u8 codepoint = peek_codepoint(lexer);
        if (is_space(codepoint) && codepoint != '\n') {
            skipped = true;
            advance(lexer, 1);
        } else {
            break;
        }
    }
    return skipped;
}

static Token *
pp_peek_tok(Lexer *lexer) {
    Token *token = get_current_token(lexer);
    if (token) {
        return token;
    }
    
    token = arena_alloc_struct(lexer->arena, Token);
    
    for (;;) {
        {
            u8 codepoint = peek_codepoint(lexer);
            if (!codepoint || 
                (codepoint == '\n')) {
                token->kind = TOKEN_EOS;
                break;
            }
        }
        
        if (pp_skip_spaces(lexer)) {
            continue;
        }
        if (parse(lexer, "//")) {
            while (get_current_buf(lexer) && peek_codepoint(lexer) != '\n') {
                advance(lexer, 1);
            }
            token->kind = TOKEN_EOS;
            break;
        } else if (parse(lexer, "/*")) {
            for (;;) {
                if (!get_current_buf(lexer) || parse(lexer, "*/")) {
                    break;
                }
                advance(lexer, 1);
            }
            continue;
        }
        
        // all cases below should break the loop
        token->src_loc = lexer->curr_loc;
        if (is_digit(peek_codepoint(lexer))) {
            parse_number(lexer, token);
            break;
        } else if (is_ident_start(peek_codepoint(lexer))) {
            char buf[4096];
            u32 len = 0;
            for (;;) {
                u8 codepoint = peek_codepoint(lexer);
                if (is_ident(codepoint)) {
                    buf[len++] = codepoint;
                    advance(lexer, 1);
                } else {
                    break;
                }
            }
            buf[len] = 0;
            u32 keyword_enum = 0;
            u32 keyword_iter = 0;
            ITER_PREPROCESSOR_KEYWORD(keyword_iter) {
                if (zeq(buf, PREPROCESSOR_KEYWORD_STRINGS[keyword_iter])) {
                    keyword_enum = keyword_iter;
                    break;
                }
            }
            
            if (keyword_enum) {
                token->kind = TOKEN_KEYWORD;
                token->kw = keyword_enum;
            } else {
                token->kind = TOKEN_IDENT;
                token->ident.str = mem_alloc_str(buf);
            }
            break;
        } else if (peek_codepoint(lexer) == '\"' || peek_codepoint(lexer) == '<') {
            advance(lexer, 1);
            char buf[4096];
            u32 len = 0;
            for (;;) {
                u8 codepoint = peek_codepoint(lexer);
                if (codepoint == '\"' || codepoint == '>') {
                    break;
                } else {
                    buf[len++] = codepoint;
                    advance(lexer, 1);
                }
            }
            advance(lexer, 1);
            buf[len] = 0;
            
            token->kind = TOKEN_PP_FILENAME;
            token->filename.str = mem_alloc_str(buf);
            break;
        } else if (is_punct(peek_codepoint(lexer))) {
            u32 punct_enum = 0;
            u32 punct_iter = 0;
            ITER_PUNCTUATORS(punct_iter) {
                if (parse(lexer, PUNCTUATOR_STRINGS[punct_iter - 0x100])) {
                    punct_enum = punct_iter;
                    break;
                }
            }
            
            token->kind = TOKEN_PUNCTUATOR;
            if (punct_enum) {
                token->punct = punct_enum;
            } else {
                token->punct = peek_codepoint(lexer);
                advance(lexer, 1);
            }
            break;
        } else {
            NOT_IMPLEMENTED;
        }
    }
    
    add_token_to_stack(lexer, token);
    
    return token;
}

static void 
pp_skip_to_next_matching_if(Lexer *lexer) {
    u32 depth = 0;
    for (;;) {
        u8 codepoint = peek_codepoint(lexer);
        if (codepoint == '#') {
            advance(lexer, 1);
            Token *token = pp_peek_tok(lexer);
            if (token->kind == TOKEN_KEYWORD) {
                if (token->kw == PP_KEYWORD_IF || 
                    token->kw == PP_KEYWORD_IFDEF ||
                    token->kw == PP_KEYWORD_IFNDEF) {
                    ++depth;       
                    eat_tok(lexer);
                } else if ((token->kw == PP_KEYWORD_ELIF || 
                            token->kw == PP_KEYWORD_ELIFDEF || 
                            token->kw == PP_KEYWORD_ELIFNDEF || 
                            token->kw == PP_KEYWORD_ENDIF ||
                            token->kw == PP_KEYWORD_ELSE) && depth != 0) {
                    if (token->kw == PP_KEYWORD_ENDIF) {
                        --depth;
                    } 
                    eat_tok(lexer);
                } else if ((token->kw == PP_KEYWORD_ELIF || 
                            token->kw == PP_KEYWORD_ELIFDEF || 
                            token->kw == PP_KEYWORD_ELIFNDEF || 
                            token->kw == PP_KEYWORD_ENDIF || 
                            token->kw == PP_KEYWORD_ELSE) && depth == 0) {
                    break;
                } else {
                    eat_tok(lexer);
                }
            } else {
                advance(lexer, 1);
            }
        } else if (!codepoint) {
            break;
        } else {
            advance(lexer, 1);
        }
    }
    assert(depth == 0);
}

static void 
parse_preprocessor_directive(Lexer *lexer) {
    Token *token = 0;
    // @NOTE(hl): This is used to parse whole if statement as a single directive,
    // eating last endif
start:
    token = pp_peek_tok(lexer);
    if (token->kind == TOKEN_KEYWORD) {
        switch(token->kw) {
        INVALID_DEFAULT_CASE;
        case PP_KEYWORD_DEFINE: {
            eat_tok(lexer);
            token = pp_peek_tok(lexer);
            if (token->kind == TOKEN_IDENT) {
                const char *macro_name = token->ident.str;
                eat_tok(lexer);
                PP_Macro *macro = pp_define(lexer, macro_name);
                if (peek_codepoint(lexer) == '(') {
                    advance(lexer, 1);
                    macro->is_function_like = true;
                    while (token->kind != TOKEN_EOS && 
                        !(token->kind == TOKEN_PUNCTUATOR && token->punct == ')')) {
                        token = pp_peek_tok(lexer);
                        if (token->kind == TOKEN_IDENT) {
                            if (macro->has_varargs) {
                                NOT_IMPLEMENTED;
                            } else {
                                assert(macro->arg_count + 1 < MAX_PP_MACRO_ARGS);
                                macro->arg_names[macro->arg_count++] = token->ident.str;
                            }
                        } else if (token->kind == TOKEN_PUNCTUATOR && 
                                   token->punct == PUNCTUATOR_VARARGS) {
                            macro->has_varargs = true;
                        } 
                        
                        eat_tok(lexer);
                        token = pp_peek_tok(lexer);
                        if (token->kind == TOKEN_PUNCTUATOR && token->punct == ',') {
                            if (macro->has_varargs) {
                                NOT_IMPLEMENTED;
                            }
                            eat_tok(lexer);
                            token = pp_peek_tok(lexer);
                        }
                    }
                    
                    assert(token->kind == TOKEN_PUNCTUATOR && token->punct == ')');
                    eat_tok(lexer);
                }
                // select macro definition up to end of the line
                // If \\n is met, continue parsing line as if it was one
                for (;;) {
                    if (parse(lexer, "\\\n")) {
                        // advance(lexer, 2);
                    } else {
                        u8 codepoint = peek_codepoint(lexer);
                        if (codepoint == '\n') {
                            break;
                        } else {
                            assert(macro->definition_len < MAX_PREPROCESSOR_LINE_LENGTH);
                            macro->definition[macro->definition_len++] = codepoint;
                            advance(lexer, 1);
                        }
                    }
                }
            } else {
                NOT_IMPLEMENTED;
            }
        } break;
        case PP_KEYWORD_UNDEF: {
            eat_tok(lexer);
            token = pp_peek_tok(lexer);
            if (token->kind == TOKEN_IDENT) {
                const char *macro_name = token->ident.str;
                eat_tok(lexer);
                token = pp_peek_tok(lexer);
                if (token->kind == TOKEN_EOS) {
                    eat_tok(lexer);
                    pp_undef(lexer, macro_name);
                } else {
                    NOT_IMPLEMENTED;
                }
            } else {
                NOT_IMPLEMENTED;
            }
        } break;
        case PP_KEYWORD_INCLUDE: {
            eat_tok(lexer);
            token = pp_peek_tok(lexer);
            if (token->kind == TOKEN_PP_FILENAME) {
                const char *filename = token->filename.str;
                eat_tok(lexer);
                token = pp_peek_tok(lexer);
                if (token->kind == TOKEN_EOS) {
                    eat_tok(lexer);
                    add_buffer_to_stack_file(lexer, filename);
                } else {
                    NOT_IMPLEMENTED;
                }
            } else {
                NOT_IMPLEMENTED;
            }
        } break;
        case PP_KEYWORD_IFDEF: {
            eat_tok(lexer);
            token = pp_peek_tok(lexer);
            if (token->kind == TOKEN_IDENT) {
                const char *ident_name = token->ident.str;
                eat_tok(lexer);
                token = pp_peek_tok(lexer);
                if (token->kind == TOKEN_EOS) {
                    eat_tok(lexer);
                    PP_Macro *macro = pp_get(lexer, ident_name);
                    bool is_true = macro != 0;
                    pp_push_nested_if(lexer, is_true);
                    if (!is_true) {
                        pp_skip_to_next_matching_if(lexer);
                        goto start;
                    }
                } else {
                    NOT_IMPLEMENTED;
                }
            } else {
                NOT_IMPLEMENTED;
            }
        } break;
        case PP_KEYWORD_IFNDEF: {
            eat_tok(lexer);
            token = pp_peek_tok(lexer);
            if (token->kind == TOKEN_IDENT) {
                const char *ident_name = token->ident.str;
                eat_tok(lexer);
                token = pp_peek_tok(lexer);
                if (token->kind == TOKEN_EOS) {
                    eat_tok(lexer);
                    PP_Macro *macro = pp_get(lexer, ident_name);
                    bool is_true = macro == 0;
                    pp_push_nested_if(lexer, is_true);
                    if (!is_true) {
                        pp_skip_to_next_matching_if(lexer);
                        goto start;
                    }
                } else {
                    NOT_IMPLEMENTED;
                }
            } else {
                NOT_IMPLEMENTED;
            }
        } break;
        case PP_KEYWORD_ENDIF: {
            eat_tok(lexer);
            token = pp_peek_tok(lexer);
            if (token->kind == TOKEN_EOS) {
                pp_pop_nested_if(lexer);
            } else {
                NOT_IMPLEMENTED;
            }
        } break;
        case PP_KEYWORD_ELSE: {
            eat_tok(lexer);
            token = pp_peek_tok(lexer);
            if (token->kind == TOKEN_EOS) {
                eat_tok(lexer);
                assert(lexer->nested_if_cursor != 0);
                if (pp_get_nested_if_handled(lexer)) {
                    pp_skip_to_next_matching_if(lexer);
                } else {
                    pp_set_nested_if_handled(lexer);
                }
            } else {
                NOT_IMPLEMENTED;
            }
        } break;
        case PP_KEYWORD_ELIFDEF: {
            eat_tok(lexer);token = pp_peek_tok(lexer);
            if (token->kind == TOKEN_IDENT) {
                const char *ident_name = token->ident.str;
                eat_tok(lexer);
                token = pp_peek_tok(lexer);
                if (token->kind == TOKEN_EOS) {
                    eat_tok(lexer);
                    PP_Macro *macro = pp_get(lexer, ident_name);
                    bool is_true = macro != 0;
                    assert(lexer->nested_if_cursor != 0);
                    if (pp_get_nested_if_handled(lexer) || !is_true) {
                        pp_skip_to_next_matching_if(lexer);
                    } else if (is_true) {
                        pp_set_nested_if_handled(lexer);
                    }
                } else {
                    NOT_IMPLEMENTED;
                }
            } else {
                NOT_IMPLEMENTED;
            }
        } break;
        case PP_KEYWORD_ELIFNDEF: {
            eat_tok(lexer);token = pp_peek_tok(lexer);
            if (token->kind == TOKEN_IDENT) {
                const char *ident_name = token->ident.str;
                eat_tok(lexer);
                token = pp_peek_tok(lexer);
                if (token->kind == TOKEN_EOS) {
                    eat_tok(lexer);
                    PP_Macro *macro = pp_get(lexer, ident_name);
                    bool is_true = macro == 0;
                    assert(lexer->nested_if_cursor != 0);
                    if (pp_get_nested_if_handled(lexer) || !is_true) {
                        pp_skip_to_next_matching_if(lexer);
                    } else if (is_true) {
                        pp_set_nested_if_handled(lexer);
                    }
                } else {
                    NOT_IMPLEMENTED;
                }
            } else {
                NOT_IMPLEMENTED;
            }
        } break;
        case PP_KEYWORD_IF: {
            eat_tok(lexer);
            NOT_IMPLEMENTED;
        } break;
        case PP_KEYWORD_ELIF: {
            eat_tok(lexer);
            NOT_IMPLEMENTED;
        } break;
        case PP_KEYWORD_PRAGMA: {
            NOT_IMPLEMENTED;
        } break;
        }
    } else if (token->kind != TOKEN_EOS) {
        NOT_IMPLEMENTED;
    } 
}

static void 
resolve_macro(Lexer *lexer, PP_Macro *macro) {
    Lexer_Buffer *buffer = get_new_buffer_stack_entry(lexer);
    buffer->buf = buffer->at = macro->definition;
    buffer->size = macro->definition_len;
    buffer->is_pp = true;
    buffer->macro = macro;
    if (macro->is_function_like) {
        u8 codepoint = peek_codepoint(lexer);
        if (codepoint == '(') {
            advance(lexer, 1);
            codepoint = peek_codepoint(lexer);
            char temp_buffer[4096];
            u32  temp_buffer_size = 0;
            while (codepoint != ')') {
                if (codepoint == ',') {
                    // @HACK Currently we insert space so the algorithm can function 
                    // normally and parse macro properly if it is contained in another macro
                    // argument
                    temp_buffer[temp_buffer_size++] = ' ';
                    temp_buffer[temp_buffer_size] = 0;
                    buffer->macro_args[buffer->macro_arg_count++] = mem_alloc_str(temp_buffer);
                    temp_buffer_size = 0;
                } else {
                    temp_buffer[temp_buffer_size++] = codepoint;
                }
                advance(lexer, 1);
                codepoint = peek_codepoint(lexer);
            }
            // @TODO DRY
            if (temp_buffer_size) {
                temp_buffer[temp_buffer_size++] = ' ';
                temp_buffer[temp_buffer_size] = 0;
                buffer->macro_args[buffer->macro_arg_count++] = mem_alloc_str(temp_buffer);
            }
            advance(lexer, 1);
        } else {
            NOT_IMPLEMENTED;
        } 
    }
    add_buffer_to_stack(lexer, buffer);
}

Token *
peek_tok(Lexer *lexer) {
    Token *token = get_current_token(lexer);
    if (token) {
        return token;
    }
    
    token = arena_alloc_struct(lexer->arena, Token);
    
    for (;;) {
        if (!peek_codepoint(lexer)) {
            token->kind = TOKEN_EOS;
            break;
        }
        
        // 'skipping' cases
        if (skip_spaces(lexer)) {
            continue;
        } else if (parse(lexer, "//")) {
            for (;;) {
                u8 codepoint = peek_codepoint(lexer);
                if (codepoint == '\n') {
                    advance(lexer, 1);
                    break;
                } else if (!codepoint) {
                    break;
                }
                advance(lexer, 1);
            }
            continue;
        } else if (parse(lexer, "/*")) {
            for (;;) {
                if (parse(lexer, "*/") || !peek_codepoint(lexer)) {
                    break;
                }
            }
            continue;
        } else if (peek_codepoint(lexer) == '#') {
            advance(lexer, 1);
            parse_preprocessor_directive(lexer);
            continue;
        } 
        
        // all cases below should break the loop
        token->src_loc = lexer->curr_loc;
        if (is_digit(peek_codepoint(lexer))) {
            parse_number(lexer, token);
            break;
        } else if (is_ident_start(peek_codepoint(lexer))) {
            char buf[4096];
            u32 len = 0;
            for (;;) {
                u8 codepoint = peek_codepoint(lexer);
                if (is_ident(codepoint)) {
                    buf[len++] = codepoint;
                    advance(lexer, 1);
                } else {
                    break;
                }
            }
            buf[len] = 0;
            
            // First, check if we are inside function-like macro expansion
            {
                bool is_macro_argument = false;
                for (Lexer_Buffer *lexbuf = get_current_buf(lexer);
                     lexbuf && !is_macro_argument;
                     lexbuf = lexbuf->next) {
                    if (lexbuf->is_pp) {
                        PP_Macro *macro = lexbuf->macro;
                        for (u32 macro_arg_name_idx = 0;
                             macro_arg_name_idx < macro->arg_count;
                             ++macro_arg_name_idx) {
                            const char *test = macro->arg_names[macro_arg_name_idx];
                            if (zeq(test, buf)) {
                                const char *arg_expansion = lexbuf->macro_args[macro_arg_name_idx];
                                u32 arg_expansion_len = zlen(arg_expansion);
                                Lexer_Buffer *newbuf = get_new_buffer_stack_entry(lexer);
                                newbuf->buf = newbuf->at = arg_expansion;
                                newbuf->size = arg_expansion_len;
                                is_macro_argument = true;
                                add_buffer_to_stack(lexer, newbuf);
                                break;
                            }
                        }
                    }
                }   
                
                if (is_macro_argument) {
                    continue;
                }
            }
            
            // Try to get macro
            PP_Macro *macro = pp_get(lexer, buf);
            if (macro) {
                resolve_macro(lexer, macro);
                continue;
            }
            
            // If macro does not exist, try to find keyword 
            u32 keyword_enum = 0;
            u32 keyword_iter = 0;
            ITER_KEYWORDS(keyword_iter) {
                if (zeq(buf, KEYWORD_STRINGS[keyword_iter])) {
                    keyword_enum = keyword_iter;
                    break;
                }
            }
            // If keyword does no exist, use it as identifier
            
            if (keyword_enum) {
                token->kind = TOKEN_KEYWORD;
                token->kw = keyword_enum;
            } else {
                token->kind = TOKEN_IDENT;
                token->ident.str = mem_alloc_str(buf);
            }
            break;
        } else if (peek_codepoint(lexer) == '\'') {
            parse_character_literal(lexer, token);
            break;    
        } else if (peek_codepoint(lexer) == '\"') {
            NOT_IMPLEMENTED;
            break;
        } else if (is_punct(peek_codepoint(lexer))) {
            u32 punct_enum = 0;
            u32 punct_iter = 0;
            ITER_PUNCTUATORS(punct_iter) {
                if (parse(lexer, PUNCTUATOR_STRINGS[punct_iter - 0x100])) {
                    punct_enum = punct_iter;
                    break;
                }
            }
            
            token->kind = TOKEN_PUNCTUATOR;
            if (punct_enum) {
                token->punct = punct_enum;
            } else {
                token->punct = peek_codepoint(lexer);
                advance(lexer, 1);
            }
            break;
        } else {
            assert(false);
        }
    }
    
    add_token_to_stack(lexer, token);
    
    return token;
}

void 
eat_tok(Lexer *lexer) {
    pop_token_from_stack(lexer);
}

Lexer *
create_lexer(struct Compiler_Ctx *ctx, const char *filename) {
    Lexer *lexer = arena_bootstrap(Lexer, arena);
    lexer->ctx = ctx;
    lexer->macro_hash.num_buckets = MAX_PREPROCESSOR_MACROS;
    lexer->macro_hash.keys = arena_alloc_array(lexer->arena, MAX_PREPROCESSOR_MACROS, u64);
    lexer->macro_hash.values = arena_alloc_array(lexer->arena, MAX_PREPROCESSOR_MACROS, u64);
    add_buffer_to_stack_file(lexer, filename);
    
    return lexer;
}