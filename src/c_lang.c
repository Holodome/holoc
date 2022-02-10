#include "c_lang.h"

#include <assert.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>

#include "buffer_writer.h"
#include "c_types.h"
#include "pp_lexer.h"
#include "str.h"
#include "unicode.h"

static string KEYWORD_STRINGS[] = {
    WRAPZ("(unknown)"),
    WRAPZ("auto"),
    WRAPZ("break"),
    WRAPZ("case"),
    WRAPZ("char"),
    WRAPZ("const"),
    WRAPZ("continue"),
    WRAPZ("default"),
    WRAPZ("do"),
    WRAPZ("double"),
    WRAPZ("else"),
    WRAPZ("enum"),
    WRAPZ("extern"),
    WRAPZ("float"),
    WRAPZ("for"),
    WRAPZ("goto"),
    WRAPZ("if"),
    WRAPZ("inline"),
    WRAPZ("int"),
    WRAPZ("long"),
    WRAPZ("register"),
    WRAPZ("restrict"),
    WRAPZ("return"),
    WRAPZ("short"),
    WRAPZ("signed"),
    WRAPZ("unsigned"),
    WRAPZ("static"),
    WRAPZ("struct"),
    WRAPZ("switch"),
    WRAPZ("typedef"),
    WRAPZ("union"),
    WRAPZ("unsigned"),
    WRAPZ("void"),
    WRAPZ("volatile"),
    WRAPZ("while"),
    WRAPZ("_Alignas"),
    WRAPZ("_Alignof"),
    WRAPZ("_Atomic"),
    WRAPZ("_Bool"),
    WRAPZ("_Complex"),
    WRAPZ("_Decimal128"),
    WRAPZ("_Decimal32"),
    WRAPZ("_Decimal64"),
    WRAPZ("_Generic"),
    WRAPZ("_Imaginary"),
    WRAPZ("_Noreturn"),
    WRAPZ("_Static_assert"),
    WRAPZ("_Thread_local"),
    WRAPZ("_Pragma"),
};

static string PUNCTUATOR_STRINGS[] = {
    WRAPZ("(unknown)"), WRAPZ(">>="), WRAPZ("<<="), WRAPZ("+="), WRAPZ("-="), WRAPZ("*="),
    WRAPZ("/="),        WRAPZ("%="),  WRAPZ("&="),  WRAPZ("|="), WRAPZ("^="), WRAPZ("++"),
    WRAPZ("--"),        WRAPZ(">>"),  WRAPZ("<<"),  WRAPZ("&&"), WRAPZ("||"), WRAPZ("=="),
    WRAPZ("!="),        WRAPZ("<="),  WRAPZ(">="),  WRAPZ("...")};

static string
get_kw_str(c_keyword_kind kind) {
    assert(kind < ARRAY_SIZE(KEYWORD_STRINGS));
    return KEYWORD_STRINGS[kind];
}

// NOTE: Only called for multi symbol punctuators (not ASCII ones)
static string
get_punct_str(c_punct_kind kind) {
    assert(kind > 0x100);
    assert((kind - 0x100) < ARRAY_SIZE(PUNCTUATOR_STRINGS));
    return PUNCTUATOR_STRINGS[kind - 0x100];
}

