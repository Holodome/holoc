#include "c_lang.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "allocator.h"
#include "buffer_writer.h"
#include "c_types.h"
#include "pp_lexer.h"
#include "str.h"
#include "unicode.h"

static string KEYWORD_STRINGS[] = {
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

static string PUNCTUATOR_STRINGS[] = {
    WRAP_Z("(unknown)"), WRAP_Z(">>="), WRAP_Z("<<="), WRAP_Z("+="),
    WRAP_Z("-="),        WRAP_Z("*="),  WRAP_Z("/="),  WRAP_Z("%="),
    WRAP_Z("&="),        WRAP_Z("|="),  WRAP_Z("^="),  WRAP_Z("++"),
    WRAP_Z("--"),        WRAP_Z(">>"),  WRAP_Z("<<"),  WRAP_Z("&&"),
    WRAP_Z("||"),        WRAP_Z("=="),  WRAP_Z("!="),  WRAP_Z("<="),
    WRAP_Z(">="),
};

static string
get_kw_str(c_keyword_kind kind) {
    assert(kind < (sizeof(KEYWORD_STRINGS) / sizeof(*KEYWORD_STRINGS)));
    return KEYWORD_STRINGS[kind];
}

// NOTE: Only called for multisymbol punctuators (ot ASCII ones)
static string
get_punct_str(c_punct_kind kind) {
    assert(kind > 0x100);
    assert((kind - 0x100) <
           (sizeof(PUNCTUATOR_STRINGS) / sizeof(*PUNCTUATOR_STRINGS)));
    return PUNCTUATOR_STRINGS[kind - 0x100];
}

static c_punct_kind
get_c_punct_kind_from_pp(pp_punct_kind kind) {
    c_punct_kind result = 0;
    if (kind < 0x100) {
        result = (uint32_t)kind;
    } else {
        switch (kind) {
        default:
            assert(false);
        case PP_TOK_PUNCT_IRSHIFT:
            result = C_PUNCT_IRSHIFT;
            break;
        case PP_TOK_PUNCT_ILSHIFT:
            result = C_PUNCT_ILSHIFT;
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

uint32_t
fmt_token(token *tok, char *buf, uint32_t buf_size) {
    buffer_writer w = {buf, buf + buf_size};
    switch (tok->kind) {
    case TOK_EOF:
        break;
    case TOK_ID:
        buf_write(&w, "%.*s", tok->str.len, tok->str.data);
        break;
    case TOK_NUM:
        if (c_type_is_int(tok->type->kind)) {
            buf_write(&w, "%llu", tok->uint_value);
            switch (tok->type->kind) {
            default:
                break;
            case C_TYPE_ULLINT:
            case C_TYPE_SLLINT:
                buf_write(&w, "ll");
                break;
            case C_TYPE_ULINT:
            case C_TYPE_SLINT:
                buf_write(&w, "l");
                break;
            }
            if (c_type_is_int_unsigned(tok->type->kind)) {
                buf_write(&w, "u");
            }
        } else {
            buf_write(&w, "%Lg", tok->float_value);
            switch (tok->type->kind) {
            default:
                break;
            case C_TYPE_FLOAT:
                buf_write(&w, "f");
                break;
            case C_TYPE_LDOUBLE:
                buf_write(&w, "l");
                break;
            }
        }
        break;
    case TOK_STR:
        assert(tok->type->kind == C_TYPE_ARRAY);
        switch (tok->type->ptr_to->kind) {
        default:
            buf_write(&w, "\"");
            break;
        case C_TYPE_WCHAR:
            buf_write(&w, "L\"");
            break;
        case C_TYPE_UCHAR:
            buf_write(&w, "u8\"");
            break;
        case C_TYPE_CHAR16:
            buf_write(&w, "u\"");
            break;
        case C_TYPE_CHAR32:
            buf_write(&w, "U\"");
            break;
        }
        buf_write(&w, "%.*s", tok->str.len, tok->str.data);
        buf_write(&w, "\"");
        break;
    case TOK_PUNCT:
        if (tok->punct < 0x100) {
            buf_write(&w, "%c", tok->punct);
        } else {
            string punct_str = get_punct_str(tok->punct);
            buf_write(&w, "%.*s", punct_str.len, punct_str.data);
        }
        break;
    case TOK_KW: {
        string kw_str = get_kw_str(tok->kw);
        buf_write(&w, "%.*s", kw_str.len, kw_str.data);
    } break;
    }
    return w.cursor - buf;
}

uint32_t
fmt_tok_verbose(token *tok, char *buf, uint32_t buf_size) {
    uint32_t result = 0;
    switch (tok->kind) {
    case TOK_EOF:
        result = snprintf(buf, buf_size, "<EOF>");
        break;
    case TOK_ID:
        result = snprintf(buf, buf_size, "<ID>");
        break;
    case TOK_NUM:
        result = snprintf(buf, buf_size, "<Num>");
        break;
    case TOK_STR:
        result = snprintf(buf, buf_size, "<Str>");
        break;
    case TOK_PUNCT:
        result = snprintf(buf, buf_size, "<Punct>");
        break;
    case TOK_KW:
        result = snprintf(buf, buf_size, "<Kw>");
        break;
    }
    buf += result;
    buf_size -= result;
    result += fmt_token(tok, buf, buf_size);
    return result;
}

token
convert_pp_token(pp_token *pp_tok, struct allocator *a) {
    token tok = {0};
    switch (pp_tok->kind) {
    case PP_TOK_EOF: {
        tok.kind = TOK_EOF;
    } break;
    case PP_TOK_ID: {
        tok.kind = TOK_ID;
        tok.str  = string_dup(a, pp_tok->str);
    } break;
    case PP_TOK_NUM: {
        tok.kind = TOK_NUM;
    } break;
    case PP_TOK_STR: {
        switch (pp_tok->str_kind) {
            c_type_kind base_type_kind = 0;
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
            tok.type           = make_array_type(base_type, len + 1, a);
            void *buffer       = aalloc(a, byte_stride * (len + 1));
            void *write_cursor = buffer;
            cursor             = pp_tok->str.data;
            while (*cursor) {
                uint32_t cp;
                cursor = utf8_decode(cursor, &cp);
                if (byte_stride == 1) {
                    // TODO: If resulting string is utf8 we can skip decodind
                    // and encoding
                    write_cursor = utf8_encode(write_cursor, cp);
                } else if (byte_stride == 2) {
                    write_cursor = utf16_encode(write_cursor, cp);
                } else if (byte_stride == 4) {
                    memcpy(write_cursor, &cp, 4);
                    write_cursor = (char *)write_cursor + 4;
                } else {
                    assert(false);
                }
            }
            uint32_t zero = 0;
            memcpy(write_cursor, &zero, byte_stride);

            tok.kind = TOK_STR;
            tok.str  = string(buffer, byte_stride * len);
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
            goto process_char;
            base_type_kind = C_TYPE_CHAR32;
            goto process_char;
        case PP_TOK_STR_CWIDE: {
            base_type_kind = C_TYPE_WCHAR;
        process_char:
            (void)0;
            // Treat all character literals as mutltibyte.
            // Convert it to number
            uint64_t value       = 0;
            char *cursor         = pp_tok->str.data;
            uint32_t byte_stride = tok.type->size;
            while (*cursor) {
                uint32_t cp;
                cursor = utf8_decode(cursor, &cp);
                if (cp >= 1 << (8 * byte_stride)) {
                    // TODO: Diagnostic
                    break;
                }
                value = (value << (8 * byte_stride)) | cp;
            }
            // Make sure the value is in expected range
            value &= (1 << (8 * byte_stride)) - 1;
            tok.kind       = TOK_NUM;
            tok.uint_value = value;
        } break;
        }
    } break;
    case PP_TOK_PUNCT: {
        tok.kind  = TOK_PUNCT;
        tok.punct = get_c_punct_kind_from_pp(pp_tok->punct_kind);
        assert(tok.punct);
    } break;
    case PP_TOK_OTHER: {
        assert(false);
    } break;
    }
    return tok;
}
