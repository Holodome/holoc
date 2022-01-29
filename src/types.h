/*
Author: Holodome
Date: 21.11.2021
File: src/types.h
Version: 0
*/
#ifndef TYPES_H
#define TYPES_H

// Include headers that provide necessary types
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>

// Macro used to get size of statically allocated array.
// It becomes tedious to write this construct all the time, so let's just use a macro.
#define ARRAY_SIZE(_a) (sizeof(_a) / sizeof(*(_a)))

// We want to make string a first-class citizen of program
// Where possible, strings should be used instead of c-strings
// in order to minimize conversions (and strlen count)
// There are a few places where c-strings are necessary (in lower-level API's,
// interacting with OS). In that cases writing non-terminated string to buffer
// may be a solution, but practically these places should not be exposed as
// high-level API and main part of the program should be fine using only strings
typedef struct string {
    char *data;
    uint32_t len;
} string;

typedef struct source_loc {
    string filename;
    uint32_t line;
    uint32_t col;
} source_loc;

#if 1
// Not implemented macro, useful when need to put assert(false) but
// want to distinguish it from assert(false) that guard unreachable paths
#define NOT_IMPL assert(false && "NOT IMPLEMENTED")
#define UNREACHABLE assert(false && "UNREACHABLE")
#define INVALID_DEFAULT_CASE \
    default:                 \
        UNREACHABLE;         \
        break
#else 
#define NOT_IMPL (void)0
#define UNREACHABLE (void)0
#define INVALID_DEFAULT_CASE default: break
#endif 

// This is a flag used to attach debug information to structs
// so that viewing them in debugger is more informative
#ifndef HOLOC_DEBUG
#define HOLOC_DEBUG 1
#endif

#if HOLOC_DEBUG
#define DEBUG_BREAKPOINT assert(false && "Debug breakpoint")
#else 
#define DEBUG_BREAKPOINT (void)0
#endif 

#endif
