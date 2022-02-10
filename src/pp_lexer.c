#include "pp_lexer.h"

#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "buffer_writer.h"
#include "str.h"
#include "unicode.h"

#define PP_TOK_STR_ADVANCE 0x10
#define PP_TOK_PUNCT_ADVANCE 0x100

static string PUNCT_STRS[] = {
    WRAPZ(">>="), WRAPZ("<<="), WRAPZ("..."), WRAPZ("+="), WRAPZ("-="), WRAPZ("*="),
    WRAPZ("/="),  WRAPZ("%="),  WRAPZ("&="),  WRAPZ("|="), WRAPZ("^="), WRAPZ("++"),
    WRAPZ("--"),  WRAPZ(">>"),  WRAPZ("<<"),  WRAPZ("&&"), WRAPZ("||"), WRAPZ("=="),
    WRAPZ("!="),  WRAPZ("<="),  WRAPZ(">="),  WRAPZ("##"),
};

static string
get_str_opener(pp_string_kind kind) {
    string result = {0};
    switch (kind) {
        INVALID_DEFAULT_CASE;
    case PP_TOK_STR_SCHAR:
        result = (string)WRAPZ("\"");
        break;
    case PP_TOK_STR_SUTF8:
        result = (string)WRAPZ("u8\"");
        break;
    case PP_TOK_STR_SUTF16:
        result = (string)WRAPZ("u\"");
        break;
    case PP_TOK_STR_SUTF32:
        result = (string)WRAPZ("U\"");
        break;
    case PP_TOK_STR_SWIDE:
        result = (string)WRAPZ("L\"");
        break;
    case PP_TOK_STR_CCHAR:
        result = (string)WRAPZ("\'");
        break;
    case PP_TOK_STR_CUTF8:
        result = (string)WRAPZ("u8\'");
        break;
    case PP_TOK_STR_CUTF16:
        result = (string)WRAPZ("u\'");
        break;
    case PP_TOK_STR_CUTF32:
        result = (string)WRAPZ("U\'");
        break;
    case PP_TOK_STR_CWIDE:
        result = (string)WRAPZ("L\'");
        break;
    }

    return result;
}

static bool
next_eq(pp_lexer *lex, string lit) {
#if 1
    return (lex->cursor + lit.len < lex->eof) && (memcmp(lex->cursor, lit.data, lit.len) == 0);
#else
    // In theory memcmp does per-byte compares, so in won't touch uninitialized
    // memory. But it actually may use some form of SIMD so this assumption
    // would be incorrect.
    return memcmp(lex->cursor, lit.data, lit.len) == 0;
#endif
}

// Handling of whitespace characters
static bool
parse_whitespaces(pp_lexer *lex, pp_token *tok) {
    bool result = false;

    // Skip ASCII whitespaces
    while (isspace(*lex->cursor)) {
        result = true;
        if (*lex->cursor == '\n') {
            tok->at_line_start   = true;
            lex->last_line_start = lex->cursor + 1;
            ++lex->line;
        }
        ++lex->cursor;
    }

    // Skip single-line comments
    if (next_eq(lex, (string)WRAPZ("//"))) {
        result = true;
        while (*lex->cursor != '\n' && *lex->cursor) {
            ++lex->cursor;
        }

        if (*lex->cursor != 0) {
            ++lex->cursor;
            ++lex->line;
            tok->at_line_start   = true;
            lex->last_line_start = lex->cursor;
        }
    }

    // Skip multi-line comments
    if (next_eq(lex, (string)WRAPZ("/*"))) {
        result = true;
        while (*lex->cursor && !next_eq(lex, (string)WRAPZ("*/"))) {
            if (*lex->cursor == '\n') {
                lex->last_line_start = lex->cursor;
                ++lex->line;
            }
            ++lex->cursor;
        }

        if (*lex->cursor == 0) {
            printf("Unterminated multiline comment\n");
        } else {
            lex->cursor += 2;
        }
    }

    // Because skipping of whitespace characters in this function is done
    // sequentially, we may need to call this function multiple times (like if
    // multiline comment is followed by spaces they won't be skipped in single
    // pass)
    tok->has_whitespace = tok->has_whitespace || result;
    return result;
}

// Get digit number from its ASCII representation
static uint32_t
from_hex(char cp) {
    uint32_t result;
    if ('0' <= cp && cp <= '9') {
        result = cp - '0';
    } else if ('A' <= cp && cp <= 'F') {
        result = cp - 'A' + 10;
    } else {
        result = cp - 'a' + 10;
    }
    return result;
}

