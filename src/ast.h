/*
Author: Holodome
Date: 12.10.2021
File: src/ast.h
Version: 0
*/
#ifndef AST_H
#define AST_H
#include "lib/general.h"

#include "file_registry.h"

enum {
    AST_IDENT     = 0x1, // value
    AST_LITERAL   = 0x2, // 1, 1.0, "assa", 'a'
    AST_UNARY     = 0x3, // -a
    AST_BINARY    = 0x4, // a - b
    AST_COND      = 0x5, // a ? b : c
    AST_IF        = 0x6, // if 
    AST_FOR       = 0x7, // for, while
    AST_DO        = 0x8, // do
    AST_SWITCH    = 0x9, // switch
    AST_CASE      = 0xA, // case 
    AST_BLOCK     = 0xB, // {}
    AST_GOTO      = 0xC, // goto
    AST_LABEL     = 0xD, // label: 
    AST_FUNC_CALL = 0xE, // func(a)
    AST_ASSIGN    = 0xF, // a = b
    AST_CAST      = 0x10, // (int)a
    AST_MEMB      = 0x11, // a.b
    AST_RETURN    = 0x12, // return 
    AST_DECL      = 0x13, // int a = b;
    AST_ADDR      = 0x14, // &
    AST_DEREF     = 0x15, // *
};

#define AST_FIELDS \
u32 ast_kind;      \
Src_Loc src_loc;   \
struct Ast *next;  \
struct Ast *prev;

typedef struct Ast {
    AST_FIELDS;
} Ast;

typedef struct {
    Ast sentinel;
} Ast_List;

typedef struct {
    AST_FIELDS;
    const char *ident;
} Ast_Ident;

// enum {
    
// };

typedef struct {
    AST_FIELDS
    u32 kind;  
} Ast_Literal;

enum {
    AST_UNARY_MINUS       = 0x1, // -
    AST_UNARY_PLUS        = 0x2, // +
    AST_UNARY_LOGICAL_NOT = 0x3, // ~
    AST_UNARY_NOT         = 0x4, // !
};

typedef struct Ast_Unary {
    AST_FIELDS
    u32 kind;
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
};

typedef struct Ast_Binary {
    AST_FIELDS
} Ast_Binary;

typedef struct Ast_Cond {
    AST_FIELDS
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

typedef struct Ast_Assign {
    AST_FIELDS
    Ast *left;
    Ast *right;
} Ast_Assign;

typedef struct Ast_Cast {
    AST_FIELDS
} Ast_Cast;

typedef struct Ast_Member {
    AST_FIELDS
    Ast *member;
} Ast_Member;

typedef struct Ast_Return {
    AST_FIELDS
    Ast *expr;
} Ast_Return;

typedef struct Ast_Decl {
    AST_FIELDS
    Ast *left;
    Ast *assign;
} Ast_Decl;

typedef struct Ast_Addr {
    AST_FIELDS
    Ast *expr;
} Ast_Addr;

typedef struct Ast_Deref {
    AST_FIELDS
    Ast *expr;
} Ast_Deref;

#endif 