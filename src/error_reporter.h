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
#include "lib/stream.h"

struct Token;
struct AST;

typedef struct {
    u32 line;
    u32 symb;
    FileID file;
} Src_Loc;

typedef struct {
    Memory_Arena *arena;
    u32 warning_count;
    u32 error_count;
    Out_Stream *out;
    Out_Stream *errout;
} Error_Reporter;

Error_Reporter *create_error_reporter(Out_Stream *out, Out_Stream *errout, Memory_Arena *arena);
void report_errorv(Error_Reporter *reporter, Src_Loc src_loc, const char *msg, va_list args);
void report_error(Error_Reporter *reporter, Src_Loc src_loc, const char *msg, ...);
void report_error_tok(Error_Reporter *reporter, struct Token *token, const char *msg, ...);
void report_error_ast(Error_Reporter *reporter, struct AST *ast, const char *msg, ...);
void report_warningv(Error_Reporter *reporter, Src_Loc src_loc, const char *msg, va_list args);
void report_warning(Error_Reporter *reporter, Src_Loc src_loc, const char *msg, ...);
void report_warning_tok(Error_Reporter *reporter, struct Token *token, const char *msg, ...);
void report_warning_ast(Error_Reporter *reporter, struct AST *ast, const char *msg, ...);
bool is_error_reported(Error_Reporter *reporter);
void print_reporter_summary(Error_Reporter *reporter);
// Pretty printing error messages to the error stream
void report_error_generalv(const char *msg, va_list args);
void report_error_general(const char *msg, ...);