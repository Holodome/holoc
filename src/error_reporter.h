/*
Author: Holodome
Date: 12.10.2021
File: src/error_reporter.h
Version: 0
*/
#ifndef ERROR_REPORTER_H
#define ERROR_REPORTER_H
#include "lib/general.h"

#include "file_registry.h" // Src_Loc

struct Token;
struct Ast;
struct Compiler_Ctx;

typedef struct Error_Reporter {
    struct Compiler_Ctx *ctx;
    u32 warning_count;
    u32 error_count;
} Error_Reporter;

void report_errorv(Error_Reporter *reporter, const Src_Loc *src_loc, const char *msg, va_list args);
void report_error(Error_Reporter *reporter, const Src_Loc *src_loc, const char *msg, ...);
void report_error_tok(Error_Reporter *reporter, struct Token *token, const char *msg, ...);
void report_error_ast(Error_Reporter *reporter, struct Ast *ast, const char *msg, ...);
bool is_error_reported(Error_Reporter *reporter);
#endif
