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

#define LEX_STR_HASH_FUNC    djb2
#define LEX_SCRATCH_BUF_SIZE KB(16)

#define ITER_KEYWORDS(_it) \
for ((_it) = KEYWORD_AUTO; (_it) < KEYWORD_SENTINEL; ++(_it))
#define ITER_PREPROCESSOR_KEYWORDS(_it) \
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

static Str KEYWORD_STRINGS[] = {
    WRAP_Z("(unknown)"),
    WRAP_Z("auto"),
    WRAP_Z("break"),
    WRAP_Z("case"),
    WRAP_Z("char"),
    WRAP_Z("const"),
    WRAP_Z("continue"),
    WRAP_Z("default"),
    WRAP_Z("do"),
    WRAP_Z("double"),
    WRAP_Z("else"),
    WRAP_Z("enum"),
    WRAP_Z("extern"),
    WRAP_Z("float"),
    WRAP_Z("for"),
    WRAP_Z("goto"),
    WRAP_Z("if"),
    WRAP_Z("inline"),
    WRAP_Z("int"),
    WRAP_Z("long"),
    WRAP_Z("register"),
    WRAP_Z("restrict"),
    WRAP_Z("return"),
    WRAP_Z("short"),
    WRAP_Z("signed"),
    WRAP_Z("unsigned"),
    WRAP_Z("static"),
    WRAP_Z("struct"),
    WRAP_Z("switch"),
    WRAP_Z("typedef"),
    WRAP_Z("union"),
    WRAP_Z("unsigned"),
    WRAP_Z("void"),
    WRAP_Z("volatile"),
    WRAP_Z("while"),
    WRAP_Z("_Alignas"),
    WRAP_Z("_Alignof"),
    WRAP_Z("_Atomic"),
    WRAP_Z("_Bool"),
    WRAP_Z("_Complex"),
    WRAP_Z("_Decimal128"),
    WRAP_Z("_Decimal32"),
    WRAP_Z("_Decimal64"),
    WRAP_Z("_Generic"),
    WRAP_Z("_Imaginary"),
    WRAP_Z("_Noreturn"),
    WRAP_Z("_Static_assert"),
    WRAP_Z("_Thread_local"),
    WRAP_Z("_Pragma"),
};

static Str PREPROCESSOR_KEYWORD_STRINGS[] = {
    WRAP_Z("(unknown)"),
    WRAP_Z("define"),
    WRAP_Z("undef"),
    WRAP_Z("include"),
    WRAP_Z("if"),
    WRAP_Z("ifdef"),
    WRAP_Z("ifndef"),
    WRAP_Z("else"),
    WRAP_Z("elifdef"),
    WRAP_Z("elifndef"),
    WRAP_Z("pragma"),
    WRAP_Z("error"),
    WRAP_Z("defined"),
    WRAP_Z("line"),
    WRAP_Z("elif"),
    WRAP_Z("endif"),
};

static Str PUNCTUATOR_STRINGS[] = {
    WRAP_Z("(unknown)"),
    WRAP_Z(">>="),
    WRAP_Z("<<="), 
    WRAP_Z("..."),
    WRAP_Z("+="),
    WRAP_Z("-="),
    WRAP_Z("*="),
    WRAP_Z("/="),
    WRAP_Z("%="), 
    WRAP_Z("&="),
    WRAP_Z("|="),
    WRAP_Z("^="),
    WRAP_Z("++"),
    WRAP_Z("--"),
    WRAP_Z(">>"),
    WRAP_Z("<<"),
    WRAP_Z("&&"),
    WRAP_Z("||"),
    WRAP_Z("=="),
    WRAP_Z("!="),
    WRAP_Z("<="),
    WRAP_Z(">="), 
    WRAP_Z("->"),
    WRAP_Z("##"),
};

static u32 
get_keyword_from_str(Str str) {
    u32 keyword_iter = 0;
    u32 found_keyword = 0;
    ITER_KEYWORDS(keyword_iter) {
        Str test = KEYWORD_STRINGS[keyword_iter];
        if (str_eq(test, str)) {
            found_keyword = keyword_iter;
            break;
        }
    }
    return found_keyword;
}

