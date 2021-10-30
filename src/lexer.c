#include "lexer.h"

#include "lib/memory.h"
#include "lib/lists.h"
#include "lib/strings.h"
#include "lib/stream.h"
#include "lib/text_file_lexer.h"

#include "ast.h"
#include "string_storage.h"
#include "compiler_ctx.h"
#include "file_registry.h"
#include "error_reporter.h"
#include "type_table.h"

#define LEX_STR_HASH_FUNC    fnv64
#define LEX_SCRATCH_BUF_SIZE KB(16)

#define ITER_KEYWORDS(_it) \
for ((_it) = KEYWORD_AUTO; (_it) < KEYWORD_SENTINEL; ++(_it))
#define ITER_PREPROCESSOR_KEYWORD(_it) \
for ((_it) = PP_KEYWORD_DEFINE; (_it) < PP_KEYWORD_SENTINEL; ++(_it))
#define ITER_PUNCTUATORS(_it) \
for ((_it) = PUNCTUATOR_IRSHIFT; (_it) < PUNCTUATOR_SENTINEL; ++(_it))

enum { 
    CHAR_LIT_REG  = 0x1, // '
    CHAR_LIT_UTF8 = 0x2, // u8'
    CHAR_LIT_U16  = 0x3, // u'
    CHAR_LIT_U32  = 0x4, // U'
    CHAR_LIT_WIDE = 0x5  // L'
};

enum {
    STRING_LIT_REG  = 0x1, // "
    STRING_LIT_UTF8 = 0x2, // u8"
    STRING_LIT_U16  = 0x3, // u"
    STRING_LIT_U32  = 0x4, // U"
    STRING_LIT_WIDE = 0x5, // L"
};

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
    "else",
    "enum",
    "extern",
    "float",
    "for",
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
    "union",
    "unsigned",
    "void",
    "volatile",
    "while",
    "_Alignas",
    "_Alignof",
    "_Atomic",
    "_Bool",
    "_Complex",
    "_Decimal128",
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
    "endif",
    // "__VA_ARGS__"
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
    "##",
};

// static const char *ALPHABET       = "0123456789ABCDEF";
// static const char *ALPHABET_LOWER = "0123456789abcdef";

u32 
fmt_token_kind(Out_Stream *stream, u32 kind) {
    u32 result = 0;
    switch (kind) {
    INVALID_DEFAULT_CASE;
    case TOKEN_EOS: {
        result = out_streamf(stream, "<eos>");
    } break;
    case TOKEN_IDENT: {
        result = out_streamf(stream, "<ident>");
    } break;
    case TOKEN_KEYWORD: {
        result = out_streamf(stream, "<kw>");
    } break;
    case TOKEN_STRING: {
        result = out_streamf(stream, "<str>");
    } break;
    case TOKEN_NUMBER: {
        result = out_streamf(stream, "<num>");
    } break;
    case TOKEN_PUNCTUATOR: {
        result = out_streamf(stream, "<punct>");
    } break;
    }    
    return result;
}

u32 
fmt_token(Out_Stream *stream, Token *token) {
    u32 result = 0;
    switch (token->kind) {
    INVALID_DEFAULT_CASE;
    case TOKEN_EOS: {
        result = out_streamf(stream, "<eos>");
    } break;
    case TOKEN_IDENT: {
        result = out_streamf(stream, "<ident>%s", token->ident.str);
    } break;
    case TOKEN_KEYWORD: {
        result = out_streamf(stream, "<kw>%s", KEYWORD_STRINGS[token->kw]);
    } break;
    case TOKEN_STRING: {
        result = out_streamf(stream, "<str>%s", token->str.str);
    } break;
    case TOKEN_NUMBER: {
        result = out_streamf(stream, "<num>%lld", 
            token->number.sint_value);
    } break;
    case TOKEN_PUNCTUATOR: {
        if (token->punct < 0x100) {
            result += out_streamf(stream, "<punct>%c", token->punct);
        } else {
            result += out_streamf(stream, "<punct>%s", 
                PUNCTUATOR_STRINGS[token->punct - 0x100]);
        }
    } break;
    }    
    return result;
}


