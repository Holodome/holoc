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
    buffer += Str::format(buffer, buffer_size, "<Token kind=%s", token_kind_to_str(kind));
    buffer_size -= buffer - buffer_init;
    if ((u32)kind > 0xFF) {
        switch(kind) {
            case TokenKind::Identifier: {
                buffer += Str::format(buffer, buffer_size, ", ident='%s'", ident.data);
                buffer_size -= buffer - buffer_init;
            } break;
            case TokenKind::Integer: {
                buffer += Str::format(buffer, buffer_size, ", int=%lli", integer);
                buffer_size -= buffer - buffer_init;
            } break;
            case TokenKind::String: {
                buffer += Str::format(buffer, buffer_size, ", str='%s'", ident.data);
                buffer_size -= buffer - buffer_init;
            } break;
            default: {
                assert(false);  
            };
        }
    } 
    buffer += Str::format(buffer, buffer_size, ">");
    buffer_size -= buffer - buffer_init;
    
    return buffer - buffer_init;
}
 
void Lexer::init(const Str &filename, const void *buffer, size_t buffer_size) {
    this->filename = filename;
    file_data = (const u8 *)buffer;
    file_size = buffer_size;
    cursor = file_data;
}

Token *Lexer::peek_tok() {
    if (last_token) {
        return last_token;
    }   

    Token *token = new Token;
    
    last_token = token;
    for (;;) {
        u32 curc = *cursor;
        if (!curc) {
            token->kind = TokenKind::EOS;
            break;
        }

        // Skip and continue
        if (curc == '\n') {
            ++current_line_number;
            current_character_index = 0;
            ++cursor;
            continue;
        } else if (isspace(curc)) {
            ++cursor;
            continue;
        } else if (curc == '#') {
            while (*cursor != '\n') {
                ++cursor;
            }
            continue;
        } 
        
        // Parse and break
        token->cursor_at_tok = cursor;
        token->line_number = current_line_number;
        if (isdigit(curc)) {
            i64 value = 0;
            while (isdigit(*cursor)) {
                value = value * 10 + (*cursor - '0');
                ++cursor;
            }
            token->kind = TokenKind::Integer;
            token->integer = value;
            break;
        } else if (isalpha(curc)) {
            const u8 *ident_end = cursor;
            do {
                ++ident_end;
            } while(isalpha(*ident_end) || isdigit(*ident_end) || *ident_end == '_');
            token->ident = Str((const char *)cursor, 0u, ident_end - cursor);
            cursor = ident_end;
            token->kind = TokenKind::Identifier;
            break;
        } else if (ispunct(curc)) {
            token->kind = (TokenKind)curc;
            ++cursor;
            break;
        } else {
            error_at(filename.data, (char *)file_data, (char *)cursor, "Invalid token '%lc'", curc);
        }
    }

    return token;
}
    
void Lexer::eat_tok() {
    assert(last_token);
    last_token = 0;
}