// Read at least 'len' characters of hex value.
static bool
read_unicode_value(pp_lexer *lex, uint32_t len, uint32_t *valuep) {
    bool result       = true;
    char *test_cursor = lex->cursor;
    uint32_t value    = 0;
    for (uint32_t idx = 0; idx < len; ++idx) {
        if (!isxdigit(*test_cursor)) {
            result = false;
            break;
        }
        value = (value << 4) | from_hex(*test_cursor++);
    }

    if (result) {
        *valuep = value;
    }
    return result;
}

static uint32_t
read_escaped_char(pp_lexer *lex) {
    uint32_t result = 0;

    if ('0' <= *lex->cursor && *lex->cursor <= '7') {
        uint32_t octal_value = *lex->cursor++ - '0';
        if ('0' <= *lex->cursor && *lex->cursor <= '7') {
            octal_value = (octal_value << 3) | (*lex->cursor++ - '0');
            if ('0' <= *lex->cursor && *lex->cursor <= '7') {
                octal_value = (octal_value << 3) | (*lex->cursor++ - '0');
            }
        }
        result = octal_value;
    } else if (*lex->cursor == 'x') {
        ++lex->cursor;
        if (!isxdigit(*lex->cursor)) {
            printf("Invalid hex constant\n");
        }

        // CLEANUP: We can use read_unicode_value here (rename it also) if set
        // len to some asurdely large value.
        uint32_t hex_value = 0;
        while (isxdigit(*lex->cursor)) {
            hex_value = (hex_value << 4) | from_hex(*lex->cursor++);
        }
        result = hex_value;
    } else if (*lex->cursor == 'u') {
        ++lex->cursor;
        uint32_t unicode_value;
        if (read_unicode_value(lex, 4, &unicode_value)) {
            result = unicode_value;
        } else {
            --lex->cursor;
            result = '\\';
        }
    } else if (*lex->cursor == 'U') {
        ++lex->cursor;
        uint32_t unicode_value;
        if (read_unicode_value(lex, 8, &unicode_value)) {
            result = unicode_value;
        } else {
            --lex->cursor;
            result = '\\';
        }
    } else {
        uint32_t cp = *lex->cursor++;
        switch (cp) {
        default:
            result = cp;
            break;
        case 'a':
            result = '\a';
            break;
        case 'b':
            result = '\b';
            break;
        case 'f':
            result = '\f';
            break;
        case 'n':
            result = '\n';
            break;
        case 'r':
            result = '\r';
            break;
        case 't':
            result = '\t';
            break;
        case 'v':
            result = '\v';
            break;
        }
    }

    return result;
}

static void
read_utf8_string_literal(pp_lexer *lex, char terminator, char *buf, uint32_t buf_size,
                         uint32_t *buf_writtenp) {
    char *write_cursor = buf;
    char *write_eof    = buf + buf_size;
    assert(write_cursor != write_eof);
    for (;;) {
        // Make sure buffer is always terminated
        if (write_cursor + 1 >= write_eof) {
            NOT_IMPL;
            break;
        }

        uint32_t cp = *lex->cursor++;
        if (cp == '\n') {
            printf("Unterminated string constant\n");
            break;
        } else if (cp == (uint32_t)terminator) {
            break;
        }

        if (cp == '\\') {
            cp = read_escaped_char(lex);
        }

        char utf8[5]    = {0};
        uint32_t cp_len = (char *)utf8_encode(utf8, cp) - utf8;
        if (write_cursor + cp_len >= write_eof) {
            break;
        }
        memcpy(write_cursor, utf8, cp_len);
        write_cursor += cp_len;
    }
    *write_cursor = 0;
    *buf_writtenp = write_cursor - buf;
    assert(*buf_writtenp < buf_size);
}

#if 0
static void
read_utf16_string_literal(pp_lexer *lex, char terminator) {
    uint16_t *write_cursor = (uint16_t *)lex->tok_buf;
    for (;;) {
        uint32_t cp = *lex->cursor++;
        if (cp == '\n') {
            printf("Unterminated string constant\n");
            break;
        } else if (cp == (uint32_t)terminator) {
            break;
        }

        if (cp == '\\') {
            cp = read_escaped_char(lex);
        }

        write_cursor = utf16_encode(write_cursor, cp);
    }
    *write_cursor      = 0;
    lex->tok_buf_len = (char *)write_cursor - lex->tok_buf;
}

static void
read_utf32_string_literal(pp_lexer *lex, char terminator) {
    uint32_t *write_cursor = (uint32_t *)lex->tok_buf;
    for (;;) {
        uint32_t cp = *lex->cursor++;
        if (cp == '\n') {
            printf("Unterminated string constant\n");
            break;
        } else if (cp == (uint32_t)terminator) {
            break;
        }

        if (cp == '\\') {
            cp = read_escaped_char(lex);
        }

        *write_cursor++ = cp;
    }
    *write_cursor      = 0;
    lex->tok_buf_len = (char *)write_cursor - lex->tok_buf;
}
#endif

