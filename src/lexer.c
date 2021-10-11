#include "lexer.h"

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
    ++lexer->token_stack_size;
}

static void 
pop_token_from_stack(Lexer *lexer) {
    assert(lexer->token_stack_size);
    --lexer->token_stack_size;
    Token_Stack_Entry *stack_entry = lexer->token_stack;
    STACK_ADD(lexer->token_stack_freelist, stack_entry);
    STACK_POP(lexer->token_stack);
}

static Token *
get_current_token(Lexer *lexer) {
    Token *result = 0;
    if (lexer->token_stack_size) {
        result = lexer->token_stack->token;
    }
    return result;
}

static void 
populate_unicode_buffer(Lexer *lexer) {
    lexer->unicode_buf_at = 0;
    
}

static void 
advance(Lexer *lexer, u32 n) {
    lexer->unicode_buf_size += n;
    if (lexer->unicode_buf_size >= lexer->unicode_buf_capacity) {
        populate_unicode_buffer(lexer);
    }
}

static bool
parse(Lexer *lexer, const char *lit) {
    u32 length = str_len(lit);
    bool result = true;
    for (u32 i = 0; i < length; ++i) {
        if ((u32)lit[i] != lexer->unicode_buf[lexer->unicode_buf_size + i]) {
            result = false;
            break;
        }
    }
    if (result) {
        advance(lexer, length);
    }
    return result;
}

static void 
parse_preprocessor_directive(Lexer *lexer) {
}

static inline u32 
peek_next_codepoint(Lexer *lexer) {
    u32 result = 0;
    assert(lexer->unicode_buf_size < lexer->unicode_buf_capacity);
    result = lexer->unicode_buf[lexer->unicode_buf_size];
    return result;
}

static inline void 
eat_codepoint(Lexer *lexer) {
    
}

static const char *ALPHABET       = "0123456789ABCDEF";
static const char *ALPHABET_LOWER = "0123456789abcdef";

static bool
is_symb_base(u32 symb, u32 base) {
    bool result = false;
    if (base == 2) {
        result = (symb == '0' || symb == '1');
    } else if (base == 8) {
        result = ('0' <= symb && symb <= '7');
    } else if (base == 16) {
        result = ('0' <= symb && symb <= '9') || ('A' <= symb && symb <= 'F') || ('a' <= symb && symb <= 'f');
    } else if (base == 10) {
        result = ('0' <= symb && symb <= '9');
    } else {
        UNREACHABLE;
    }
}

static u64 
symb_to_base(u32 symb, u32 base) {
    // 0110000 - 0111001 from 0 - 9
    // 0001111 mask 
    
    // 1100001 - 1111010 from  a - z
    // 1000001 - 1011010 to    A - Z 
    // 0011111 mask
    // -1 
    u64 result = 0;
    if (base == 2) {
        result = symb & 0xF;
    } else if (base == 8) {
        result = symb & 0xF;;
    } else if (base == 16) {
        if ('0' <= symb && symb <= '9') {
            result = symb & 0xF;;
        } else {
            result = (symb & 0x1F) - 1;
        }
    } else if (base == 10) {
        result = symb & 0xF;;
    } else {
        UNREACHABLE;
    }
    return result;
}

static u64 
str_to_u64_base(Lexer *lexer, u32 base) {
    u64 value = 0;
    u32 codepoint = peek_next_codepoint(lexer); 
    while (codepoint && is_symb_base(codepoint, base)) {
        value = value * base + symb_to_base(codepoint, base);
        codepoint = peek_next_codepoint(lexer);
    }
    return value;
}

