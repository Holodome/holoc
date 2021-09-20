#pragma once 
#include "general.h"
#include "filesystem.h"
#include "tokenizer.h"
#include "ast.h"

typedef struct Interp {
    MemoryArena arena;
    Tokenizer *tokenizer;
    
    FileHandle file_id;
    InStream file_in_st;
    
    b32 reported_error;
} Interp;

Interp create_interp(const char *filename);
void do_interp(Interp *interp);
void report_error(Interp *interp, const char *msg, ...);
void report_error_at(Interp *interp, SourceLocation source_loc, const char *msg, ...);
void report_error_tok(Interp *interp, Token *token, const char *msg, ...);
void report_error_ast(Interp *interp, AST *ast, const char *msg, ...);