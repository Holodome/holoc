//
// error_reporter.h
//
// Provides tools for reporting errors in whole program. Can be used to report errors 
// both inside text processing parts of the program and in the general way. 
#pragma once
#include "general.h"
#include "tokenizer.h"
#include "ast.h"

typedef struct {
    MemoryArena arena;
    const char *filename;
    
    u32 warning_count;
    u32 error_count;
} ErrorReporter;

ErrorReporter *create_error_reporter(const char *filename);
void destroy_error_reporter(ErrorReporter *reporter);

void report_errorv(ErrorReporter *reporter, SrcLoc src_loc, const char *msg, va_list args);
void report_error(ErrorReporter *reporter, SrcLoc src_loc, const char *msg, ...);
void report_error_tok(ErrorReporter *reporter, Token *token, const char *msg, ...);
void report_error_ast(ErrorReporter *reporter, AST *ast, const char *msg, ...);
void report_warningv(ErrorReporter *reporter, SrcLoc src_loc, const char *msg, va_list args);
void report_warning(ErrorReporter *reporter, SrcLoc src_loc, const char *msg, ...);
void report_warning_tok(ErrorReporter *reporter, Token *token, const char *msg, ...);
b32 is_error_reported(ErrorReporter *reporter);
// Preyyt printing error messages to the error stream
void report_error_generalv(const char *msg, va_list args);
void report_error_general(const char *msg, ...);