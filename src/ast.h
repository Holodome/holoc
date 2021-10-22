/*
Author: Holodome
Date: 12.10.2021
File: src/ast.h
Version: 0
*/
#ifndef AST_H
#define AST_H
#include "lib/general.h"

struct Memory_Arena;
struct Src_Loc;

enum {
    AST_IDENT      = 0x1, // value
    AST_STRING_LIT = 0x2,
    AST_NUMBER_LIT = 0x3,
    AST_UNARY      = 0x4, // -a
    AST_BINARY     = 0x5, // a - b
    AST_COND       = 0x6, // a ? b : c
    AST_IF         = 0x7, // if 
    AST_FOR        = 0x8, // for, while
    AST_DO         = 0x9, // do
    AST_SWITCH     = 0xA, // switch
    AST_CASE       = 0xB, // case 
    AST_BLOCK      = 0xC, // {}
    AST_GOTO       = 0xD, // goto
    AST_LABEL      = 0xE, // label: 
    AST_FUNC_CALL  = 0xF, // func(a)
    AST_CAST       = 0x10, // (int)a
    AST_MEMB       = 0x11, // a.b
    AST_RETURN     = 0x12, // return 
};

typedef struct Ast_Link {
    struct Ast_Link *next;
    struct Ast_Link *prev;    
} Ast_Link;

#define AST_FIELDS \
Ast_Link link;     \
u32 ast_kind;      \
struct Src_Loc *src_loc;  

typedef struct Ast {
    AST_FIELDS;
} Ast;

typedef struct {
    bool is_initialized;
    Ast_Link sentinel;
} Ast_List;

typedef struct {
    AST_FIELDS;
    const char *ident;
} Ast_Ident;

typedef struct Ast_Number_Lit {
    AST_FIELDS;
    u32 type;
    union {
        // @NOTE(hl): No negative numbers here
        u64         int_value;
        long double float_value;  
    };
} Ast_Number_Lit;

typedef struct Ast_String_Lit {
    AST_FIELDS;
    u32 type;
    const char *string;  
} Ast_String_Lit;

enum {
    AST_UNARY_MINUS       = 0x1, // -
    AST_UNARY_PLUS        = 0x2, // +
    AST_UNARY_LOGICAL_NOT = 0x3, // !
    AST_UNARY_NOT         = 0x4, // ~
    
    AST_UNARY_SUFFIX_INC  = 0x5, // ++
    AST_UNARY_POSTFIX_INC = 0x6, // ++
    AST_UNARY_SUFFIX_DEC  = 0x7, // --
    AST_UNARY_POSTFIX_DEC = 0x8, // --
    
    AST_UNARY_DEREF       = 0x9, // *
    AST_UNARY_ADDR        = 0xA, // &
};

typedef struct Ast_Unary {
    AST_FIELDS
    u32 kind;
    Ast *expr;
} Ast_Unary;

enum {
    AST_BINARY_ADD         = 0x1, // +
    AST_BINARY_SUB         = 0x2, // -
    AST_BINARY_MUL         = 0x3, // *
    AST_BINARY_DIV         = 0x4, // /
    AST_BINARY_MOD         = 0x5, // %
    AST_BINARY_LE          = 0x6, // <=
    AST_BINARY_L           = 0x7, // <
    AST_BINARY_GE          = 0x8, // >=
    AST_BINARY_G           = 0x9, // >
    AST_BINARY_EQ          = 0xA, // == 
    AST_BINARY_NEQ         = 0xB, // != 
    AST_BINARY_AND         = 0xC, // &
    AST_BINARY_OR          = 0xD, // |
    AST_BINARY_XOR         = 0xE, // ^
    AST_BINARY_LSHIFT      = 0xF, // <<
    AST_BINARY_RSHIFT      = 0x10, // >>
    AST_BINARY_LOGICAL_AND = 0x11, // &&
    AST_BINARY_LOGICAL_OR  = 0x12, // ||
    
    AST_BINARY_A           = 0x13, // =
    AST_BINARY_ADDA        = 0x14, // +=
    AST_BINARY_SUBA        = 0x16, // -=  
    AST_BINARY_DIVA        = 0x17, // /=  
    AST_BINARY_MULA        = 0x18, // *=  
    AST_BINARY_MODA        = 0x19, // %=  
    AST_BINARY_LSHIFTA     = 0x1A, // <<=   
    AST_BINARY_RSHIFTA     = 0x1B, // >>=   
    AST_BINARY_ANDA        = 0x1C, // &=   
    AST_BINARY_ORA         = 0x1D, // |=   
    AST_BINARY_XORA        = 0x1E, // ^=   
    
    AST_BINARY_ASSIGN      = 0x1F, // =   
    AST_BINARY_COMMA       = 0x20, // ,
};

typedef struct Ast_Binary {
    AST_FIELDS
    u32 kind;
    Ast *left;
    Ast *right;
} Ast_Binary;

typedef struct Ast_Cond {
    AST_FIELDS
    Ast *cond;
    Ast *expr;
    Ast *else_expr;
} Ast_Cond;

typedef struct Ast_If {
    AST_FIELDS
    Ast *cond;
    Ast *st;
    Ast *else_st;
} Ast_If;

typedef struct Ast_For {
    AST_FIELDS
    Ast *init;
    Ast *cond;
    Ast *inc;
} Ast_For;

typedef struct Ast_Do {
    AST_FIELDS
    Ast *st;
    Ast *cond;
} Ast_Do;

typedef struct Ast_Switch {
    AST_FIELDS
    Ast_List cases;
    struct Ast_Case *default_case;
} Ast_Switch;

typedef struct Ast_Case {
    AST_FIELDS
    Ast *expr;
    Ast *st;
} Ast_Case;

typedef struct Ast_Block {
    AST_FIELDS
    Ast_List statements;
} Ast_Block;

typedef struct Ast_Goto {
    AST_FIELDS
} Ast_Goto;

typedef struct Ast_Label {
    AST_FIELDS
} Ast_Label;

typedef struct Ast_Func_Call {
    AST_FIELDS
    Ast *callable;
    Ast_List arguments;
} Ast_Func_Call;

typedef struct Ast_Cast {
    AST_FIELDS
    struct C_Type *type;
    Ast *expr;
} Ast_Cast;

typedef struct Ast_Member {
    AST_FIELDS
    Ast *member;
} Ast_Member;

typedef struct Ast_Return {
    AST_FIELDS
    Ast *expr;
} Ast_Return;

Ast_List ast_list(void);
void ast_list_add(Ast_List *list, Ast *ast);
Ast *ast_new(struct Memory_Arena *arena, u32 kind);

#endif 