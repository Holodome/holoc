// Author: Holodome
// Date: 24.08.2021 
// File: pkby/src/interp.h
// Version: 0
//
// Up to this point it is unclear how program is going to be structured further,
// but there should be interpeter for AST, that compiles from text format to 
// some other.
#pragma once 
#include "lib/general.h"
#include "lib/files.h"
#include "tokenizer.h"
#include "ast.h"
#include "bytecode_builder.h"
#include "error_reporter.h"

typedef struct {
    MemoryArena arena;
    Tokenizer *tr;
    BytecodeBuilder *bytecode_builder;
    StringStorage ss;
    ErrorReporter er;
    
    FileID in_file_id;
    InStream file_in_st;
    
    const char *out_filename;

} Interp;

Interp *create_interp(const char *filename, const char *out_filename);
void destroy_interp(Interp *interp);
void do_interp(Interp *interp);

