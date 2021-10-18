/*
Author: Holodome
Date: 18.10.2021
File: src/parser.h
Version: 0
*/
#ifndef PARSER_H
#define PARSER_H

#include "lib/general.h"

struct Memory_Arena;

typedef struct {
    bool is_default;
    
} Type;

typedef struct {
    
} Variable;

typedef struct Parser {
    struct Memory_Arena *arena;
    
    
} Parser;

void push_scope(Parser *parser);
void pop_scope(Parser *parser);



#endif