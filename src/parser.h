#pragma once 
#include "general.h"

#include "memory.h"

#include "ast.h"

typedef struct Parser {
    MemoryArena arena;
    struct Tokenizer *tokenizer;    
    struct Interp *interp;
} Parser;

Parser create_parser(struct Tokenizer *tokenizer, struct Interp *interp);
// Return generated syntax tree for whole program
AST *parse(Parser *parser);