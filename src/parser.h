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