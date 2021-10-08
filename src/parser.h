// Author: Holodome
// Date: 25.09.2021 
// File: pkby/src/parser.h
// Version: 0
#pragma once 
#include "common.h"

struct Compiler_Ctx;
struct Lexer;
struct AST;

typedef struct Parser {
    struct Memory_Arena *arena;
    struct Compiler_Ctx *ctx;
    struct Lexer *lexer;
    
    u32 DBG_asts_allocated;
} Parser;

Parser *create_parser(struct Compiler_Ctx *ctx, struct Lexer *lexer);
void destroy_parser(Parser *parser);
struct AST *parser_parse_toplevel(Parser *parser);