/*
Author: Holodome
Date: 18.10.2021
File: src/parser.h
Version: 0
*/
#ifndef PARSER_H
#define PARSER_H

#include "lib/general.h"

#define SCOPE_VARIABLE_HASH_SIZE 1024
#define SCOPE_TAG_HASH_SIZE      256

struct Compiler_Ctx;
struct Ast;
struct Lexer;
struct C_Type;

typedef struct C_Struct_Member_Link {
    struct C_Struct_Member_Link *next;
    struct C_Struct_Member_Link *prev;
} C_Struct_Member_Link;

typedef struct C_Struct_Member {
    C_Struct_Member_Link link;
    const char *name;
    struct Src_Loc *decl_loc;
    
    struct C_Type *type;
    u32 index;  // index in structure
    u32 align;  // align 
    u32 offset; // byte offset
} C_Struct_Member;

typedef struct C_Type_Link {
    struct C_Type_Link *next;
    struct C_Type_Link *prev;
} C_Type_Link;

typedef struct C_Type {
    C_Type_Link link;
    u32 align;
    u32 size;  // value returned by sizeof operator
    
    u32 kind;
    struct Src_Loc *decl_loc;
    union {
        struct C_Type *ptr_to;
        struct {
            struct C_Type *array_base;
            u64 array_size;  
        };
        struct {
            bool is_variadic;
            struct C_Type *return_type;
            C_Type_Link param_sentinel;
            u32 param_count;
        };
        struct {
            bool is_packed;
            u32 member_count;   
            C_Struct_Member_Link member_sentinel;
        };
    };
} C_Type;

// Variable is anyhting scoped - type, function, enum values
typedef union {
    i32     enum_value;
    C_Type *type;
    
} Parser_Var;

typedef struct Parser_Obj {
    const char *name;
    struct C_Type *type;
    struct Src_Loc *declared_at;
} Parser_Obj;

typedef struct Parser_Scope {
    
    
    struct Parser_Scope *next;
} Parser_Scope;

typedef struct Parser {
    C_Type standard_types[0x16];
    
    struct Compiler_Ctx *ctx;
    struct Lexer *lex;    
} Parser;

Parser *create_parser(struct Compiler_Ctx *ctx);
struct Ast *parse_toplevel(Parser *parser);

#endif