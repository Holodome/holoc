#include "lexer.hh"

const char *token_kind_to_str(TokenKind kind) {
    const char *result = 0;
    
    static char ascii_table[512] = {};
    static const char *token_names[] = {
        "EOS",
        "Identifier",
        "Integer",
        "String"
    };
    u32 ik = (u32)kind;
    if (ik <= 0xFF) {
        if (!ascii_table[ik]) {
            snprintf(ascii_table + ik * 2, 2, "%c", ik);
        }
        result = ascii_table + ik * 2;
    } else {
        result = token_names[ik - (u32)TokenKind::EOS];  
    }
    
    return result;
}

size_t Token::format(char *buffer, size_t buffer_size) {
    char *buffer_init = buffer;
    buffer += snprintf(buffer, buffer_size, "<Token kind='%s'", token_kind_to_str(kind));
    buffer_size -= buffer - buffer_init;
    if ((u32)kind > 0xFF) {
        switch(kind) {
            case TokenKind::Identifier: {
                buffer += snprintf(buffer, buffer_size, ", ident='%s'", ident.data);
                buffer_size -= buffer - buffer_init;
            } break;
            case TokenKind::Integer: {
                buffer += snprintf(buffer, buffer_size, ", int=%lli", integer);
                buffer_size -= buffer - buffer_init;
            } break;
            case TokenKind::String: {
                buffer += snprintf(buffer, buffer_size, ", str='%s'", ident.data);
                buffer_size -= buffer - buffer_init;
            } break;
            default: {
                assert(false);  
            };
        }
    } 
    buffer += snprintf(buffer, buffer_size, ">");
    buffer_size -= buffer - buffer_init;
    
    return buffer - buffer_init;
}
 
void Lexer::init(const void *buffer, size_t buffer_size) {
    file_data = (const u8 *)buffer;
    file_size = buffer_size;
    cursor = file_data;
    
    advance_character();
    current_char_number = 0;
    current_line_number = 1;
}

Token *Lexer::peek_tok() {
    if (last_token) {
        return last_token;
    }   

    Token *token = new Token;
    
    last_token = token;
    for (;;) {
        if (!current_utf32) {
            token->kind = TokenKind::EOS;
            break;
        }

        // Skip and continue
        if (current_utf32 == '\n') {
            ++current_line_number;
            current_char_number = 0;
            advance_character();
            continue;
        } else if (isspace(current_utf32)) {
            advance_character();
            continue;
        } else if (current_utf32 == '#') {
            while (current_utf32 != '\n') {
                advance_character();
            }
            continue;
        } 
        
        // Parse and break
        token->char_number = current_char_number;
        token->line_number = current_line_number;
        if (isdigit(current_utf32)) {
            i64 value = 0;
            while (isdigit(current_utf32)) {
                value = value * 10 + (current_utf32 - '0');
                advance_character();
            }
            token->kind = TokenKind::Integer;
            token->integer = value;
            break;
        } else if (isalpha(current_utf32) && current_utf32 <= 0xFF) {
            // @HACK
            const u8 *ident_start = cursor - last_char_byte_length;
            do {
                advance_character();
            } while(isalpha(current_utf32) || isdigit(current_utf32) || current_utf32 == '_');
            token->ident = Str((const char *)ident_start, 0u, cursor - ident_start - last_char_byte_length);
            token->kind = TokenKind::Identifier;
            break;
        } else if (ispunct(current_utf32)) {
            token->kind = (TokenKind)((u8)current_utf32);
            advance_character();
            break;
        } else {
            token->kind = TokenKind::Unknown;
            token->utf32 = current_utf32;
            break;
        }
    }

    return token;
}
    
void Lexer::eat_tok() {
    assert(last_token);
    last_token = 0;
}

void Lexer::advance_character() {
    ++current_char_number;
    const u8 *new_cursor;
    current_utf32 = utf8_decode(cursor, &new_cursor);
    last_char_byte_length = new_cursor - cursor;
    assert(last_char_byte_length <= 4);
    cursor = new_cursor;
}