static u32 
get_pp_keyword_from_str(Str str) {
    u32 keyword_iter = 0;
    u32 found_keyword = 0;
    ITER_PREPROCESSOR_KEYWORDS(keyword_iter) {
        Str test = PREPROCESSOR_KEYWORD_STRINGS[keyword_iter];
        if (str_eq(test, str)) {
            found_keyword = keyword_iter;
            break;
        }
    }
    return found_keyword;
}

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
        result = out_streamf(stream, "<ident>%s", token->str.data);
    } break;
    case TOKEN_KEYWORD: {
        result = out_streamf(stream, "<kw>%s", KEYWORD_STRINGS[token->kw].data);
    } break;
    case TOKEN_STRING: {
        result = out_streamf(stream, "<str>%s", token->str.data);
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
                PUNCTUATOR_STRINGS[token->punct - 0x100].data);
        }
    } break;
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
    u64 result = 0;
    if (base == 2 || base == 8 || base == 10) {
        result = symb & 0xF;
    } else if (base == 16) {
        if ('0' <= symb && symb <= '9') {
            result = symb & 0xF;;
        } else {
            result = (symb & 0x1F) - 1;
        }
    } else {
        NOT_IMPLEMENTED;
    }
    return result;
}

typedef struct {
    const char *cusror;
    u32 value;
} Escape_Sequence_Get_Result;

// Escape sequences are defined by backslash and some characters following it
// C standard specifies that any escape sequnce is valid, but it does not specify what to 
// do if resulting storage has to space to fit it (for example, character constant)
static Escape_Sequence_Get_Result
get_escape_sequency_value(const char *cursor) {    
    u32 result = 0;
    assert(*cursor == '\\');
    ++cursor;
    
    switch (*cursor) {
    case '0':
    case '1':
    case '2': 
    case '3':
    case '4':
    case '5':
    case '6':
    case '7': {
        // If not handled in other cases, this is octal constant with 3 (or less) digits
        for (u32 i = 0; i < 3; ++i) {
            char symb = *cursor;
            if (!is_symb_base(symb, 8)) {
                assert(i != 0);
                break;
            }
            result = (result << 3) | symb_to_base(symb, 8);
            ++cursor;
        }
    } break;
    case 'U': {
        // Unicode value with 8 digits
        ++cursor;
        for (u32 i = 0; i < 8; ++i) {
            char symb = *cursor++;
            assert(is_symb_base(symb, 16));
            result = (result << 4) | symb_to_base(symb, 16); 
        }
    } break;
    case 'u': {
        // Unicode value with 4 digits
        ++cursor;
        for (u32 i = 0; i < 4; ++i) {
            char symb = *cursor++;
            assert(is_symb_base(symb, 16));
            result = (result << 4) | symb_to_base(symb, 16);
        }
    } break;
    case 'x': {
        // Hex value with arbitrary width
        ++cursor;
        for (u32 i = 0; ; ++i) {
            char symb = *cursor;
            if (!is_symb_base(symb, 16)) {
                assert(i != 0);
                break;
            }
            result = (result << 4) | symb_to_base(symb, 16);
            ++cursor;
        }
    } break;
    case '\'': {
        cursor += 2;
        result = '\'';
    } break;
    case '\"': {
        cursor += 2;
        result = '\"';
    } break;
    case '?': {
        cursor += 2;
        result = '\?';
    } break;
    case '\\': {
        cursor += 2;
        result = '\\';
    } break;
    case 'a': {
        cursor += 2;
        result = '\a';
    } break;
    case 'b': {
        cursor += 2;
        result = '\b';
    } break;
    case 'f': {
        cursor += 2;
        result = '\f';
    } break;
    case 'n': {
        cursor += 2;
        result = '\n';
    } break;
    case 'r': {
        cursor += 2;
        result = '\r';
    } break;
    case 't': {
        cursor += 2;
        result = '\t';
    } break;
    case 'v': {
        cursor += 2;
        result = '\v';
    } break;
    }
    
    Escape_Sequence_Get_Result get_result = {0};
    get_result.value = result;
    get_result.cusror = cursor;
    return get_result;
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
lexbuf_next_eq(Lexer_Buffer *buffer, Str str) {
    bool result = false;
    if (buffer->at + str.len < buffer->buf + buffer->size) {
        result = mem_eq(buffer->at, str.data, str.len);
    }
    return result;
}

bool 
lexbuf_parse(Lexer_Buffer *buffer, Str str) {
    bool result = lexbuf_next_eq(buffer, str);
    if (result) {
        lexbuf_advance(buffer, str.len);
    }
    return result;
}

#if 0 


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
    assert(lexer->buffer_stack_size < 1024);
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
    return 0;
    // Temp_Memory temp = begin_temp_memory(lexer->arena);
    // // Create temporary array for storing buffers in stack-like structure
    // u32 count = lexer->buffer_stack_size;
    // Lexer_Buffer **buffers = arena_alloc_arr(temp.arena, count, Lexer_Buffer *);
    // {
    //     u32 cursor = count;
    //     // Populare array in reverse order
    //     for (Lexer_Buffer *buffer = lexer->buffer_stack;
    //         buffer;
    //         buffer = buffer->next) {
    //         assert(cursor);
    //         --cursor;
    //         buffers[cursor] = buffer;
    //     }
    //     // Now start generating src locations going from parent to child
    //     assert(cursor == 0);
    // }
    
    // const Src_Loc *current_loc = 0;
    // for (u32 cursor = 0;
    //      cursor < count;
    //      ++cursor) {
    //     Lexer_Buffer *buffer = buffers[cursor];
    //     switch (buffer->kind) {
    //     INVALID_DEFAULT_CASE;
    //     case LEXER_BUFFER_CONCAT: {
    //         current_loc = get_src_loc_macro_arg(lexer->ctx->fr, buffer->symb, current_loc);
    //     } break;
    //     case LEXER_BUFFER_FILE: {
    //         current_loc = get_src_loc_file(lexer->ctx->fr, buffer->file_id, buffer->line, 
    //             buffer->symb, current_loc);
    //     } break;
    //     case LEXER_BUFFER_MACRO: {
    //         current_loc = get_src_loc_macro(lexer->ctx->fr, buffer->macro->loc, buffer->symb, 
    //             current_loc);
    //     } break;
    //     case LEXER_BUFFER_MACRO_ARG: {
    //         current_loc = get_src_loc_macro_arg(lexer->ctx->fr, buffer->symb, current_loc);
    //     } break;
    //     }
    // }
    // end_temp_memory(temp);
    // const Src_Loc *loc = current_loc;
    // return loc;
}

