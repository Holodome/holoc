#if !defined(LEXER_HH)

#include "general.hh"
#include "utf8.hh"

enum struct TokenKind {
    None = 0x0,
    EOS = 0x100, // End of stream
    Identifier,
    Integer,
    String,
    Unknown // Means that we encountered error
};

// Returns string equivalent to token ASCII reprsentation or its name in enum  
const char *token_kind_to_str(TokenKind kind);

struct Token {
    TokenKind kind = TokenKind::None;
    
    u32 utf32; // Unicode codepoint for error reporting
    StrUTF8 ident; // Identifier, String
    i64 integer;
    
    u32 line_number;
    u32 char_number;

    size_t format(char *buffer, size_t buffer_size);
};

struct Lexer {
    const u8 *file_data = 0;
    size_t file_size = 0;
    
    const u8 *cursor = 0;  // Internal cursor, should not be read directly
    u32 current_utf32 = 0; // Last unicode character
    u32 last_char_byte_length = 0;

    Token *last_token = 0; 
    u32 current_line_number = 0;
    u32 current_char_number = 0;
    
    void init(const void *buffer, size_t buffer_size);
    Token *peek_tok();
    void eat_tok();
private:
    void advance_character();
};


#define LEXER_HH 1
#endif 
