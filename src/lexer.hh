#if !defined(LEXER_HH)

#include "general.hh"
#include "utf8.hh"

enum struct TokenKind {
    None = 0x0,
    EOS = 0x100, // End of stream
    Identifier,
    Integer,
    String
};

// Returns string equivalent to token ASCII reprsentation or its name in enum  
const char *token_kind_to_str(TokenKind kind);

struct Token {
    TokenKind kind = TokenKind::None;
    
    Str ident;
    i64 integer;
    
    u32 line_number;
    u32 col;
    u32 col_end;
    const u8 *cursor_at_tok;

    size_t format(char *buffer, size_t buffer_size);
};

void verror_at(const char *filename, const char *file_data, u32 line_number,
               const char *cursor, const char *format, va_list args) {
    const char *line_start = cursor;
    while (file_data < line_start && *(line_start - 1) != '\n') {
        --line_start; 
    }
    const char *line_end = cursor;
    while (*line_end && *line_end != '\n') {
        ++line_end;
    }

    u32 indent = fprintf(stderr, "%s:%d,%d: ", filename, line_number, (int)(cursor - line_start));
    fprintf(stderr, "%.*s\n", (int)(line_end - line_start), line_start);
    u32 indent_until_cursor = indent + (cursor - line_start);
    fprintf(stderr, "%*s", indent_until_cursor, "");
    fprintf(stderr, "^ ");
    vfprintf(stderr, format, args);
    fprintf(stderr, "\n");
}

static void error_at(const char *filename, const char *file_data, const char *cursor, const char *format, ...) {
    u32 line_number = 0;
    const char *temp = file_data;
    while (temp < cursor) {
        line_number += *temp == '\n';
        ++temp;
    }

    va_list args;
    va_start(args, format);
    verror_at(filename, file_data, line_number, cursor, format, args);
    va_end(args);
    exit(1);
}

struct Lexer {
    Str filename;
    const u8 *file_data = 0;
    size_t file_size = 0;
    const u8 *cursor = 0;

    Token *last_token = 0; 
    u32 current_line_number = 0;
    u32 current_character_index = 0;

    void init(const Str &filename, const void *buffer, size_t buffer_size);
    Token *peek_tok();
    void eat_tok();

    void error(Token *token, const char *format, ...) {
        va_list args;
        va_start(args, format);
        verror_at(filename.data, (const char *)file_data, token->line_number, (const char *)token->cursor_at_tok, format, args);
        va_end(args);
        exit(1);
    }
    
    void error(const char *format, ...) {
        assert(last_token);
        
        va_list args;
        va_start(args, format);
        verror_at(filename.data, (const char *)file_data, last_token->line_number, (const char *)last_token->cursor_at_tok, format, args);
        va_end(args);
        exit(1);
    }
};


#define LEXER_HH 1
#endif 