PP_Macro *
pp_get_macro_internal(Lexer *lexer, u32 hash,
    bool or_zero) {
    PP_Macro *result = 0;
    CT_ASSERT(IS_POW2(MAX_PREPROCESSOR_MACROS));
    for (u32 offset = 0; offset < MAX_PREPROCESSOR_MACROS; ++offset) {
        u32 hash_value = (hash + offset) & (MAX_PREPROCESSOR_MACROS - 1);    
        PP_Macro *hash_entry = lexer->macro_hash + hash_value;
        if (hash_entry && (hash_entry->hash == hash || 
            (!hash_entry->hash && or_zero))) {
            result = hash_entry;
            break;
        }
    }
    return result;
}

PP_Macro *
pp_get(Lexer *lexer, Str name) {
    PP_Macro *result = 0;
    u32 name_hash = LEX_STR_HASH_FUNC(name.data, name.len);
    return pp_get_macro_internal(lexer, name_hash, false);
}

PP_Macro * 
pp_define(Lexer *lexer, Str name) {
    u32 name_hash = LEX_STR_HASH_FUNC(name.data, name.len);
    PP_Macro *macro = pp_get_macro_internal(lexer, name_hash, 1);
    mem_zero(macro, sizeof(*macro));
    macro->name = name;
    macro->hash = name_hash;
    return macro;
}

