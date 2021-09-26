// Author: Holodome
// Date: 23.09.2021 
// File: pkby/src/error_reporter.h
// Version: 0
//
// Provides tools for reporting errors in whole program. Can be used to report errors 
// both inside text processing parts of the program and in the general way. 
#pragma once
#include "lib/general.h"
#include "lib/filesystem.h"

struct Token;
struct AST;

typedef struct {
    u32 line;
    u32 symb;
    FileID file;
} SrcLoc;

typedef struct {
    u32 warning_count;
    u32 error_count;
} ErrorReporter;

void report_errorv(ErrorReporter *reporter, SrcLoc src_loc, const char *msg, va_list args);
void report_error(ErrorReporter *reporter, SrcLoc src_loc, const char *msg, ...);
void report_error_tok(ErrorReporter *reporter, struct Token *token, const char *msg, ...);
void report_error_ast(ErrorReporter *reporter, struct AST *ast, const char *msg, ...);
void report_warningv(ErrorReporter *reporter, SrcLoc src_loc, const char *msg, va_list args);
void report_warning(ErrorReporter *reporter, SrcLoc src_loc, const char *msg, ...);
void report_warning_tok(ErrorReporter *reporter, struct Token *token, const char *msg, ...);
void report_warning_ast(ErrorReporter *reporter, struct AST *ast, const char *msg, ...);
b32 is_error_reported(ErrorReporter *reporter);
// Pretty printing error messages to the error stream
void report_error_generalv(const char *msg, va_list args);
void report_error_general(const char *msg, ...);