static c_punct_kind
get_c_punct_kind_from_pp(pp_punct_kind kind) {
    c_punct_kind result = 0;
    if (kind < 0x100) {
        if (kind != '#') {
            result = (uint32_t)kind;
        }
    } else {
        switch (kind) {
            INVALID_DEFAULT_CASE;
        case PP_TOK_PUNCT_IRSHIFT:
            result = C_PUNCT_IRSHIFT;
            break;
        case PP_TOK_PUNCT_ILSHIFT:
            result = C_PUNCT_ILSHIFT;
            break;
        case PP_TOK_PUNCT_VARARGS:
            result = C_PUNCT_VARARGS;
            break;
        case PP_TOK_PUNCT_IADD:
            result = C_PUNCT_IADD;
            break;
        case PP_TOK_PUNCT_ISUB:
            result = C_PUNCT_ISUB;
            break;
        case PP_TOK_PUNCT_IMUL:
            result = C_PUNCT_IMUL;
            break;
        case PP_TOK_PUNCT_IDIV:
            result = C_PUNCT_IDIV;
            break;
        case PP_TOK_PUNCT_IMOD:
            result = C_PUNCT_IMOD;
            break;
        case PP_TOK_PUNCT_IAND:
            result = C_PUNCT_IAND;
            break;
        case PP_TOK_PUNCT_IOR:
            result = C_PUNCT_IOR;
            break;
        case PP_TOK_PUNCT_IXOR:
            result = C_PUNCT_IXOR;
            break;
        case PP_TOK_PUNCT_INC:
            result = C_PUNCT_INC;
            break;
        case PP_TOK_PUNCT_DEC:
            result = C_PUNCT_DEC;
            break;
        case PP_TOK_PUNCT_RSHIFT:
            result = C_PUNCT_RSHIFT;
            break;
        case PP_TOK_PUNCT_LSHIFT:
            result = C_PUNCT_LSHIFT;
            break;
        case PP_TOK_PUNCT_LAND:
            result = C_PUNCT_LAND;
            break;
        case PP_TOK_PUNCT_LOR:
            result = C_PUNCT_LOR;
            break;
        case PP_TOK_PUNCT_EQ:
            result = C_PUNCT_EQ;
            break;
        case PP_TOK_PUNCT_NEQ:
            result = C_PUNCT_NEQ;
            break;
        case PP_TOK_PUNCT_LEQ:
            result = C_PUNCT_LEQ;
            break;
        case PP_TOK_PUNCT_GEQ:
            result = C_PUNCT_GEQ;
            break;
        }
    }
    return result;
}

static c_keyword_kind
get_kw_kind(string test) {
    c_keyword_kind kind = 0;
    for (uint32_t i = 0; i < ARRAY_SIZE(KEYWORD_STRINGS); ++i) {
        if (string_eq(test, KEYWORD_STRINGS[i])) {
            kind = i;
            break;
        }
    }
    return kind;
}

