#include "lexer.h"

#include "str.h"
#include "unicode.h"

#include <stdio.h>
#include <assert.h>
#include <ctype.h>
#include <string.h>

static bool
next_eq(pp_lexer *lexer, string lit) {
    return (lexer->cursor + lit.len < lexer->eof) &&
        (memcmp(lexer->cursor, lit.data, lit.len) == 0);
}

static bool
parse_whitespaces(pp_lexer *lexer) {
    bool result = false;

    while (isspace(*lexer->cursor)) {
        result = true;
        if (*lexer->cursor == '\n') {
            lexer->tok_at_line_start = true;
        }
        ++lexer->cursor;
    }

    if (next_eq(lexer, WRAP_Z("//"))) {
        result = true;
        while (*lexer->cursor != '\n' && *lexer->cursor) {
            ++lexer->cursor;
        }

        lexer->cursor += (*lexer->cursor != 0);
    }

    if (next_eq(lexer, WRAP_Z("/*"))) {
        result = true;
        while (*lexer->cursor && !next_eq(lexer, WRAP_Z("*/"))) {
            ++lexer->cursor;
        }

        if (*lexer->cursor == 0) {
            printf("Unterminated multiline comment\n");
        } else {
            lexer->cursor += 2;
        }
    }

    lexer->tok_has_whitespace = result;
    return result;
}

static uint32_t 
from_hex(char cp) {
    uint32_t result = 0;
    if ('0' <= cp && cp <= '9') {
        result = cp - '0';
    } else if ('A' <= cp && cp <= 'F') {
        result = cp - 'A' + 10;
    } else {
        result = cp - 'a' + 10;
    }
    return result;
}

