#include "parser.h"

#include "ast.h"
#include "lexer.h"
#include "type_table.h"
#include "compiler_ctx.h"


enum {
    // @NOTE(hl): Although typedef statement has special meaning from other attributes (it has its own statement)
    // it is syntactically similar to all others
    TYPE_ATTR_TYPEDEF_BIT      = 0x1,
    
    TYPE_ATTR_AUTO_BIT         = 0x2,    
    TYPE_ATTR_STATIC_BIT       = 0x4,
    TYPE_ATTR_EXTERN_BIT       = 0x8,

    TYPE_ATTR_CONST_BIT        = 0x10,
    TYPE_ATTR_VOLATILE_BIT     = 0x20,
    TYPE_ATTR_RESTRICT_BIT     = 0x40,
    TYPE_ATTR_ATOMIC_BIT       = 0x80,
    
    TYPE_ATTR_THREAD_LOCAL_BIT = 0x100,
    
    TYPE_ATTR_INLINE_BIT       = 0x200,
    TYPE_ATTR_NORETURN_BIT     = 0x400,
};

typedef struct {
    u32 flags;
    u32 align; // if _Alignof specified
} Type_Attributes;

static C_Type *
parse_struct(Parser *parser) {
    Token *token = peek_tok(parser->lex);
    assert(IS_KW(token, KEYWORD_STRUCT));
    eat_tok(parser->lex);
    C_Type *type = 0;
    if (IS_PUNCT(token, '{')) {
        type = create_type();
        parse_struct_members(parser, type);
    } else if (token->kind == TOKEN_IDENT) {
        const char *type_name = token->ident.str;
        // type = tt_get_type(parser->ctx->tt, C_TYPE_TAG_STRUCT, )
    } else {
        NOT_IMPLEMENTED;
    }
    NOT_IMPLEMENTED;
    return type;
}

