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
}

static void 
read_utf16_string_literal(pp_lexer *lexer, char terminator) { 
}

static void 
read_utf32_string_literal(pp_lexer *lexer, char terminator) {

}

static bool
parse_string_literal(pp_lexer *lexer) {
    bool result = false;
    char *test_cursor = lexer->cursor;
    preprocessor_string_kind str_kind = 0;

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
        if (!str_kind) {
            str_kind = PP_TOK_STR_SCHAR;
        }
        
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

    return result;
}

static bool 
parse_punctuator(pp_lexer *lexer) {
    bool result = false;

    return result;
}

bool 
pp_lexer_parse(pp_lexer *lexer) {
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