static bool
parse_string_literal(pp_lexer *lex, pp_token *tok, char *buf, uint32_t buf_size,
                     uint32_t *buf_writtenp) {
    bool result             = false;
    char *test_cursor       = lex->cursor;
    pp_string_kind str_kind = PP_TOK_STR_SCHAR;

    if (*test_cursor == 'u' && test_cursor[1] == '8') {
        test_cursor += 2;
        str_kind = PP_TOK_STR_SUTF8;
    } else if (*test_cursor == 'u') {
        ++test_cursor;
        str_kind = PP_TOK_STR_SUTF16;
    } else if (*test_cursor == 'U') {
        ++test_cursor;
        str_kind = PP_TOK_STR_SUTF32;
    } else if (*test_cursor == 'L') {
        ++test_cursor;
        str_kind = PP_TOK_STR_SWIDE;
    }

    if (*test_cursor == '\'' || *test_cursor == '\"') {
        bool is_char    = *test_cursor == '\'';
        char terminator = *test_cursor++;
        lex->cursor     = test_cursor;
        result          = true;

        read_utf8_string_literal(lex, terminator, buf, buf_size, buf_writtenp);
#if 0
        switch (str_kind) {
        default:
            assert(false);
        case PP_TOK_STR_SCHAR:
        case PP_TOK_STR_SUTF8:
            read_utf8_string_literal(lex, terminator);
            break;
        case PP_TOK_STR_SUTF16:
            read_utf16_string_literal(lex, terminator);
            break;
        case PP_TOK_STR_SUTF32:
        case PP_TOK_STR_SWIDE:
            read_utf32_string_literal(lex, terminator);
            break;
        }
#endif
        if (is_char) {
            str_kind += PP_TOK_STR_ADVANCE;
        }
        tok->kind     = PP_TOK_STR;
        tok->str_kind = str_kind;
        tok->str      = (string){buf, *buf_writtenp};
    }

    return result;
}

static bool
parse_number(pp_lexer *lex, pp_token *tok, char *buf, uint32_t buf_size,
             uint32_t *buf_writtenp) {
    bool result        = false;
    char *write_cursor = buf;
    char *write_eof    = buf + buf_size;
    assert(write_cursor != write_eof);
    if (isdigit(*lex->cursor) || (*lex->cursor == '.' && isdigit(lex->cursor[1]))) {
        result          = true;
        *write_cursor++ = *lex->cursor++;
        for (;;) {
            if (write_cursor + 1 >= write_eof) {
                NOT_IMPL;
                break;
            }
            if (isalnum(*lex->cursor) || *lex->cursor == '_' || *lex->cursor == '\'') {
                *write_cursor++ = *lex->cursor++;
            } else if (*lex->cursor && strchr("eEpP", *lex->cursor) &&
                       strchr("+-", lex->cursor[1])) {
                *write_cursor++ = *lex->cursor++;
                *write_cursor++ = *lex->cursor++;
            } else {
                break;
            }
        }
        *write_cursor = 0;
        *buf_writtenp = write_cursor - buf;
        tok->str      = (string){buf, *buf_writtenp};
        assert(*buf_writtenp < buf_size);
        tok->kind = PP_TOK_NUM;
    }

    return result;
}

static bool
parse_punctuator(pp_lexer *lex, pp_token *tok) {
    bool result = false;
    for (uint32_t idx = 0; idx < ARRAY_SIZE(PUNCT_STRS); ++idx) {
        string punct = PUNCT_STRS[idx];
        if (next_eq(lex, punct)) {
            lex->cursor += punct.len;

            tok->kind       = PP_TOK_PUNCT;
            tok->punct_kind = PP_TOK_PUNCT_ADVANCE + idx;

            result = true;
            break;
        }
    }

    if (!result) {
        if (ispunct(*lex->cursor)) {
            tok->kind       = PP_TOK_PUNCT;
            tok->punct_kind = *lex->cursor++;

            result = true;
        }
    }

    return result;
}

static bool
parse_ident(pp_lexer *lex, pp_token *tok, char *buf, uint32_t buf_size,
            uint32_t *buf_writtenp) {
    bool result = false;
    if (isalpha(*lex->cursor) || *lex->cursor == '_') {
        char *write_cursor = buf;
        char *write_eof    = buf + buf_size;
        assert(write_cursor != write_eof);
        *write_cursor++ = *lex->cursor++;

        while (isalnum(*lex->cursor) || *lex->cursor == '_') {
            if (write_cursor + 1 >= write_eof) {
                NOT_IMPL;
                break;
            }
            *write_cursor++ = *lex->cursor++;
        }
        *write_cursor = 0;

        *buf_writtenp = write_cursor - buf;
        assert(*buf_writtenp < buf_size);
        tok->str  = (string){buf, *buf_writtenp};
        tok->kind = PP_TOK_ID;

        result = true;
    }
    return result;
}

