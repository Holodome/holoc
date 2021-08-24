#pragma once 
#include "general.h"

#include "memory.h"

#include "ast.h"

typedef struct Parser {
    MemoryArena arena;
    struct Tokenizer *tokenizer;    
} Parser;

Parser create_parser(struct Tokenizer *tokenizer);
// Return generated syntax tree for whole program
AST *parse(Parser *parser);