u32 
fmt_token_as_code(struct Out_Stream *stream, Token *token) {
        u32 result = 0;
    switch (token->kind) {
    INVALID_DEFAULT_CASE;
    case TOKEN_EOS: {
    } break;
    case TOKEN_IDENT: {
        result = out_streamf(stream, "%s", token->ident.str);
    } break;
    case TOKEN_KEYWORD: {
        result = out_streamf(stream, "%s", KEYWORD_STRINGS[token->kw]);
    } break;
    case TOKEN_STRING: {
        result = out_streamf(stream, "\"%s\"", token->str.str);
    } break;
    case TOKEN_NUMBER: {
        result = out_streamf(stream, "%lld", 
            token->number.sint_value);
    } break;
    case TOKEN_PUNCTUATOR: {
        if (token->punct < 0x100) {
            result += out_streamf(stream, "%c", token->punct);
        } else {
            result += out_streamf(stream, "%s", 
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
    while (buffer->at < buffer->buf + buffer->size && n--) {
        if (*buffer->at == '\n') {
            ++buffer->line;
            buffer->symb = 0;
        } else {
            ++buffer->symb;
        }
        ++buffer->at;
        ++advanced;
    }
    return advanced;
}

bool 
lexbuf_next_eq(Lexer_Buffer *buffer, const char *lit) {
    u32 length = zlen(lit);
    bool result = false;
    if (buffer->at + length < buffer->buf + buffer->size) {
        result = mem_eq(buffer->at, lit, length);
    }
    return result;
}

bool 
lexbuf_parse(Lexer_Buffer *buffer, const char *lit) {
    bool result = lexbuf_next_eq(buffer, lit);
    if (result) {
        lexbuf_advance(buffer, zlen(lit));
    }
    return result;
}

#if 0 

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
            } else if (value <= LONG_MAX) {
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
    // token->number.uint_value = value;
}

static void
parse_character_literal(Lexer *lexer, Token *token) {
    u8 codepoint = peek_codepoint(lexer);
    assert(codepoint == '\'');
    advance(lexer, 1);
    char char_buf[1024];
    u32 char_buf_len = 0;
    codepoint = peek_codepoint(lexer);
    while (codepoint && codepoint != '\'') {
        char_buf[char_buf_len++] = codepoint;
        advance(lexer, 1);
        codepoint = peek_codepoint(lexer);
    }
    codepoint = peek_codepoint(lexer);
    assert(codepoint == '\'');
    advance(lexer, 1);
    char_buf[char_buf_len] = 0;
    
    int value = 0;
    if (zeq(char_buf, "\\\'")) {
        value = '\'';
    } else if (zeq(char_buf, "\\\"")) {
        value = '\"';
    } else if (zeq(char_buf, "\\\?")) {
        value = '\?';
    } else if (zeq(char_buf, "\\\\")) {
        value = '\\';
    } else if (zeq(char_buf, "\\\a")) {
        value = '\a';
    } else if (zeq(char_buf, "\\\b")) {
        value = '\b';
    } else if (zeq(char_buf, "\\\f")) {
        value = '\f';
    } else if (zeq(char_buf, "\\\n")) {
        value = '\n';
    } else if (zeq(char_buf, "\\\r")) {
        value = '\r';
    } else if (zeq(char_buf, "\\\t")) {
        value = '\t';
    } else if (zeq(char_buf, "\\\v")) {
        value = '\v';
    } else if (char_buf_len == 1) {
        value = char_buf[0];
    } else {
        report_error(lexer->ctx->er, token->src_loc, "Unexpected format in character constant");
    }
    
    token->kind = TOKEN_NUMBER;
    // token->number.sint_value = value;
    token->number.type = C_TYPE_SINT;
    
    char new_buf[4096];l
    fmt(new_buf, sizeof(new_buf), "'%s'", char_buf);
    token->number.string = mem_alloc_str(new_buf);
}

#endif 

static Token_Stack_Entry *
get_new_stack_entry(Lexer *lexer) {
    Token_Stack_Entry *entry = lexer->token_stack_freelist;
    if (!entry) {
        entry = arena_alloc_struct(lexer->arena, Token_Stack_Entry);
    } else {
        STACK_POP(lexer->token_stack_freelist);
        mem_zero_ptr(entry);
    }
    return entry;
}

void 
add_token_to_stack(Lexer *lexer, Token *token) {
    Token_Stack_Entry *stack_entry = get_new_stack_entry(lexer);
    stack_entry->token = token;
    Token_Stack_Entry *last_token_stack = lexer->token_stack;
    if (last_token_stack) {
        while (last_token_stack->next) {
            last_token_stack = last_token_stack->next;
        }
        last_token_stack->next = stack_entry;
    } else {
        lexer->token_stack = stack_entry;
    }
    // STACK_ADD(lexer->token_stack, stack_entry);
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
        mem_zero_ptr(result);
    } else {
        result = arena_alloc_struct(lexer->arena, Lexer_Buffer);
    }
    return result;
}

void 
add_buffer_to_stack(Lexer *lexer, Lexer_Buffer *entry) {
    STACK_ADD(lexer->buffer_stack, entry);
    ++lexer->buffer_stack_size;
    if (entry->kind == LEXER_BUFFER_MACRO) {
        ++lexer->is_in_preprocessor_ctx;
    } 
}

void 
add_buffer_to_stack_file(Lexer *lexer, const char *filename) {
    File_ID parent_id = {0};
    {
        Lexer_Buffer *entry = get_current_buf(lexer); // @TODO Look into parents too
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
        entry->size = data.size;
        entry->file_id = new_id;
        entry->kind = LEXER_BUFFER_FILE;
        add_buffer_to_stack(lexer, entry);
    } else {
        NOT_IMPLEMENTED;
    }
}

void 
add_buffer_to_stack_concat(Lexer *lexer) {
    Lexer_Buffer *entry = get_new_buffer_stack_entry(lexer);
    entry->buf = entry->at = mem_alloc_str(lexer->scratch_buf);
    entry->size = lexer->scratch_buf_size;
    entry->kind = LEXER_BUFFER_CONCAT;
    add_buffer_to_stack(lexer, entry);
}

void 
pop_buffer_from_stack(Lexer *lexer) {
    Lexer_Buffer *entry = lexer->buffer_stack;
    if (entry->kind == LEXER_BUFFER_MACRO) {
        --lexer->is_in_preprocessor_ctx;
    }
    --lexer->buffer_stack_size;
    STACK_POP(lexer->buffer_stack);
    STACK_ADD(lexer->buffer_freelist, entry);
}

Lexer_Buffer *
get_current_buf(Lexer *lexer) {
    return lexer->buffer_stack;
}

static const Src_Loc * 
get_current_loc(Lexer *lexer) { 
    Temp_Memory temp = begin_temp_memory(lexer->arena);
    // Create temporary array for storing buffers in stack-like structure
    u32 count = lexer->buffer_stack_size;
    Lexer_Buffer **buffers = arena_alloc_arr(temp.arena, count, Lexer_Buffer *);
    {
        u32 cursor = count;
        // Populare array in reverse order
        for (Lexer_Buffer *buffer = lexer->buffer_stack;
            buffer;
            buffer = buffer->next) {
            assert(cursor);
            --cursor;
            buffers[cursor] = buffer;
        }
        // Now start generating src locations going from parent to child
        assert(cursor == 0);
    }
    
    const Src_Loc *current_loc = 0;
    for (u32 cursor = 0;
         cursor < count;
         ++cursor) {
        Lexer_Buffer *buffer = buffers[cursor];
        switch (buffer->kind) {
        INVALID_DEFAULT_CASE;
        case LEXER_BUFFER_CONCAT: {
            current_loc = get_src_loc_macro_arg(lexer->ctx->fr, buffer->symb, current_loc);
        } break;
        case LEXER_BUFFER_FILE: {
            current_loc = get_src_loc_file(lexer->ctx->fr, buffer->file_id, buffer->line, 
                buffer->symb, current_loc);
        } break;
        case LEXER_BUFFER_MACRO: {
            current_loc = get_src_loc_macro(lexer->ctx->fr, buffer->macro->loc, buffer->symb, 
                current_loc);
        } break;
        case LEXER_BUFFER_MACRO_ARG: {
            current_loc = get_src_loc_macro_arg(lexer->ctx->fr, buffer->symb, current_loc);
        } break;
        }
    }
    end_temp_memory(temp);
    const Src_Loc *loc = current_loc;
    return loc;
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
    assert(lexer->nested_if_cursor < MAX_NESTED_IFS);
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
next_eq(Lexer *lexer, const char *lit) {
    bool result = false;
    Lexer_Buffer *buffer = get_current_buf(lexer);
    if (buffer) {
        result = lexbuf_next_eq(buffer, lit);
    }
    return result;
}

bool 
parse(Lexer *lexer, const char *lit) {
    bool result = false;
    Lexer_Buffer *buffer = get_current_buf(lexer);
    if (buffer) {
        result = lexbuf_parse(buffer, lit);
        if (result) {
            u32 len = zlen(lit);
            mem_copy(lexer->scratch_buf + lexer->scratch_buf_size, lit, len);
            lexer->scratch_buf_size += len;
        }
    }
    return result;
}

bool 
parse_no_scratch(Lexer *lexer, const char *lit) {
    bool result = false;
    Lexer_Buffer *buffer = get_current_buf(lexer);
    while (!lexbuf_peek(buffer)) {
        pop_buffer_from_stack(lexer);
        buffer = get_current_buf(lexer);
    }
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
        token->src_loc = get_current_loc(lexer); 
        if (is_digit(peek_codepoint(lexer))) {
            // parse_number(lexer, token);
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

// Precedence:
// 0: Ident, Literal
// 1: ! ~ (type) sizeof() _Alignof()
// 2: * / %
// 3: + -
// 4: << >>
// 5: < <=
// 6: > >=
// 7: == !=
// 8: &
// 9: |
// 10: &&
// 10: ||
// 10: ? :
// 10: ,

static Ast *
pp_parse_expr(Lexer *lexer) {
    // Token *token = pp_peek_tok(lexer);
    
    NOT_IMPLEMENTED;   
    return 0;
}
#if 0
static i64
pp_eval_expr(Lexer *lexer, Ast *expr) {
    i64 result = 0;
    
    switch (expr->ast_kind) {
    case AST_BINARY: {
        Ast_Binary *binary = (Ast_Binary *)expr;
        i64 left_eval = pp_eval_expr(lexer, binary->left);
        i64 right_eval = pp_eval_expr(lexer, binary->right);
        switch (binary->kind) {
        INVALID_DEFAULT_CASE;
        case AST_BINARY_COMMA: {
            result =  right_eval;
        } break;
        case AST_BINARY_ADD: {
            result = left_eval + right_eval;
        } break;
        case AST_BINARY_SUB: {
            result = left_eval - right_eval;
        } break;
        case AST_BINARY_MUL: {
            result = left_eval * right_eval;
        } break;
        case AST_BINARY_DIV: {
            result = left_eval / right_eval;
        } break;
        case AST_BINARY_MOD: {
            result = left_eval % right_eval;
        } break;
        case AST_BINARY_LE: {
            result = left_eval <= right_eval;
        } break;
        case AST_BINARY_L: {
            result = left_eval < right_eval;
        } break;
        case AST_BINARY_GE: {
            result = left_eval >= right_eval;
        } break;
        case AST_BINARY_G: {
            result = left_eval > right_eval;
        } break;
        case AST_BINARY_EQ: {
            result = left_eval == right_eval;
        } break;
        case AST_BINARY_NEQ: {
            result = left_eval != right_eval;
        } break;
        case AST_BINARY_AND: {
            result = left_eval & right_eval;
        } break;
        case AST_BINARY_OR: {
            result = left_eval | right_eval;
        } break;
        case AST_BINARY_XOR: {
            result = left_eval ^ right_eval;
        } break;
        case AST_BINARY_LSHIFT: {
            result = left_eval << right_eval;
        } break;
        case AST_BINARY_RSHIFT: {
            result = left_eval >> right_eval;
        } break;
        case AST_BINARY_LOGICAL_AND: {
            result = left_eval && right_eval;
        } break;
        case AST_BINARY_LOGICAL_OR: {
            result = left_eval || right_eval;
        } break;
        }
    } break;
    case AST_UNARY: {
        Ast_Unary *unary = (Ast_Unary *)expr;
        i64 subexpr_eval = pp_eval_expr(lexer, unary->expr);
        switch (unary->kind) {
        INVALID_DEFAULT_CASE;
        case AST_UNARY_MINUS: {
            result = -subexpr_eval;
        } break;
        case AST_UNARY_PLUS: {
            result = subexpr_eval;
        } break;
        case AST_UNARY_LOGICAL_NOT: {
            result = !subexpr_eval;
        } break;
        case AST_UNARY_NOT: {
            result = ~subexpr_eval;
        } break;
        }
    } break;
    case AST_COND: {
        Ast_Cond *cond = (Ast_Cond *)expr;
        i64 cond_eval = pp_eval_expr(lexer, cond->cond);
        if (cond_eval) {
            result = pp_eval_expr(lexer, cond->expr);
        } else {
            result = pp_eval_expr(lexer, cond->else_expr);
        }
    } break;
    case AST_NUMBER_LIT: {
        Ast_Number_Lit *lit = (Ast_Number_Lit *)expr;
        if (C_TYPE_IS_FLOAT(lit->type)) {
            result = (i64)lit->float_value;
        } else {
            assert(C_TYPE_IS_INT(lit->type));
            result = lit->int_value;
        }
    } break;
    case AST_CAST: {
        Ast_Cast *cast = (Ast_Cast *)expr;
        i64 expr_eval = pp_eval_expr(lexer, cast->expr);
        bool is_unsigned = C_TYPE_IS_UNSIGNED(cast->type->kind);
        switch (cast->type->size) {
        case 1: {
            result = ( is_unsigned ? (u8)expr_eval : (i8)expr_eval );
        } break;
        case 2: {
            result = ( is_unsigned ? (u16)expr_eval : (i16)expr_eval );
        } break;
        case 4: {
            result = ( is_unsigned ? (u32)expr_eval : (i32)expr_eval );
        } break;
        default: {
            result = expr_eval;
        } break;
        }
    } break;
    }

    return result;
}

#endif

static void 
add_arg_expansion_buffer(Lexer *lexer, const char *def) {
    u32 arg_expansion_len = zlen(def);
    Lexer_Buffer *newbuf = get_new_buffer_stack_entry(lexer);
    newbuf->kind = LEXER_BUFFER_MACRO_ARG;
    newbuf->buf = newbuf->at = def;
    newbuf->size = arg_expansion_len;
    add_buffer_to_stack(lexer, newbuf);
}

Token *
peek_tok(Lexer *lexer) {
    return peek_tok_forward(lexer, 1);
}

void 
eat_tok(Lexer *lexer) {
    pop_token_from_stack(lexer);
}

Token *
peek_tok_forward(Lexer *lexer, u32 forward) {
    Token *token = 0;
    assert(forward);
    while (lexer->token_stack_size < forward) {
        // peek_tok_internal(lexer);
    }   
    Token_Stack_Entry *stack = lexer->token_stack; 
    while (--forward) {
        stack = stack->next;
    }
    token = stack->token;
    return token;
}

Lexer *
create_lexer(struct Compiler_Ctx *ctx, const char *filename) {
    Lexer *lexer = arena_bootstrap(Lexer, arena);
    lexer->ctx = ctx;
    lexer->macro_hash.num_buckets = MAX_PREPROCESSOR_MACROS;
    lexer->macro_hash.keys = arena_alloc_arr(lexer->arena, MAX_PREPROCESSOR_MACROS, u64);
    lexer->macro_hash.values = arena_alloc_arr(lexer->arena, MAX_PREPROCESSOR_MACROS, u64);
    lexer->scratch_buf_capacity = LEX_SCRATCH_BUF_SIZE;
    lexer->scratch_buf = arena_alloc(lexer->arena, lexer->scratch_buf_capacity);
    add_buffer_to_stack_file(lexer, filename);
    
    return lexer;
}

void 
lexer_gen_pp_token(Lexer *lexer) {
    bool skip_to_next_matching_if = false;
start:
    if (!peek_codepoint(lexer)) {
        lexer->expected_token_kind = TOKEN_EOS;
        goto token_generated;
    }
    
    bool is_space_advanced = false;
    while (is_space(peek_codepoint(lexer))) {
        advance(lexer, 1);
        is_space_advanced = true;
    }
    
    if (is_space_advanced) {
        goto start;
    }
    
    if (parse(lexer, "/*")) {
        while (!parse(lexer, "*/")) {
            advance(lexer, 1);
        }
        goto start;
    }
    
    if (parse(lexer, "//")) {
        while (peek_codepoint(lexer) && peek_codepoint(lexer) != '\n') {
            advance(lexer, 1);
        }
        goto start;
    }
    
    if (peek_codepoint(lexer) == '#' && 
        !lexer->is_in_preprocessor_ctx
        && !lexer->line_had_tokens) {
        // Copy whole preprocessing line in the buffer with respect to
        // rules of \\n, while also deleting comments 
        lexer->scratch_buf_size = 0;
        for (;;) {
            if (parse(lexer, "\\n")) {
                assert(lexer->scratch_buf_size < lexer->scratch_buf_capacity);
                lexer->scratch_buf[lexer->scratch_buf_size++] = ' ';
                continue;
                // \
                adssd
            } else if (parse(lexer, "//")) {
                for (;;) {
                    u8 codepoint = peek_codepoint(lexer);
                    if (!codepoint && codepoint != '\\' && codepoint != '\n') {
                        break;
                    }
                    advance(lexer, 1);
                }
                continue;
            } else if (parse(lexer, "/*")) {
                bool should_end_line = false;
                for (;;) {
                    if (!get_current_buf(lexer) || parse(lexer, "*/")) {
                        break;
                    }
                    if (peek_codepoint(lexer) == '\n') {
                        should_end_line = true;
                    }
                    advance(lexer, 1);
                }
                
                assert(lexer->scratch_buf_size < lexer->scratch_buf_capacity);
                lexer->scratch_buf[lexer->scratch_buf_size++] = ' ';
                if (should_end_line) {
                    break;
                }
            }
            u8 codepoint = peek_codepoint(lexer);
            if (codepoint == '\n' || !codepoint) {
                advance(lexer, 1);
                break;
            } else {
                assert(lexer->scratch_buf_size < lexer->scratch_buf_capacity);
                lexer->scratch_buf[lexer->scratch_buf_size++] = codepoint;
                advance(lexer, 1);
            }
        }
        assert(lexer->scratch_buf_size < lexer->scratch_buf_capacity);
        lexer->scratch_buf[lexer->scratch_buf_size] = 0;
        
        // @NOTE(hl): Because we define scratch buffer to be size greater than maximum line length
        //  we can use it for 2 buffers: text and string 
        Text_Lexer pp_lexer = text_lexer(lexer->scratch_buf, lexer->scratch_buf_size,
            lexer->scratch_buf + lexer->scratch_buf_size + 1, lexer->scratch_buf_capacity - lexer->scratch_buf_size - 1);
        //
        // Parse the preprocessor directive
        //
        text_lexer_token(&pp_lexer);
        if (pp_lexer.token_kind == '#') {
            text_lexer_next(&pp_lexer);
            if (pp_lexer.token_kind == TEXT_LEXER_TOKEN_IDENT) {
                u32 keyword_iter = 0;
                u32 keyword_enum = 0;
                ITER_PREPROCESSOR_KEYWORD(keyword_iter) {
                    if (zeq(pp_lexer.string_buf, PREPROCESSOR_KEYWORD_STRINGS[keyword_iter])) {
                        keyword_enum = keyword_iter;
                        break;
                    }
                }
                text_lexer_next(&pp_lexer);
                
                switch (keyword_enum) {
                default: {
                    NOT_IMPLEMENTED;
                } break;
                case PP_KEYWORD_DEFINE: {
                    if (pp_lexer.token_kind == TEXT_LEXER_TOKEN_IDENT) {
                        PP_Macro *macro = pp_define(lexer, pp_lexer.string_buf);
                        // @TODO(hl): Warn if macro is already defined
                        text_lexer_next(&pp_lexer);
                        if (pp_lexer.token_kind == '(') {
                            macro->is_function_like = true;
                            
                            text_lexer_next(&pp_lexer);
                            while (pp_lexer.token_kind && pp_lexer.token_kind != ')') {
                                if (pp_lexer.token_kind == TEXT_LEXER_TOKEN_IDENT) {
                                    macro->arg_names[macro->arg_count++] = mem_alloc_str(pp_lexer.string_buf);
                                    text_lexer_next(&pp_lexer);
                                } else if (pp_lexer.token_kind == ',') {
                                    text_lexer_next(&pp_lexer);
                                } else if (pp_lexer.token_kind == '.') {
                                    if (macro->has_varargs) {
                                        NOT_IMPLEMENTED;
                                    }
                                    
                                    bool is_correct = false;
                                    text_lexer_next(&pp_lexer);
                                    if (pp_lexer.token_kind == '.') {
                                        text_lexer_next(&pp_lexer);
                                        if (pp_lexer.token_kind == '.') {
                                            text_lexer_next(&pp_lexer);
                                            macro->has_varargs = true;
                                            macro->varargs_idx = macro->arg_count++;
                                            is_correct = true;
                                        }
                                    } 
                                    
                                    if (!is_correct) {
                                        NOT_IMPLEMENTED;
                                    }
                                } else {
                                    NOT_IMPLEMENTED;
                                    break;
                                }
                            }
                            
                            if (pp_lexer.token_kind == ')') {
                                const char *definition_start = (const char *)pp_lexer.buf_at;
                                const char *definition_end = (const char *)pp_lexer.buf_eof;
                                u32 len = fmt(macro->definition, sizeof(macro->definition),
                                    "%.*s  ", (int)(definition_end - definition_start), definition_start);
                                macro->definition_len = len;
                            } else {
                                NOT_IMPLEMENTED;
                            }
                        }
                    } else {
                        NOT_IMPLEMENTED;
                    }
                } break;
                case PP_KEYWORD_UNDEF: {
                    if (pp_lexer.token_kind == TEXT_LEXER_TOKEN_IDENT) {
                        pp_undef(lexer, pp_lexer.string_buf);
                    } else {
                        NOT_IMPLEMENTED;
                    }
                } break;
                case PP_KEYWORD_INCLUDE: {
                    if (pp_lexer.token_kind == '<') {
                        // @HACK
                        text_lexer_next(&pp_lexer);
                        const char *path_start = (const char *)pp_lexer.buf_at;
                        while (pp_lexer.token_kind != '>' && pp_lexer.token_kind != TOKEN_EOS) {
                            text_lexer_next(&pp_lexer);
                        }
                        
                        if (pp_lexer.token_kind == '>') {
                            const char *path_end = (const char *)pp_lexer.buf_at;
                            
                            char filename_buffer[4096];
                            fmt(filename_buffer, sizeof(filename_buffer), "%.*s", 
                                (int)(path_end - path_start), path_start);
                            add_buffer_to_stack_file(lexer, filename_buffer);
                        }
                    } else if (pp_lexer.token_kind == TEXT_LEXER_TOKEN_STR) {
                        add_buffer_to_stack_file(lexer, pp_lexer.string_buf);
                    } else {
                        NOT_IMPLEMENTED;
                    }
                } break;
                case PP_KEYWORD_IF: {
                    NOT_IMPLEMENTED;
                } break;
                case PP_KEYWORD_IFDEF: {
                    if (pp_lexer.token_kind == TEXT_LEXER_TOKEN_IDENT) {
                        PP_Macro *macro = pp_get(lexer, pp_lexer.string_buf);
                        bool is_true = macro != 0;
                        if (is_true) {
                            pp_push_nested_if(lexer, is_true);
                        } else {
                            skip_to_next_matching_if = true;
                        }
                    } else {
                        NOT_IMPLEMENTED;
                    }
                } break;
                case PP_KEYWORD_IFNDEF: {
                    if (pp_lexer.token_kind == TEXT_LEXER_TOKEN_IDENT) {
                        PP_Macro *macro = pp_get(lexer, pp_lexer.string_buf);
                        bool is_true = macro == 0;
                        if (is_true) {
                            pp_push_nested_if(lexer, is_true);
                        } else {
                            skip_to_next_matching_if = true;
                        }
                    } else {
                        NOT_IMPLEMENTED;
                    }
                } break;
                case PP_KEYWORD_ELSE: {
                    NOT_IMPLEMENTED;
                } break;
                case PP_KEYWORD_ELIFDEF: {
                    NOT_IMPLEMENTED;
                } break;
                case PP_KEYWORD_ELIFNDEF: {
                    NOT_IMPLEMENTED;
                } break;
                case PP_KEYWORD_PRAGMA: {
                    NOT_IMPLEMENTED;
                } break;
                case PP_KEYWORD_ERROR: {
                    NOT_IMPLEMENTED;
                } break;
                case PP_KEYWORD_DEFINED: {
                    NOT_IMPLEMENTED;
                } break;
                case PP_KEYWORD_LINE: {
                    NOT_IMPLEMENTED;
                } break;
                case PP_KEYWORD_ELIF: {
                    NOT_IMPLEMENTED;
                } break;
                case PP_KEYWORD_ENDIF: {
                    pp_pop_nested_if(lexer);
                } break;
                }
            } else {
                NOT_IMPLEMENTED;
            }
            
            
        } else {
            NOT_IMPLEMENTED;
        }
        
        lexer->scratch_buf_size = 0;
        goto start;
    }
    
    // Character literal
    {
        u32 char_lit_kind = 0;
        if (parse(lexer, "u8'")) {
            char_lit_kind = CHAR_LIT_UTF8;
        } else if (parse(lexer, "u'")) {
            char_lit_kind = CHAR_LIT_U16;
        } else if (parse(lexer, "U'")) {\
            char_lit_kind = CHAR_LIT_U32;
        } else if (parse(lexer, "L'")) {
            char_lit_kind = CHAR_LIT_WIDE;
        } else if (parse(lexer, "'")) {
            char_lit_kind = CHAR_LIT_REG;
        }
        
        if (char_lit_kind) {
            while (peek_codepoint(lexer) && peek_codepoint(lexer) != '\'') {
                assert(lexer->scratch_buf_size < lexer->scratch_buf_capacity);
                lexer->scratch_buf[lexer->scratch_buf_size++] = peek_codepoint(lexer);
                advance(lexer, 1);
            }
            
            if (peek_codepoint(lexer) == '\'') {
                assert(lexer->scratch_buf_size < lexer->scratch_buf_capacity);
                lexer->scratch_buf[lexer->scratch_buf_size++] = '\'';
            } else {
                NOT_IMPLEMENTED;
            }
            
            lexer->expected_token_kind = TOKEN_PP_CHAR_LIT;
            goto token_generated;
        }
    }
    
    // String literal
    {
        u32 string_lit_kind = 0;
        if (parse(lexer, "u8'")) {
            string_lit_kind = STRING_LIT_UTF8;
        } else if (parse(lexer, "u'")) {
            string_lit_kind = STRING_LIT_U16;
        } else if (parse(lexer, "U'")) {\
            string_lit_kind = STRING_LIT_U32;
        } else if (parse(lexer, "L'")) {
            string_lit_kind = STRING_LIT_WIDE;
        } else if (parse(lexer, "'")) {
            string_lit_kind = STRING_LIT_REG;
        }
        
        if (string_lit_kind) {
            while (peek_codepoint(lexer) && peek_codepoint(lexer) != '\"') {
                if (peek_codepoint(lexer) == '\n') {
                    NOT_IMPLEMENTED;
                }
                
                assert(lexer->scratch_buf_size < lexer->scratch_buf_capacity);
                lexer->scratch_buf[lexer->scratch_buf_size++] = peek_codepoint(lexer);
                advance(lexer, 1);
            }
            
            if (peek_codepoint(lexer) == '\"') {
                assert(lexer->scratch_buf_size < lexer->scratch_buf_capacity);
                lexer->scratch_buf[lexer->scratch_buf_size++] = '\"';
            } else {
                NOT_IMPLEMENTED;
            }
            
            lexer->expected_token_kind = TOKEN_PP_STRING_LIT;
            goto token_generated;
        }
    }
    
    if (is_digit(peek_codepoint(lexer))) {
        lexer->expected_token_kind = TOKEN_PP_NUMBER_LIT;
        for (;;) {
            u8 codepoint = peek_codepoint(lexer);
            if (!codepoint || !is_digit(codepoint)) {
                break;
            }
            assert(lexer->scratch_buf_size < lexer->scratch_buf_capacity);
            lexer->scratch_buf[lexer->scratch_buf_size++] = codepoint;
            advance(lexer, 1);
        }
        goto token_generated;
    }
    
    if (is_ident_start(peek_codepoint(lexer))) {
        for (;;) {
            u8 codepoint = peek_codepoint(lexer);
            if (!is_ident(codepoint)) {
                break;
            }
            assert(lexer->scratch_buf_size < lexer->scratch_buf_capacity);
            lexer->scratch_buf[lexer->scratch_buf_size++] = codepoint;
            advance(lexer, 1);
        }
        lexer->expected_token_kind = TOKEN_IDENT;
        goto token_generated;
    }
    
    if (is_punct(peek_codepoint(lexer))) {
        // Maximal munch
        u32 punctuator_iter = 0;
        u32 punctuator_enum = 0;
        ITER_PUNCTUATORS(punctuator_iter) {
            const char *test = PUNCTUATOR_STRINGS[punctuator_iter - 0x100];
            if (next_eq(lexer, test)) {
                punctuator_enum = punctuator_iter;
                mem_copy(lexer->scratch_buf + lexer->scratch_buf_size, test, zlen(test));
                lexer->scratch_buf_size += zlen(test);
                break;
            }
        }
        
        lexer->expected_token_kind = TOKEN_PUNCTUATOR;
        if (punctuator_enum) {
            lexer->expected_punct = punctuator_enum;
        } else {
            u8 codepoint = peek_codepoint(lexer);
            assert(lexer->scratch_buf_size < lexer->scratch_buf_capacity);
            lexer->scratch_buf[lexer->scratch_buf_size++] = codepoint;
            advance(lexer, 1);
            lexer->expected_punct = codepoint;
        }
        goto token_generated;
    }
token_generated:    
    return;
}

Token *
get_new_token(Lexer *lexer) {
    Token *token = arena_alloc_struct(lexer->arena, Token);
    return token;
}

Token *
lexer_peek_token_new(Lexer *lexer) {
    u32 expected = 0;
    Token *token = 0;
    bool is_in_preprocessor_ctx = false;
    u32 last_size = 0;
    const char *lexbuf_cursor = lexer->scratch_buf + last_size;
    bool next_is_concat = false;
start:
    lexer->scratch_buf_size = last_size;
    lexer_gen_pp_token(lexer);
    lexer->scratch_buf[lexer->scratch_buf_size] = 0;
    expected = lexer->expected_token_kind;
    is_in_preprocessor_ctx = lexer->is_in_preprocessor_ctx;
    lexbuf_cursor = lexer->scratch_buf + last_size;
    
    if (expected == TOKEN_IDENT) {
        // First, check if we are inside function-like macro expansion
        // If so, check if current identifier is macro argument
        if (is_in_preprocessor_ctx) {
            bool is_macro_argument = false;
            for (Lexer_Buffer *lexbuf = get_current_buf(lexer);
                 lexbuf && !is_macro_argument;
                 lexbuf = lexbuf->next) {
                if (lexbuf->kind == LEXER_BUFFER_MACRO) {
                    PP_Macro *macro = lexbuf->macro;
                    if (zstartswith(lexbuf_cursor, "__VA_ARGS__")) {
                        if (macro->has_varargs) {
                            lexer->scratch_buf_size = last_size;
                            add_arg_expansion_buffer(lexer, lexbuf->macro_args[macro->varargs_idx]);
                            is_macro_argument = true;
                            break;
                        } else {
                            NOT_IMPLEMENTED;
                        }
                    }
                    
                    for (u32 macro_arg_name_idx = 0;
                        macro_arg_name_idx < macro->arg_count - macro->has_varargs;
                        ++macro_arg_name_idx) {
                        const char *test = macro->arg_names[macro_arg_name_idx];
                        if (zstartswith(lexbuf_cursor, test)) {
                            const char *arg_expansion = lexbuf->macro_args[macro_arg_name_idx];
                            add_arg_expansion_buffer(lexer, arg_expansion);
                            is_macro_argument = true;
                            lexer->scratch_buf_size = last_size;
                            break;
                        }
                    }
                }
            }
            // If we expanded from macor argument, call for token getting again
            if (is_macro_argument) {
                goto start;    
            }
        }
        // Try to get macro of same name
        {
            PP_Macro *macro = pp_get(lexer, lexbuf_cursor);
            if (macro) {
                // @NOTE(hl): CLEANUP buffer is get here ant initialized inline because 
                //  we need to write arguments to it. This is better be wrapped in a function to be consistent
                Lexer_Buffer *buffer = get_new_buffer_stack_entry(lexer);
                buffer->buf = buffer->at = macro->definition;
                buffer->size = macro->definition_len;
                buffer->kind = LEXER_BUFFER_MACRO;
                buffer->macro = macro;
                if (macro->is_function_like) {
                    // @NOTE(hl): Advance actual text cursor to eat arguments of function-like macro 
                    u8 codepoint = peek_codepoint(lexer);
                    if (codepoint == '(') {
                        advance(lexer, 1);
                        codepoint = peek_codepoint(lexer);
                        
                        char temp_buffer[4096];
                        u32  temp_buffer_size = 0;
                        while (codepoint != ')') {
                            // If current macro is positional, use , as separator, and if we parse variadic 
                            // arguments, just put this arguments into one
                            if ( codepoint == ',' && 
                                (!macro->has_varargs || buffer->macro_arg_count < macro->varargs_idx) ) {
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
                        report_error(lexer->ctx->er, get_current_loc(lexer), "'(' Expected in function-like macro");
                    } 
                }
                add_buffer_to_stack(lexer, buffer);
                lexer->scratch_buf_size = last_size;
                goto start;
            }
        }   
    }
    
    bool stringify = false;
    if (is_in_preprocessor_ctx) {
        if (expected == TOKEN_PUNCTUATOR && lexer->expected_punct == '#') {
            lexer->scratch_buf_size = 0;
            lexer_gen_pp_token(lexer);
            expected = lexer->expected_token_kind;
            stringify = true;

            goto generate_token;
        }
    }
    
    if (next_is_concat) {
        add_buffer_to_stack_concat(lexer);
        last_size = 0;
        next_is_concat = false;
        goto start;
    }
    
    if (is_in_preprocessor_ctx) {
        // @NOTE(hl): Advance while not terminating the buffer
        Lexer_Buffer *buffer = get_current_buf(lexer);
        while (is_space(lexbuf_peek(buffer)) && lexbuf_peek(buffer)) {
            lexbuf_advance(buffer, 1);
        }
        // _a expands to 1 + 2. 
        // Then it sees ##
        // Jump over ##
        // Save cursor index of scratch
        // Parse next token
        // If it is macro expansion, 
        //  Undo writing it to scratch (reverse to index of start)
        //  Expand macro 
        //  _b expands to 2 + 3
        // Now the correct new buffer is formed
        if (parse_no_scratch(lexer, "##")) {
            last_size = lexer->scratch_buf_size;
            // lexer_gen_pp_token(lexer);
            u32 new_expected = lexer->expected_token_kind;
            // add_buffer_to_stack_concat(lexer);
            
            expected = new_expected;
            next_is_concat = true;
            goto start;
        }
    }
    
check_string:
    if (expected == TOKEN_PP_STRING_LIT) {
        while (is_space(peek_codepoint(lexer))) {
            advance(lexer, 1);
        }
        
        if (peek_codepoint(lexer) == '\"') {
            lexer_gen_pp_token(lexer);
            goto check_string;
        }
    }
    
generate_token:

    if (stringify) {
        NOT_IMPLEMENTED;
    } else if (expected == TOKEN_IDENT) {
        // Try to find keyword 
        {
            u32 keyword_iter = 0;
            u32 found_keyword = 0;
            ITER_KEYWORDS(keyword_iter) {
                const char *test = KEYWORD_STRINGS[keyword_iter];
                if (zstartswith(lexer->scratch_buf, test)) {
                    found_keyword = keyword_iter;
                    break;
                }
            }
            
            if (found_keyword) {
                token = get_new_token(lexer);
                token->kind = TOKEN_KEYWORD;
                token->kw = found_keyword;
            }
        }       
        
        // Otherwise, this is regular identifier
        if (!token) {
            token = get_new_token(lexer);
            token->kind = TOKEN_IDENT;
            token->ident.str = mem_alloc_str(lexer->scratch_buf);
        }
    } else if (expected == TOKEN_PUNCTUATOR) {
        token = get_new_token(lexer);
        token->kind = TOKEN_PUNCTUATOR;
        token->punct = lexer->expected_punct;
    } else if (expected == TOKEN_PP_CHAR_LIT) {
        token = get_new_token(lexer);
        const char *cursor = lexer->scratch_buf;
        
        u32 lit_kind = 0;
        if (zstartswith(cursor, "'")) {
            ++cursor;
            lit_kind = CHAR_LIT_REG;
        } else if (zstartswith(cursor, "u8'")) {
            cursor += zlen("u8'");
            lit_kind = CHAR_LIT_UTF8;
        } else if (zstartswith(cursor, "u'")) {
            cursor += zlen("u'");
            lit_kind = CHAR_LIT_U16;
        } else if (zstartswith(cursor, "U'")) {
            cursor += zlen("U'");
            lit_kind = CHAR_LIT_U32;
        } else if (zstartswith(cursor, "L'")) {
            cursor += zlen("L'");
            lit_kind = CHAR_LIT_WIDE;
        }
        
        i64 value = 0;
        switch (lit_kind) {
        case CHAR_LIT_REG: {
            if (zstartswith(cursor, "\\\'")) {
                cursor += 2;
                value = '\'';
            } else if (zstartswith(cursor, "\\\"")) {
                cursor += 2;
                value = '\"';
            } else if (zstartswith(cursor, "\\\?")) {
                cursor += 2;
                value = '\?';
            } else if (zstartswith(cursor, "\\\\")) {
                cursor += 2;
                value = '\\';
            } else if (zstartswith(cursor, "\\\a")) {
                cursor += 2;
                value = '\a';
            } else if (zstartswith(cursor, "\\\b")) {
                cursor += 2;
                value = '\b';
            } else if (zstartswith(cursor, "\\\f")) {
                cursor += 2;
                value = '\f';
            } else if (zstartswith(cursor, "\\\n")) {
                cursor += 2;
                value = '\n';
            } else if (zstartswith(cursor, "\\\r")) {
                cursor += 2;
                value = '\r';
            } else if (zstartswith(cursor, "\\\t")) {
                cursor += 2;
                value = '\t';
            } else if (zstartswith(cursor, "\\\v")) {
                cursor += 2;
                value = '\v';
            } else {
                value = *cursor++;
            }
        } break;
        case CHAR_LIT_UTF8: {
            NOT_IMPLEMENTED;
        } break;
        case CHAR_LIT_U16: {
            NOT_IMPLEMENTED;
        } break;
        case CHAR_LIT_U32: {
            NOT_IMPLEMENTED;
        } break;
        case CHAR_LIT_WIDE: {
            NOT_IMPLEMENTED;
        } break;
        }
        
        if (*cursor == '\'') {
            ++cursor;
            assert(*cursor == 0);
            
            token = get_new_token(lexer);
            token->kind = TOKEN_NUMBER;
            token->number.type = C_TYPE_SINT;
            token->number.sint_value = value;
        } else {
            NOT_IMPLEMENTED;
        }
    } else if (expected == TOKEN_PP_STRING_LIT) {
        NOT_IMPLEMENTED;
    } else if (expected == TOKEN_PP_NUMBER_LIT) {
        token = get_new_token(lexer);
        token->kind = TOKEN_NUMBER;
        token->number.type = C_TYPE_SLINT;
        token->number.sint_value = z2i64(lexer->scratch_buf);
    } else if (expected == TOKEN_EOS) {
        token = get_new_token(lexer);
        token->kind = TOKEN_EOS;
    } else {
        NOT_IMPLEMENTED;
    }
    
    assert(token);
    return token;
}


void 
preprocess(Lexer *lexer, Out_Stream *stream) {
    for (;;) {
        Token *token = lexer_peek_token_new(lexer);
        if (token->kind == TOKEN_EOS) {
            break;
        }
        fmt_token(STDOUT, token);
        out_streamf(STDOUT, "\n");
        out_stream_flush(STDOUT);
    }
}