static void 
parse_number(Lexer *lexer, Token *token) {
    // https://en.cppreference.com/w/c/language/integer_constant
    u32 base = 10;
    if (parse(lexer, "0x")) {
        base = 16;
    } else if (parse(lexer, "0")) {
        base = 8;
    } else if (parse(lexer, "0b")) {
        base = 2;
    };
    
    u64 value = str_to_u64_base(lexer, base);
    u32 mask = 0;
    enum { MASK_L = 0x1, MASK_LL = 0x2, MASK_U = 0x4 };
    if (parse(lexer, "LLU") || parse(lexer, "LLu") ||
        parse(lexer, "llU") || parse(lexer, "llu") ||
        parse(lexer, "ULL") || parse(lexer, "Ull") ||
        parse(lexer, "uLL") || parse(lexer, "ull")) {
        mask = MASK_LL | MASK_U;
    } else if (parse(lexer, "ll") || parse(lexer, "LL")) {
        mask = MASK_LL;
    } else if (parse(lexer, "LU") || parse(lexer, "Lu") || 
               parse(lexer, "lU") || parse(lexer, "lu")) {
        mask = MASK_L | MASK_U;
    } else if (parse(lexer, "L") || parse(lexer, "l")) {
        mask = MASK_L;
    } else if (parse(lexer, "U") || parse(lexer, "u")) {
        mask = MASK_U;
    }
    
    // Infer type
    u32 type;
    if (base == 10) {
        if (mask & (MASK_LL | MASK_U)) {
            type = C_TYPE_ULLINT;;
        } else if (mask & MASK_LL) {
            type = C_TYPE_SLLINT;
        } else if (mask & (MASK_L & MASK_U)) {
            if (value <= ULONG_MAX) {
                type = C_TYPE_ULINT;
            } else {
                type = C_TYPE_ULLINT;
            }
        } else if (mask & MASK_L) {
            if (value <= LONG_MAX) {
                type = C_TYPE_SLINT;
            } else {
                type = C_TYPE_SLLINT;
            }
        } else if (mask & MASK_U) {
            if (value <= UINT_MAX) {
                type = C_TYPE_UINT;
            } else if (value <= ULONG_MAX) {
                type = C_TYPE_ULINT;
            } else {
                type = C_TYPE_ULLINT;
            }
        } else {
            if (value <= INT_MAX) {
                type = C_TYPE_SINT;
            } else if (value <= LONG_MAX) {
                type = C_TYPE_SLINT;
            } else {
                type = C_TYPE_SLLINT;
            }
        }
    } else {
        if (mask & (MASK_LL | MASK_U)) {
            type = C_TYPE_ULLINT;;
        } else if (mask & MASK_LL) {
            if (value <= LLONG_MAX) {
                type = C_TYPE_SLLINT;    
            } else {
                type = C_TYPE_ULLINT;
            }
        } else if (mask & (MASK_L & MASK_U)) {
            if (value <= ULONG_MAX) {
                type = C_TYPE_ULINT;
            } else {
                type = C_TYPE_ULLINT;
            }
        } else if (mask & MASK_L) {
            if (value <= LONG_MAX) {
                type = C_TYPE_SLINT;
            } else if (value <= ULONG_MAX) {
                type = C_TYPE_ULINT;
            } else if (value <= LLONG_MAX) {
                type = C_TYPE_SLLINT;    
            } else {
                type = C_TYPE_ULLINT;
            }
        } else if (mask & MASK_U) {
            if (value <= UINT_MAX) {
                type = C_TYPE_UINT;
            } else if (value <= ULONG_MAX) {
                type = C_TYPE_ULINT;
            } else {
                type = C_TYPE_ULLINT;
            }
        } else {
            if (value <= INT_MAX) {
                type = C_TYPE_SINT;
            } else if (value <= UINT_MAX) {
                type = C_TYPE_UINT;
            }else if (value <= LONG_MAX) {
                type = C_TYPE_SLINT;
            } else if (value <= ULONG_MAX) {
                type = C_TYPE_ULINT;
            } else if (value <= LLONG_MAX) {
                type = C_TYPE_SLLINT;
            } else {
                type = C_TYPE_ULLINT;
            }
        }
    }
    
    token->kind = TOKEN_NUMBER;
    token->number.type = type;
    token->number.int_value = value;
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
        u32 codepoint = *lexer->unicode_buf;
        if (!codepoint) {
            token->kind = TOKEN_EOS;
            break;
        }
        
        // 'skipping' cases
        if (is_space(codepoint)) {
            advance(lexer, 1);
            continue;
        } else if (parse(lexer, "//")) {
            for (;;) {
                advance(lexer, 1);
                if (parse(lexer, "\n") || !*lexer->unicode_buf) {
                    break;
                }   
            }
            continue;
        } else if (parse(lexer, "/*")) {
            for (;;) {
                if (parse(lexer, "*/") || !*lexer->unicode_buf) {
                    break;
                }
            }
            continue;
        } else if (codepoint == '#') {
            parse_preprocessor_directive(lexer);
            continue;
        } 
        
        // all cases below should break the loop
        token->src_loc = lexer->curr_loc;
        if (is_digit(codepoint)) {
            parse_number(lexer, token);
        } else if (is_ident_start(codepoint)) {
            
            for (;;) {
                
            }
        } else if (codepoint == '\"') {
            
        } else if (is_punct(codepoint)) {
            
        } else {
            assert(false);
        }
    }
    
    return token;
}