static bool
read_unicode_value(pp_lexer *lexer, uint32_t len, uint32_t *valuep) {
    bool result = true;
    char *test_cursor = lexer->cursor;
    uint32_t value = 0;
    for (uint32_t idx = 0; 
         idx < len;
         ++idx) {
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
read_escaped_char(pp_lexer *lexer) {
    uint32_t result = 0;
    
    if ('0' <= *lexer->cursor && *lexer->cursor <= '7') {
        uint32_t octal_value = *lexer->cursor++ - '0';
        if ('0' <= *lexer->cursor && *lexer->cursor <= '7') {
            octal_value = (octal_value << 3) | (*lexer->cursor++ - '0');
            if ('0' <= *lexer->cursor && *lexer->cursor <= '7') {
                octal_value = (octal_value << 3) | (*lexer->cursor++ - '0');
            }
        }
        result = octal_value;
    } else if (*lexer->cursor == 'x') {
        ++lexer->cursor;
        if (!isxdigit(*lexer->cursor)) {
            printf("Invalid hex constant\n");
        }

        uint32_t hex_value = 0;
        while (isxdigit(*lexer->cursor)) {
            hex_value = (hex_value << 4) | from_hex(*lexer->cursor++);
        }
        result = hex_value;
    } else if (*lexer->cursor == 'u') {
        ++lexer->cursor;
        uint32_t unicode_value;
        if (read_unicode_value(lexer, 4, &unicode_value)) {
            result = unicode_value;
        } else {
            --lexer->cursor;
            result = '\\';
        }
    } else if (*lexer->cursor == 'U') {
        ++lexer->cursor;
        uint32_t unicode_value;
        if (read_unicode_value(lexer, 4, &unicode_value)) {
            result = unicode_value;
        } else {
            --lexer->cursor;
            result = '\\';
        }
    } else {
        uint32_t cp = *lexer->cursor++;
        switch (cp) {
        default: 
            result = cp;
            break;
        case '\'':
            result = '\'';
            break;
        case '\"':
            result = '\"';
            break;
        case '?':
            result = '?';
            break;
        case '\\':
            result = '\\';
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
read_utf8_string_literal(pp_lexer *lexer, char terminator) { 
    char *write_cursor = lexer->tok_buf;
    for (;;) {
        uint32_t cp = *lexer->cursor++;
        if (cp == '\n') {
            printf("Unterminated string constant\n");
            break;
        } else if (cp == (uint32_t)terminator) {
            break;
        }

        if (cp == '\\') {
            cp = read_escaped_char(lexer);
        } 

        write_cursor = utf8_encode(write_cursor, cp);
    }
    *write_cursor = 0;
    lexer->tok_buf_len = (char *)write_cursor - lexer->tok_buf;
}

static void 
read_utf16_string_literal(pp_lexer *lexer, char terminator) { 
    uint16_t *write_cursor = (uint16_t *)lexer->tok_buf;
    for (;;) {
        uint32_t cp = *lexer->cursor++;
        if (cp == '\n') {
            printf("Unterminated string constant\n");
            break;
        } else if (cp == (uint32_t)terminator) {
            break;
        }

        if (cp == '\\') {
            cp = read_escaped_char(lexer);
        } 

        write_cursor = utf16_encode(write_cursor, cp);
    } 
    *write_cursor = 0;
    lexer->tok_buf_len = (char *)write_cursor - lexer->tok_buf;
}

static void 
read_utf32_string_literal(pp_lexer *lexer, char terminator) { 
    uint32_t *write_cursor = (uint32_t *)lexer->tok_buf;
    for (;;) {
        uint32_t cp = *lexer->cursor++;
        if (cp == '\n') {
            printf("Unterminated string constant\n");
            break;
        } else if (cp == (uint32_t)terminator) {
            break;
        }

        if (cp == '\\') {
            cp = read_escaped_char(lexer);
        } 

        *write_cursor++ = cp;
    } 
    *write_cursor = 0;
    lexer->tok_buf_len = (char *)write_cursor - lexer->tok_buf;
}

static bool
parse_string_literal(pp_lexer *lexer) {
    bool result = false;
    char *test_cursor = lexer->cursor;
    preprocessor_string_kind str_kind = PP_TOK_STR_SCHAR;

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
        char terminator = *test_cursor++;
        lexer->cursor = test_cursor;
        result = true;

        switch (str_kind) {
        default: assert(false);
        case PP_TOK_STR_SCHAR:
        case PP_TOK_STR_SUTF8:
            read_utf8_string_literal(lexer, terminator);
            break;
        case PP_TOK_STR_SUTF16:
            read_utf16_string_literal(lexer, terminator);
            break;
        case PP_TOK_STR_SUTF32:
        case PP_TOK_STR_SWIDE:
            read_utf32_string_literal(lexer, terminator);
            break;
        }

        if (*test_cursor == '\'') {
            str_kind += 0x10;
        } 
    }

    return result;

}

static bool
parse_number(pp_lexer *lexer) {
    bool result = false;
    char *write_cursor = lexer->tok_buf;
    if (isdigit(*lexer->cursor) || (*lexer->cursor == '.' && isdigit(lexer->cursor[1]))) {
        result = true;
        *write_cursor++ = *lexer->cursor++;
        for (;;) {
            if (isalnum(*lexer->cursor) || *lexer->cursor == '_' || *lexer->cursor == '\'') {
                *write_cursor++ = *lexer->cursor++;
            } else if (*lexer->cursor && strchr("eEpP", *lexer->cursor) && strchr("+-", lexer->cursor[1])) {
                *write_cursor++ = *lexer->cursor++;
                *write_cursor++ = *lexer->cursor++;
            } else {
                break;
            }
        }
        *write_cursor = 0;
        lexer->tok_buf_len = write_cursor - lexer->tok_buf;
    }

    return result;
}

static bool 
parse_punctuator(pp_lexer *lexer) {
    bool result = false;

    return result;
}

bool 
pp_lexer_parse(pp_lexer *lexer) {
    lexer->tok_at_line_start = false;

    for (;;) {
        {
            char cp = *lexer->cursor;
            if (!cp) {
                lexer->tok_kind = PP_TOK_EOF;
                break;
            } else if (cp & 0x80) {
                lexer->tok_kind = PP_TOK_OTHER;
                lexer->tok_buf[0] = cp;
                lexer->tok_buf[1] = 0;
                lexer->tok_buf_len = 1;
                break;
            }
        }

        if (parse_whitespaces(lexer)) {
            continue;
        }

        if (parse_string_literal(lexer)) {
            break;
        }
        
        if (parse_number(lexer)) {
            break;
        }

        if (parse_punctuator(lexer)) {
            break;
        }

        assert(false);
    }

    return lexer->tok_kind != PP_TOK_EOF;
}
