#include "new_lexer.h"

#include "lib/memory.h"
#include "lib/lists.h"
#include "lib/strings.h"

#define ITER_KEYWORDS(_it) \
for ((_it) = KEYWORD_AUTO; (_it) < KEYWORD_SENTINEL; ++(_it))
#define ITER_PREPROCESSOR_KEYWORD(_it) \
for ((_it) = KEYWORD_P_DEFINE; (_it) < KEYWORD_P_SENTINEL; ++(_it))
#define ITER_OPERATORS(_it) \
for ((_it) = OPERATOR_IADD; (_it) < OPERATOR_SENTINEL; ++(_it))

static const char *KEYWORD_STRINGS[] = {
    "(unknown)",
    "auto",
    "break",
    "case",
    "char",
    "const",
    "continue",
    "default",
    "do",
    "double",
    "else ",
    "enum ",
    "extern",
    "float",
    "for ",
    "goto",
    "if",
    "inline",
    "int",
    "long",
    "register",
    "restrict",
    "return",
    "short",
    "signed",
    "unsigned ",
    "static ",
    "struct",
    "switch ",
    "typedef",
    "union ",
    "unsigned",
    "void",
    "volatile",
    "while",
    "_Alignas",
    "_Alignof",
    "_Atomic",
    "_Bool",
    "_Complex",
    "_Decimal128 ",
    "_Decimal32",
    "_Decimal64",
    "_Generic",
    "_Imaginary",
    "_Noreturn",
    "_Static_assert",
    "_Thread_local",
    "_Pragma",
};

static const char *PREPROCESSOR_KEYWORD_STRINGS[] = {
    "(unknown)",
    "define",
    "undef",
    "include",
    "if",
    "ifdef",
    "ifndef",
    "else",
    "elifdef",
    "elifndef",
    "pragma",
};

static const char *OPERATOR_STRINGS[] = {
    "(unknown)",
    ">>=",
    "<<=", 
    "+=",
    "-=",
    "*=",
    "/=",
    "%=", 
    "&=",
    "|=",
    "^=",
    "++",
    "--",
    ">>",
    "<<",
    "&&",
    "||",
    "==",
    "!=", 
    "<=", 
    ">=", 
    "->",
};

// static Token *
// peek_forward(Lexer *lexer, u32 n) {
//     Token *token = 0;
//     if (n < lexer->stack_size) {
//         Token_Stack_Entry *entry = lexer->token_stack;
//         while (n--) {
//              entry = entry->next;
//         }
//         assert(entry);
//         token = entry->token;
//     }
    
//     return token;
// }

static Token_Stack_Entry *
get_stack_entry(Lexer *lexer) {
    Token_Stack_Entry *entry = lexer->token_stack;
    if (!entry) {
        entry = arena_alloc_struct(lexer->arena, Token_Stack_Entry);
    } else {
        entry->token = 0;
    }
    return entry;
}

static void 
add_token_to_stack(Lexer *lexer, Token *token) {
    Token_Stack_Entry *stack_entry = get_stack_entry(lexer);
    stack_entry->token = token;
    STACK_ADD(lexer->token_stack, stack_entry);
    ++lexer->stack_size;
}

static void 
pop_token_from_stack(Lexer *lexer) {
    assert(lexer->stack_size);
    --lexer->stack_size;
    Token_Stack_Entry *stack_entry = lexer->token_stack;
    STACK_ADD(lexer->token_stack_freelist, stack_entry);
    STACK_POP(lexer->token_stack);
}

static Token *
get_current_token(Lexer *lexer) {
    Token *result = 0;
    if (lexer->stack_size) {
        result = lexer->token_stack->token;
    }
    return result;
}

static void 
advance(Lexer *lexer, u32 n) {
    u32 line_idx = lexer->curr_loc.symb;
    for (u32 i = 0; i < n; ++i) {
        u32 codepoint = 0;
        lexer->cursor = (u8 *)utf8_decode(lexer->cursor, &codepoint);
        if (codepoint == '\n') {
            ++lexer->curr_loc.line;
            line_idx = 0;
        }
        ++line_idx;
    }
    lexer->curr_loc.symb = line_idx;
}

static bool
parse(Lexer *lexer, const char *lit) {
    bool result = false;
    u32 lit_len = str_len(lit);  
    u64 new_cursor = lexer->cursor - lexer->bf + lit_len;
    if (new_cursor <= lexer->bf_sz) {
        result = mem_eq(lexer->cursor, lit, lit_len);
        if (result) {
            advance(lexer, lit_len);
        }
    }
    return result;
}

static void 
parse_preprocessor_directive(Lexer *lexer) {
    u32 keyword_enum;
    u32 index = 0;
    ITER_PREPROCESSOR_KEYWORD(keyword_enum) {
        ++index;
        
        if (parse(lexer, PREPROCESSOR_KEYWORD_STRINGS[index])) {
            
        }
    }
}

Token *
peek_tok(Lexer *lexer) {
    Token *token = get_current_token(lexer);
    if (token) {
        return token;
    }
    
    token = arena_alloc_struct(lexer->arena, Token);
    add_token_to_stack(lexer, token);
    
    for (;;) {
        u32 codepoint;
        utf8_decode(lexer->cursor, &codepoint);
        if (!codepoint) {
            token->kind = TOKEN_EOS;
            break;
        }
        
        if (is_space(codepoint)) {
            advance(lexer, 1);
            continue;
        } else if (parse(lexer, "//")) {
            for (;;) {
                lexer->cursor = (u8 *)utf8_decode(lexer->cursor, &codepoint);
            }
            continue;
        } else if (parse(lexer, "/*")) {
            while (codepoint) {
                lexer->cursor = (u8 *)utf8_decode(lexer->cursor, &codepoint);
                if (parse(lexer, "*/")) {
                    break;
                }
            }
            continue;
        }
        
        token->src_loc = lexer->curr_loc;
        if (codepoint == '#') {
            advance(lexer, 1);
            parse_preprocessor_directive(lexer);
            continue;
        } else if (is_digit(codepoint)) {
            
        } else if (is_ident_start(codepoint)) {
            
        } else if (codepoint == '\"') {
            
        } else if (is_punct(codepoint)) {
            
        } else {
            assert(false);
        }
    }
    
    return token;
}