//
// interp.h
// 
// Up to this point it is unclear how program is going to be structured further,
// but there should be interpeter for AST, that compiles from text format to 
// some other.
//
// @NOTE There has been made a decision that error handling should not be done with expections 
// (no setjmp/longjmp). Error codes are used instead.
// This way we can safely continue program execution after some kind of unexpected behaviour.
// If this behaviour is code-related, we could even skip the problematic part until interpeter finds
// something it can understand and continue form there. This way program could e able to provide user 
// with multiple error messages, instead of one at a time.
#pragma once 
#include "general.h"
#include "filesystem.h"
#include "tokenizer.h"
#include "ast.h"
#include "bytecode.h"

typedef struct Interp {
    MemoryArena arena;
    Tokenizer *tokenizer;
    BytecodeBuilder *bytecode_builder;
    
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