void 
pp_undef(Lexer *lexer, Str name) {
    u32 name_hash = LEX_STR_HASH_FUNC(name.data, name.len);
    PP_Macro *macro = pp_get_macro_internal(lexer, name_hash, 0);
    if (macro) {
        macro->hash = 0;
    }
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
next_eq(Lexer *lexer, Str lit) {
    bool result = false;
    Lexer_Buffer *buffer = get_current_buf(lexer);
    if (buffer) {
        result = lexbuf_next_eq(buffer, lit);
    }
    return result;
}

bool 
parse(Lexer *lexer, Str lit) {
    bool result = false;
    Lexer_Buffer *buffer = get_current_buf(lexer);
    if (buffer) {
        result = lexbuf_parse(buffer, lit);
        if (result) {
            mem_copy(lexer->scratch_buf + lexer->scratch_buf_size, lit.data, lit.len);
            lexer->scratch_buf_size += lit.len;
        }
    }
    return result;
}

bool 
parse_no_scratch(Lexer *lexer, Str lit) {
    bool result = false;
    Lexer_Buffer *buffer = get_current_buf(lexer);
    while (buffer && !lexbuf_peek(buffer)) {
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

static void 
add_arg_expansion_buffer(Lexer *lexer, Str def) {
    Lexer_Buffer *newbuf = get_new_buffer_stack_entry(lexer);
    newbuf->kind = LEXER_BUFFER_MACRO_ARG;
    newbuf->buf = newbuf->at = def.data;
    newbuf->size = def.len;
    add_buffer_to_stack(lexer, newbuf);
}

Lexer *
create_lexer(struct Compiler_Ctx *ctx, const char *filename) {
    Lexer *lexer = arena_bootstrap(Lexer, arena);
    lexer->ctx = ctx;
    lexer->scratch_buf_capacity = LEX_SCRATCH_BUF_SIZE;
    lexer->scratch_buf = arena_alloc(lexer->arena, lexer->scratch_buf_capacity);
    add_buffer_to_stack_file(lexer, filename);
    
    return lexer;
}

void 
lexer_gen_pp_token(Lexer *lexer) {
    bool skip_to_next_matching_if = false;
    u32 if_skip_depth = 0;
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
    
    if (parse_no_scratch(lexer, WRAP_Z("/*"))) {
        while (!parse_no_scratch(lexer, WRAP_Z("*/"))) {
            advance(lexer, 1);
        }
        goto start;
    }
    
    if (parse_no_scratch(lexer, WRAP_Z("//"))) {
        while (peek_codepoint(lexer) && peek_codepoint(lexer) != '\n') {
            advance(lexer, 1);
        }
        goto start;
    }
    
    if (skip_to_next_matching_if) {
        for (;;) {
            u8 codepoint = peek_codepoint(lexer);
            if (codepoint == '#' || !codepoint) {
                break;
            }
            advance(lexer, 1);
        }
    }
    
    if (peek_codepoint(lexer) == '#' && 
        !lexer->is_in_preprocessor_ctx) {
        // Copy whole preprocessing line in the buffer with respect to
        // rules of \\n, while also deleting comments 
        lexer->scratch_buf_size = 0;
        for (;;) {
            if (parse_no_scratch(lexer, WRAP_Z("\\n"))) {
                assert(lexer->scratch_buf_size < lexer->scratch_buf_capacity);
                lexer->scratch_buf[lexer->scratch_buf_size++] = ' ';
                continue;
            } else if (parse_no_scratch(lexer, WRAP_Z("//"))) {
                for (;;) {
                    u8 codepoint = peek_codepoint(lexer);
                    if (!codepoint && codepoint != '\\' && codepoint != '\n') {
                        break;
                    }
                    advance(lexer, 1);
                }
                continue;
            } else if (parse_no_scratch(lexer, WRAP_Z("/*"))) {
                bool should_end_line = false;
                for (;;) {
                    if (!get_current_buf(lexer) || parse(lexer, WRAP_Z("*/"))) {
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
            lexer->scratch_buf + lexer->scratch_buf_size + 1, 
            lexer->scratch_buf_capacity - lexer->scratch_buf_size - 1);
        //
        // Parse the preprocessor directive
        //
        text_lexer_token(&pp_lexer);
        if (pp_lexer.token_kind == '#') {
            text_lexer_next(&pp_lexer);
            
            if (pp_lexer.token_kind == TEXT_LEXER_TOKEN_IDENT) {
                u32 found_keyword = get_pp_keyword_from_str(str(pp_lexer.string_buf, pp_lexer.string_buf_used));
                
                if (skip_to_next_matching_if) {
                    switch (found_keyword) {
                    default: goto start;
                    case PP_KEYWORD_IFDEF:
                    case PP_KEYWORD_IFNDEF:
                    case PP_KEYWORD_IF: {
                        ++if_skip_depth;
                        goto start;
                    } break;
                    case PP_KEYWORD_ELIF:
                    case PP_KEYWORD_ELIFDEF:
                    case PP_KEYWORD_ELIFNDEF:
                    case PP_KEYWORD_ELSE:
                    case PP_KEYWORD_ENDIF: {
                        if (if_skip_depth) {
                            if_skip_depth -= (found_keyword == PP_KEYWORD_ENDIF);
                            goto start;    
                        } else {
                            skip_to_next_matching_if = false;
                        }
                    } break;
                    }  
                }
                
                text_lexer_next(&pp_lexer);
                switch (found_keyword) {
                default: {
                    NOT_IMPLEMENTED;
                } break;
                case PP_KEYWORD_DEFINE: {
                    if (pp_lexer.token_kind == TEXT_LEXER_TOKEN_IDENT) {
                        Str macro_name = str(pp_lexer.string_buf, pp_lexer.string_buf_used);
                        PP_Macro *macro = pp_define(lexer, macro_name);
                        // @TODO(hl): Warn if macro is already defined
                        text_lexer_next(&pp_lexer);
                        if (pp_lexer.token_kind == '(') {
                            macro->flags |= PP_MACRO_IS_FUNCTION_LIKE_BIT;
                            
                            text_lexer_next(&pp_lexer);
                            while (pp_lexer.token_kind && pp_lexer.token_kind != ')') {
                                if (pp_lexer.token_kind == TEXT_LEXER_TOKEN_IDENT) {
                                    Str ident = string_storage_add(lexer->ctx->ss, pp_lexer.string_buf, pp_lexer.string_buf_used);
                                    macro->arg_names[macro->arg_count++] = ident;
                                    text_lexer_next(&pp_lexer);
                                } else if (pp_lexer.token_kind == ',') {
                                    text_lexer_next(&pp_lexer);
                                } else if (pp_lexer.token_kind == '.') {
                                    if (macro->flags & PP_MACRO_HAS_VARARGS_BIT) {
                                        NOT_IMPLEMENTED;
                                    }
                                    
                                    bool is_correct = false;
                                    text_lexer_next(&pp_lexer);
                                    if (pp_lexer.token_kind == '.') {
                                        text_lexer_next(&pp_lexer);
                                        if (pp_lexer.token_kind == '.') {
                                            text_lexer_next(&pp_lexer);
                                            macro->flags |= PP_MACRO_HAS_VARARGS_BIT;
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
                        Str macro_name = str(pp_lexer.string_buf, pp_lexer.string_buf_used);
                        pp_undef(lexer, macro_name);
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
                        Str macro_name = str(pp_lexer.string_buf, pp_lexer.string_buf_used);
                        PP_Macro *macro = pp_get(lexer, macro_name);
                        bool is_true = macro != 0;
                        pp_push_nested_if(lexer, is_true);
                        if (!is_true) {
                            skip_to_next_matching_if = true;
                        }
                    } else {
                        NOT_IMPLEMENTED;
                    }
                } break;
                case PP_KEYWORD_IFNDEF: {
                    if (pp_lexer.token_kind == TEXT_LEXER_TOKEN_IDENT) {
                        Str macro_name = str(pp_lexer.string_buf, pp_lexer.string_buf_used);
                        PP_Macro *macro = pp_get(lexer, macro_name);
                        bool is_true = macro == 0;
                        pp_push_nested_if(lexer, is_true);
                        if (!is_true) {
                            skip_to_next_matching_if = true;
                        }
                    } else {
                        NOT_IMPLEMENTED;
                    }
                } break;
                case PP_KEYWORD_ELSE: {
                    bool is_handled = pp_get_nested_if_handled(lexer);
                    if (is_handled) {
                        skip_to_next_matching_if = true;
                    } else {
                        pp_set_nested_if_handled(lexer);    
                    }
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
        if (parse(lexer, WRAP_Z("u8'"))) {
            char_lit_kind = CHAR_LIT_UTF8;
        } else if (parse(lexer, WRAP_Z("u'"))) {
            char_lit_kind = CHAR_LIT_U16;
        } else if (parse(lexer, WRAP_Z("U'"))) {\
            char_lit_kind = CHAR_LIT_U32;
        } else if (parse(lexer, WRAP_Z("L'"))) {
            char_lit_kind = CHAR_LIT_WIDE;
        } else if (parse(lexer, WRAP_Z("'"))) {
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
        if (parse(lexer, WRAP_Z("u8'"))) {
            string_lit_kind = STRING_LIT_UTF8;
        } else if (parse(lexer, WRAP_Z("u'"))) {
            string_lit_kind = STRING_LIT_U16;
        } else if (parse(lexer, WRAP_Z("U'"))) {\
            string_lit_kind = STRING_LIT_U32;
        } else if (parse(lexer, WRAP_Z("L'"))) {
            string_lit_kind = STRING_LIT_WIDE;
        } else if (parse(lexer, WRAP_Z("'"))) {
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
            Str test = PUNCTUATOR_STRINGS[punctuator_iter - 0x100];
            if (next_eq(lexer, test)) {
                punctuator_enum = punctuator_iter;
                mem_copy(lexer->scratch_buf + lexer->scratch_buf_size, test.data, test.len);
                lexer->scratch_buf_size += test.len;
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
    Str lexbuf_str = str(lexer->scratch_buf + last_size, lexer->scratch_buf_size - last_size);
    
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
                    if (str_eq(lexbuf_str, WRAP_Z("__VA_ARGS__"))) {
                        if (macro->flags & PP_MACRO_HAS_VARARGS_BIT) {
                            lexer->scratch_buf_size = last_size;
                            add_arg_expansion_buffer(lexer, lexbuf->macro_args[macro->varargs_idx]);
                            is_macro_argument = true;
                            break;
                        } else {
                            NOT_IMPLEMENTED;
                        }
                    }
                    
                    for (u32 macro_arg_name_idx = 0;
                        macro_arg_name_idx < macro->arg_count - TO_BOOL(macro->flags & PP_MACRO_HAS_VARARGS_BIT);
                        ++macro_arg_name_idx) {
                        Str test = macro->arg_names[macro_arg_name_idx];
                        if (str_eq(lexbuf_str, test)) {
                            Str arg_expansion = lexbuf->macro_args[macro_arg_name_idx];
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
            PP_Macro *macro = pp_get(lexer, lexbuf_str);
            if (macro) {
                // @NOTE(hl): CLEANUP buffer is get here ant initialized inline because 
                //  we need to write arguments to it. This is better be wrapped in a function to be consistent
                Lexer_Buffer *buffer = get_new_buffer_stack_entry(lexer);
                buffer->buf = buffer->at = macro->definition;
                buffer->size = macro->definition_len;
                buffer->kind = LEXER_BUFFER_MACRO;
                buffer->macro = macro;
                if (macro->flags & PP_MACRO_IS_FUNCTION_LIKE_BIT) {
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
                                (!(macro->flags & PP_MACRO_HAS_VARARGS_BIT) || buffer->macro_arg_count < macro->varargs_idx) ) {
                                // @HACK Currently we insert space so the algorithm can function 
                                // normally and parse macro properly if it is contained in another macro
                                // argument
                                temp_buffer[temp_buffer_size++] = ' ';
                                temp_buffer[temp_buffer_size] = 0;
                                Str new_str = string_storage_add(lexer->ctx->ss, temp_buffer, temp_buffer_size);
                                buffer->macro_args[buffer->macro_arg_count++] = new_str;
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
                            Str new_str = string_storage_add(lexer->ctx->ss, temp_buffer, temp_buffer_size);
                            buffer->macro_args[buffer->macro_arg_count++] = new_str;
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
       
        if (parse_no_scratch(lexer, WRAP_Z("##"))) {
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
        // @TODO(hl): Fully generate next token to check,
        // or check begginnigs of all string literals
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
            u32 found_keyword = get_keyword_from_str(lexbuf_str);
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
            Str new_str = string_storage_add(lexer->ctx->ss, lexbuf_str.data, lexbuf_str.len);
            token->str = new_str;
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
        u32 type = 0;
        switch (lit_kind) {
        case CHAR_LIT_REG: {
            type = C_TYPE_SLINT;
            if (*cursor == '\\') {
                Escape_Sequence_Get_Result escape_seq = get_escape_sequency_value(cursor);
                cursor = escape_seq.cusror;
                // @NOTE(hl): Truncate all values out of range
                value = (i32)escape_seq.value;
            } else {
                value = *cursor++;
            }
        } break;
        case CHAR_LIT_UTF8: {
            // C specification for utf8 character basiclly limits them to be the ASCII single-byte 
            // subset of the utf8 (without higher bit set)
            type = C_TYPE_UCHAR;
            u32 symb = *cursor++;
            if (is_ascii(symb)) {
                value = symb;
            } else {
                NOT_IMPLEMENTED;
            }
        } break;
        case CHAR_LIT_U16: {
            type = C_TYPE_CHAR16;
            u32 decoded = 0;
            // @NOTE(hl): Source characters are all converted to UTF8, so any multibyte character should be in it.
            // Difference between the character literals is in the type of their storage
            cursor = utf8_decode(cursor, &decoded);
            value = (i16)decoded;
        } break;
        case CHAR_LIT_U32: {
            type = C_TYPE_CHAR32;
            u32 decoded = 0;
            cursor = utf8_decode(cursor, &decoded);
            value = (i32)decoded;
        } break;
        case CHAR_LIT_WIDE: {
            type = C_TYPE_WCHAR;
            u32 decoded = 0;
            // @NOTE(hl): Source characters are all converted to UTF8, so any multibyte character should be in it.
            // Difference between the character literals is in the type of their storage
            cursor = utf8_decode(cursor, &decoded);
            value = (i16)decoded;
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