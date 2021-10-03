// Author: Holodome
// Date: 25.09.2021 
// File: pkby/src/parser.h
// Version: 0
#pragma once 
#include "lib/general.h"
#include "compiler_ctx.h"
#include "lexer.h"
#include "ast.h"

typedef struct {
    MemoryArena arena;
    CompilerCtx *ctx;
    Lexer *lexer;
    
    u32 DBG_asts_allocated;
} Parser;

Parser *create_parser(CompilerCtx *ctx, Lexer *lexer);
void destroy_parser(Parser *parser);
AST *parser_parse_toplevel(Parser *parser);