static C_Type *
declspec_global_scope(Parser *parser, Type_Attributes *attr) {
    C_Type *type = tt_get_standard_type(parser->ctx->tt, C_TYPE_SINT);
    
    Token *token = peek_tok(parser->lex);
    // @NOTE(hl): The only type keyword that can occur multiple times in long, and we reserve 2 bits for it
    //  more proper way would be to count occurence of each keyword do to all error checkign after parsing 
    enum {  
        TYPE_BIT_INT      = 0x1,  
        TYPE_BIT_LONG     = 0x2,
        TYPE_BIT_SHORT    = 0x8,
        TYPE_BIT_CHAR     = 0x10,
        TYPE_BIT_VOID     = 0x20,
        TYPE_BIT_BOOL     = 0x40,
        TYPE_BIT_FLOAT    = 0x80,
        TYPE_BIT_DOUBLE   = 0x100,
        TYPE_BIT_SIGNED   = 0x200,
        TYPE_BIT_UNSIGNED = 0x400,
    }; 
    u32 type_bits = 0;
    // @NOTE(hl): All attributes can be parsed order-idependent
    for (;;) {
        switch (token->kind) {
        case TOKEN_KEYWORD: {
            switch (token->kw) {
            case KEYWORD_TYPEDEF: {
                assert(!(attr->flags & TYPE_ATTR_TYPEDEF_BIT));
                attr->flags |= TYPE_ATTR_TYPEDEF_BIT;
            } break;
            case KEYWORD_AUTO: {
                assert(!(attr->flags & TYPE_ATTR_AUTO_BIT));
                attr->flags |= TYPE_ATTR_AUTO_BIT;
            } break;
            case KEYWORD_STATIC: {
                assert(!(attr->flags & TYPE_ATTR_STATIC_BIT));
                attr->flags |= TYPE_ATTR_STATIC_BIT;
            } break;
            case KEYWORD_EXTERN: {
                assert(!(attr->flags & TYPE_ATTR_EXTERN_BIT));
                attr->flags |= TYPE_ATTR_EXTERN_BIT;
            } break;
            case KEYWORD_CONST: {
                assert(!(attr->flags & TYPE_ATTR_CONST_BIT));
                attr->flags |= TYPE_ATTR_CONST_BIT;
            } break;
            case KEYWORD_VOLATILE: {
                assert(!(attr->flags & TYPE_ATTR_VOLATILE_BIT));
                attr->flags |= TYPE_ATTR_VOLATILE_BIT;
            } break;
            case KEYWORD_RESTRICT: {
                assert(!(attr->flags & TYPE_ATTR_RESTRICT_BIT));
                attr->flags |= TYPE_ATTR_RESTRICT_BIT;
            } break;
            case KEYWORD_ATOMIC: {
                assert(!(attr->flags & TYPE_ATTR_ATOMIC_BIT));
                attr->flags |= TYPE_ATTR_ATOMIC_BIT;
            } break;
            case KEYWORD_THREAD_LOCAL: {
                assert(!(attr->flags & TYPE_ATTR_THREAD_LOCAL_BIT));
                attr->flags |= TYPE_ATTR_THREAD_LOCAL_BIT;
            } break;
            case KEYWORD_INLINE: {
                assert(!(attr->flags & TYPE_ATTR_INLINE_BIT));
                attr->flags |= TYPE_ATTR_INLINE_BIT;
            } break;
            case KEYWORD_NORETURN: {
                assert(!(attr->flags & TYPE_ATTR_NORETURN_BIT));
                attr->flags |= TYPE_ATTR_NORETURN_BIT;
            } break;
            
            case KEYWORD_INT: {
                assert(!(type_bits & TYPE_BIT_INT));
                type_bits |= TYPE_BIT_INT;
            } break;
            case KEYWORD_LONG: {
                assert( !( type_bits & (TYPE_BIT_LONG | (TYPE_BIT_LONG << 1)) ) );
                type_bits += TYPE_BIT_LONG;
            } break;
            case KEYWORD_SIGNED: {
                assert(!(type_bits & TYPE_BIT_SIGNED));
                type_bits |= TYPE_BIT_SIGNED;
            } break;
            case KEYWORD_UNSIGNED: {
                assert(!(type_bits & TYPE_BIT_UNSIGNED));
                type_bits |= TYPE_BIT_UNSIGNED;
            } break;
            case KEYWORD_CHAR: {
                assert(!(type_bits & TYPE_BIT_CHAR));
                type_bits |= TYPE_BIT_CHAR;
            } break;
            case KEYWORD_SHORT: {
                assert(!(type_bits & TYPE_BIT_SHORT));
                type_bits |= TYPE_BIT_SHORT;
            } break;
            case KEYWORD_BOOL: {
                assert(!(type_bits & TYPE_BIT_BOOL));
                type_bits |= TYPE_BIT_BOOL;
            } break;
            case KEYWORD_FLOAT: {
                assert(!(type_bits & TYPE_BIT_FLOAT));
                type_bits |= TYPE_BIT_FLOAT;
            } break;
            case KEYWORD_DOUBLE: {
                assert(!(type_bits & TYPE_BIT_DOUBLE));
                type_bits |= TYPE_BIT_DOUBLE;
            } break;
            
            case KEYWORD_STRUCT: {
                NOT_IMPLEMENTED;
            } break;
            case KEYWORD_UNION: {
                 NOT_IMPLEMENTED;
            } break;
            case KEYWORD_ENUM: {
                NOT_IMPLEMENTED;
            } break;
            
            default: {
                goto validate;
            }
            }    
        } break;
        
        }
        
        switch (type_bits) {
        case 0: break;
        case TYPE_BIT_INT:
        case TYPE_BIT_SIGNED:
        case TYPE_BIT_SIGNED + TYPE_BIT_INT: {
            type = get_standard_type(C_TYPE_SINT);
        } break;
        case TYPE_BIT_UNSIGNED:
        case TYPE_BIT_UNSIGNED + TYPE_BIT_INT: {
            type = get_standard_type(C_TYPE_UINT);
        } break;
        case TYPE_BIT_LONG:
        case TYPE_BIT_LONG + TYPE_BIT_INT:
        case TYPE_BIT_SIGNED + TYPE_BIT_LONG + TYPE_BIT_INT: {
            type = get_standard_type(C_TYPE_SLINT);
        } break;
        case TYPE_BIT_UNSIGNED + TYPE_BIT_LONG:
        case TYPE_BIT_UNSIGNED + TYPE_BIT_LONG + TYPE_BIT_INT: {
            type = get_standard_type(C_TYPE_SLINT);
        } break;
        case TYPE_BIT_LONG + TYPE_BIT_LONG:
        case TYPE_BIT_LONG + TYPE_BIT_LONG + TYPE_BIT_INT:
        case TYPE_BIT_SIGNED + TYPE_BIT_LONG + TYPE_BIT_LONG:
        case TYPE_BIT_SIGNED + TYPE_BIT_LONG + TYPE_BIT_LONG + TYPE_BIT_INT: {
            type = get_standard_type(C_TYPE_SLLINT);
        } break;
        case TYPE_BIT_UNSIGNED + TYPE_BIT_LONG + TYPE_BIT_LONG:
        case TYPE_BIT_UNSIGNED + TYPE_BIT_LONG + TYPE_BIT_LONG + TYPE_BIT_INT: {
            type = get_standard_type(C_TYPE_ULLINT);
        } break;
        case TYPE_BIT_SHORT: 
        case TYPE_BIT_SHORT + TYPE_BIT_INT: 
        case TYPE_BIT_SIGNED + TYPE_BIT_SHORT: 
        case TYPE_BIT_SIGNED + TYPE_BIT_SHORT + TYPE_BIT_INT: {
            type = get_standard_type(C_TYPE_SSINT);
        } break;
        case TYPE_BIT_UNSIGNED + TYPE_BIT_SHORT: 
        case TYPE_BIT_UNSIGNED + TYPE_BIT_SHORT + TYPE_BIT_INT: {
            type = get_standard_type(C_TYPE_SSINT);
        } break;
        case TYPE_BIT_CHAR: {
            type = get_standard_type(C_TYPE_CHAR);
        } break;
        case TYPE_BIT_SIGNED + TYPE_BIT_CHAR: {
            type = get_standard_type(C_TYPE_SCHAR);
        } break;
        case TYPE_BIT_UNSIGNED + TYPE_BIT_CHAR: {
            type = get_standard_type(C_TYPE_UCHAR);
        } break;
        case TYPE_BIT_FLOAT: {
            type = get_standard_type(C_TYPE_FLOAT);
        } break;
        case TYPE_BIT_DOUBLE: {
            type = get_standard_type(C_TYPE_DOUBLE);
        } break;
        case TYPE_BIT_LONG + TYPE_BIT_DOUBLE: {
            type = get_standard_type(C_TYPE_LDOUBLE);
        } break;
        case TYPE_BIT_BOOL: {
            type = get_standard_type(C_TYPE_BOOL);
        } break;
        case TYPE_BIT_VOID: {
            type = get_standard_type(C_TYPE_VOID);
        } break;
        default: {
            NOT_IMPLEMENTED;
        }
        
        
    }

validate:

    return type;
}

struct Ast *
parse_toplevel(Parser *parser) {
    Token *token = peek_tok(parser->lex);
    while (token->kind != TOKEN_EOS) {
        declspec_global_scope();
    }
}