bool
pp_lexer_parse(pp_lexer *lex, pp_token *tok, char *buf, uint32_t buf_size,
               uint32_t *buf_writtenp) {
    if (lex->cursor != lex->data) {
        tok->at_line_start = false;
    } else {
        tok->at_line_start = true;
    }

    for (;;) {
        {
            char cp = *lex->cursor;
            if (!cp) {
                tok->kind      = PP_TOK_EOF;
                lex->tok_start = lex->cursor;
                break;
            } else if (cp & 0x80) {
                lex->tok_start = lex->cursor;
                uint32_t t;
                lex->cursor   = utf8_decode(lex->cursor, &t);
                tok->kind     = PP_TOK_OTHER;
                buf[0]        = cp;
                buf[1]        = 0;
                *buf_writtenp = 1;
                tok->str      = (string){buf, *buf_writtenp};
                break;
            }
        }

        if (parse_whitespaces(lex, tok)) {
            continue;
        }

        lex->tok_start = lex->cursor;
        if (parse_string_literal(lex, tok, buf, buf_size, buf_writtenp)) {
            break;
        }

        if (parse_number(lex, tok, buf, buf_size, buf_writtenp)) {
            break;
        }

        if (parse_ident(lex, tok, buf, buf_size, buf_writtenp)) {
            break;
        }

        if (parse_punctuator(lex, tok)) {
            break;
        }

        assert(false);
    }
    tok->loc.line = lex->line;
    tok->loc.col  = lex->tok_start - lex->last_line_start + 1;
    bool is_eof   = tok->kind == PP_TOK_EOF;
    return !is_eof;
}

void
pp_lexer_init(pp_lexer *lex, char *data, char *eof) {
    lex->data            = data;
    lex->eof             = eof;
    lex->cursor          = data;
    lex->line            = 1;
    lex->last_line_start = data;
}

void
fmt_pp_tokw(buffer_writer *w, pp_token *tok) {
    switch (tok->kind) {
        INVALID_DEFAULT_CASE;
    case PP_TOK_EOF:
        break;
    case PP_TOK_ID:
    case PP_TOK_NUM:
    case PP_TOK_OTHER:
        buf_write(w, "%.*s", tok->str.len, tok->str.data);
        break;
    case PP_TOK_STR: {
        string str_opener = get_str_opener(tok->str_kind);
        if (str_opener.data) {
            char str_closer = str_opener.data[str_opener.len - 1];
            buf_write(w, "%s", str_opener.data);
            buf_write_raw_utf8(w, tok->str.data);
            buf_write(w, "%c", str_closer);
        }
    } break;
    case PP_TOK_PUNCT:
        if (tok->punct_kind < 0x100) {
            buf_write(w, "%c", tok->punct_kind);
        } else {
            string punct = PUNCT_STRS[tok->punct_kind - PP_TOK_PUNCT_ADVANCE];
            buf_write(w, "%s", punct.data);
        }
        break;
    }
}

uint32_t
fmt_pp_tok(char *buf, uint32_t buf_len, pp_token *tok) {
    buffer_writer w = {buf, buf + buf_len};
    fmt_pp_tokw(&w, tok);
    return w.cursor - buf;
}

void
fmt_pp_tok_verbosew(buffer_writer *w, pp_token *tok) {
    buf_write(w, "%.*s:%u:%u: ", tok->loc.filename.len, tok->loc.filename.data, tok->loc.line,
              tok->loc.col);
    switch (tok->kind) {
        INVALID_DEFAULT_CASE;
    case PP_TOK_EOF:
        buf_write(w, "<EOF>");
        break;
    case PP_TOK_ID:
        buf_write(w, "<ID>");
        break;
    case PP_TOK_NUM:
        buf_write(w, "<Num>");
        break;
    case PP_TOK_STR:
        buf_write(w, "<Str>");
        break;
    case PP_TOK_PUNCT:
        buf_write(w, "<Punct>");
        break;
    case PP_TOK_OTHER:
        buf_write(w, "<Other>");
        break;
    }
    fmt_pp_tokw(w, tok);
}

uint32_t
fmt_pp_tok_verbose(char *buf, uint32_t buf_len, pp_token *tok) {
    buffer_writer w = {buf, buf + buf_len};
    fmt_pp_tok_verbosew(&w, tok);
    return w.cursor - buf;
}
