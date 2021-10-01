// Author: Holodome
// Date: 29.09.2021 
// File: pkby/src/lib/files.h
// Version: 0
// 
// Assert implementation. Debug information on macos with arm processors is not generated correctly,
// and standard assert macro does not do brekpoint in correct place
#pragma once

#ifdef _MSC_VER
#define DBG_BREAKPOINT __debugbreak()
#else 
#define DBG_BREAKPOINT __builtin_trap()
#endif 

#ifndef __FUNCTION_NAME__
#ifdef WIN32   //WINDOWS
#define __FUNCTION_NAME__   __FUNCTION__  
#else          //*NIX
#define __FUNCTION_NAME__   __PRETTY_FUNCTION__ 
#endif
#endif

#ifdef _MSC_VER
#define ASSUME(_expr) __assume(_expr)
#else 
#define ASSUME(_expr) __builtin_assume(_expr)
#endif

#ifdef assert
#undef assert
#endif

#if INTERNAL_BUILD
#define assert(_expr) do { if (!(_expr)) { assert_msg(#_expr, __FILE__, __LINE__, __FUNCTION_NAME__); DBG_BREAKPOINT; } } while (0); (void)0
#else
#define assert(_expr) ASSUME(_expr)
#endif
void assert_msg(const char *expr, const char *filename, int line, const char *function);

#define NOT_IMPLEMENTED DBG_BREAKPOINT
#define INVALID_DEFAULT_CASE assert(!"InvalidDefaultCase")
#define UNREACHABLE assert(!"Unreachable")