bool
convert_pp_token(pp_token *pp_tok, token *tok, char *buf, uint32_t buf_size,
                 uint32_t *buf_writtenp) {
    bool result         = false;
    tok->at_line_start  = pp_tok->at_line_start;
    tok->has_whitespace = pp_tok->has_whitespace;
    tok->loc            = pp_tok->loc;
    switch (pp_tok->kind) {
    case PP_TOK_EOF: {
        tok->kind = TOK_EOF;
        result    = true;
    } break;
    case PP_TOK_ID: {
        c_keyword_kind kw = get_kw_kind(pp_tok->str);
        if (kw) {
            tok->kind = TOK_KW;
            tok->kw   = kw;
        } else {
            tok->kind     = TOK_ID;
            *buf_writtenp = snprintf(buf, buf_size, "%.*s", pp_tok->str.len, pp_tok->str.data);
            tok->str      = (string){buf, *buf_writtenp};
        }
        result = true;
    } break;
    case PP_TOK_NUM: {
        c_number_convert_result convert = convert_c_number(pp_tok->str.data);

        if (convert.is_valid) {
            tok->kind = TOK_NUM;
            if (convert.is_float) {
                tok->float_value = convert.float_value;
            } else {
                tok->uint_value = convert.uint_value;
            }
            tok->type = get_standard_type(convert.type_kind);
            assert(tok->type);
            result = true;
        } else {
            DEBUG_BREAKPOINT;
        }
    } break;
    case PP_TOK_STR: {
        switch (pp_tok->str_kind) {
            c_type_kind base_type_kind;
        default:
            assert(false);
            break;
        case PP_TOK_STR_SCHAR:
            base_type_kind = C_TYPE_CHAR;
            goto process_string;
        case PP_TOK_STR_SUTF8:
            base_type_kind = C_TYPE_UCHAR;
            goto process_string;
        case PP_TOK_STR_SUTF16:
            base_type_kind = C_TYPE_CHAR16;
            goto process_string;
        case PP_TOK_STR_SUTF32:
            base_type_kind = C_TYPE_CHAR32;
            goto process_string;
        case PP_TOK_STR_SWIDE: {
            base_type_kind = C_TYPE_WCHAR;
        process_string:
            (void)0;
            c_type *base_type    = get_standard_type(base_type_kind);
            uint32_t byte_stride = base_type->size;

            uint32_t len = 0;
            char *cursor = pp_tok->str.data;
            while (*cursor) {
                uint32_t cp;
                cursor = utf8_decode(cursor, &cp);
                ++len;
            }
            uintptr_t memory_size = byte_stride * (len + 1);
            assert(memory_size <= buf_size);
            void *write_cursor = buf;
            cursor             = pp_tok->str.data;
            while (*cursor) {
                uint32_t cp;
                cursor = utf8_decode(cursor, &cp);
                if (byte_stride == 1) {
                    // TODO: If resulting string is utf8 we can skip decoding
                    // and encoding
                    write_cursor = utf8_encode(write_cursor, cp);
                } else if (byte_stride == 2) {
                    write_cursor = utf16_encode(write_cursor, cp);
                } else if (byte_stride == 4) {
                    memcpy(write_cursor, &cp, 4);
                    write_cursor = (char *)write_cursor + 4;
                } else {
                    UNREACHABLE;
                }
            }
            uint32_t zero = 0;
            memcpy(write_cursor, &zero, byte_stride);
            *buf_writtenp = byte_stride * len;

            tok->kind = TOK_STR;
            tok->str  = (string){buf, byte_stride * len};
            tok->type = make_array_type(base_type, len + 1);
            result    = true;
        } break;
        case PP_TOK_STR_CCHAR:
            base_type_kind = C_TYPE_CHAR;
            goto process_char;
        case PP_TOK_STR_CUTF8:
            base_type_kind = C_TYPE_UCHAR;
            goto process_char;
        case PP_TOK_STR_CUTF16:
            base_type_kind = C_TYPE_CHAR16;
            goto process_char;
        case PP_TOK_STR_CUTF32:
            base_type_kind = C_TYPE_CHAR32;
            goto process_char;
        case PP_TOK_STR_CWIDE: {
            base_type_kind = C_TYPE_WCHAR;
        process_char:
            (void)0;
            c_type *base_type = get_standard_type(base_type_kind);
            // Treat all character literals as mutlti byte.
            // Convert it to number
            uint64_t value       = 0;
            char *cursor         = pp_tok->str.data;
            uint32_t byte_stride = base_type->size;
            while (*cursor) {
                uint32_t cp;
                cursor = utf8_decode(cursor, &cp);
                if (cp >= 1u << (8u * byte_stride)) {
                    // TODO: Diagnostic
                    break;
                }
                value = (value << (8 * byte_stride)) | cp;
            }
            // Make sure the value is in expected range
            value &= (1 << (8 * byte_stride)) - 1;
            tok->kind       = TOK_NUM;
            tok->uint_value = value;
            tok->type       = base_type;
            result          = true;
        } break;
        }
    } break;
    case PP_TOK_PUNCT: {
        c_punct_kind punct = get_c_punct_kind_from_pp(pp_tok->punct_kind);
        if (punct) {
            tok->kind  = TOK_PUNCT;
            tok->punct = punct;
            result     = true;
        }
    } break;
    case PP_TOK_OTHER: {
    } break;
    }
    return result;
}

void
fmt_c_numw(fmt_c_num_args args, buffer_writer *w) {
    if (c_type_kind_is_int(args.type->kind)) {
        buf_write(w, "%" PRIu64, args.uint_value);
        switch (args.type->kind) {
        default:
            break;
        case C_TYPE_ULLINT:
        case C_TYPE_SLLINT:
            buf_write(w, "ll");
            break;
        case C_TYPE_ULINT:
        case C_TYPE_SLINT:
            buf_write(w, "l");
            break;
        }
        if (c_type_kind_is_int_unsigned(args.type->kind)) {
            buf_write(w, "u");
        }
    } else {
        buf_write(w, "%Lg", args.float_value);
        switch (args.type->kind) {
        default:
            break;
        case C_TYPE_FLOAT:
            buf_write(w, "f");
            break;
        case C_TYPE_LDOUBLE:
            buf_write(w, "l");
            break;
        }
    }
}

