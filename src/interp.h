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
#include "files.h"
#include "tokenizer.h"
#include "ast.h"
#include "bytecode_builder.h"
#include "error_reporter.h"

typedef struct {
    MemoryArena arena;
    Tokenizer *tokenizer;
    BytecodeBuilder *bytecode_builder;
    ErrorReporter *error_reporter;
    
    const char *out_file;
    FileHandle file_id;
    InStream file_in_st;
} Interp;

Interp *create_interp(const char *filename, const char *out_filename);
void destroy_interp(Interp *interp);
void do_interp(Interp *interp);

