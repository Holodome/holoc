// Author: Holodome
// Date: 25.09.2021 
// File: pkby/src/parser.h
// Version: 0
#pragma once 
#include "lib/general.h"
#include "error_reporter.h"
#include "string_storage.h"
#include "tokenizer.h"
#include "ast.h"

typedef struct {
    MemoryArena arena;
    StringStorage *ss;
    ErrorReporter *er;
    Tokenizer *tr;
} Parser;

Parser *create_parser(Tokenizer *tokenizer, StringStorage *ss, ErrorReporter *er);
void destroy_parser(Parser *parser);
AST *parser_parse_toplevel(Parser *parser);