uint32_t
fmt_c_numr(fmt_c_num_args args, char *buf, uint32_t buf_size) {
    buffer_writer w = {buf, buf + buf_size};
    fmt_c_numw(args, &w);
    return w.cursor - buf;
}

void
fmt_c_strw(fmt_c_str_args args, buffer_writer *w) {
    switch (args.type->ptr_to->kind) {
    default:
        buf_write(w, "\"");
        buf_write_raw_utf8(w, args.str.data);
        break;
    case C_TYPE_UCHAR:
        buf_write(w, "u8\"");
        buf_write_raw_utf8(w, args.str.data);
        break;
    case C_TYPE_CHAR16:
        buf_write(w, "u\"");
        buf_write_raw_utf16(w, args.str.data);
        break;
    case C_TYPE_CHAR32:
        buf_write(w, "U\"");
        buf_write_raw_utf32(w, args.str.data);
        break;
    case C_TYPE_WCHAR:
        buf_write(w, "L\"");
        buf_write_raw_utf32(w, args.str.data);
        break;
    }
    buf_write(w, "\"");
}

uint32_t
fmt_c_str(fmt_c_str_args args, char *buf, uint32_t buf_size) {
    buffer_writer w = {buf, buf + buf_size};
    fmt_c_strw(args, &w);
    return w.cursor - buf;
}

void
fmt_tokenw(buffer_writer *w, token *tok) {
    switch (tok->kind) {
    case TOK_EOF:
        buf_write(w, "%s", "");
        break;
    case TOK_ID:
        buf_write(w, "%.*s", tok->str.len, tok->str.data);
        break;
    case TOK_NUM:
        fmt_c_numw((fmt_c_num_args){.uint_value  = tok->uint_value,
                                    .float_value = tok->float_value,
                                    .type        = tok->type},
                   w);
        break;
    case TOK_STR:
        fmt_c_strw((fmt_c_str_args){.type = tok->type, .str = tok->str}, w);
        break;
    case TOK_PUNCT:
        if (tok->punct < 0x100) {
            buf_write(w, "%c", tok->punct);
        } else {
            string punct_str = get_punct_str(tok->punct);
            buf_write(w, "%.*s", punct_str.len, punct_str.data);
        }
        break;
    case TOK_KW: {
        string kw_str = get_kw_str(tok->kw);
        buf_write(w, "%.*s", kw_str.len, kw_str.data);
    } break;
    }
}

uint32_t
fmt_token(char *buf, uint32_t buf_len, token *tok) {
    buffer_writer w = {buf, buf + buf_len};
    fmt_tokenw(&w, tok);
    return w.cursor - buf;
}

void
fmt_token_verbosew(buffer_writer *w, token *tok) {
    buf_write(w, "%.*s:%u:%u: ", tok->loc.filename.len, tok->loc.filename.data, tok->loc.line,
              tok->loc.col);
    switch (tok->kind) {
        INVALID_DEFAULT_CASE;
    case TOK_EOF:
        buf_write(w, "<EOF>");
        break;
    case TOK_ID:
        buf_write(w, "<ID>");
        break;
    case TOK_NUM:
        buf_write(w, "<Num>");
        break;
    case TOK_STR:
        buf_write(w, "<Str>");
        break;
    case TOK_PUNCT:
        buf_write(w, "<Punct>");
        break;
    case TOK_KW:
        buf_write(w, "<Kw>");
        break;
    }
    fmt_tokenw(w, tok);
}

uint32_t
fmt_token_verbose(char *buf, uint32_t buf_len, token *tok) {
    buffer_writer w = {buf, buf + buf_len};
    fmt_token_verbosew(&w, tok);
    return w.cursor - buf;
}
