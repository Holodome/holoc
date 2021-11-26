/*
Author: Holodome
Date: 18.11.2021
File: src/my_assert.h
Version: 0
*/
#ifndef MY_ASSERT_H
#define MY_ASSERT_H

#ifdef _MSC_VER
#define DBG_BREAKPOINT __debugbreak()
#else 
#define DBG_BREAKPOINT __builtin_trap()
#endif 

#ifndef FUNCTION_NAME
#ifdef _MSC_VER  
#define FUNCTION_NAME __FUNCTION__  
#else          
#define FUNCTION_NAME __PRETTY_FUNCTION__ 
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
#define assert(_expr) do { if (!(_expr)) { assert_msg(#_expr, __FILE__, __LINE__, FUNCTION_NAME); DBG_BREAKPOINT; } } while (0); (void)0
#else
#define assert(_expr) ASSUME(_expr)
#endif
void assert_msg(char *expr, char *filename, int line, char *function);

#define NOT_IMPLEMENTED DBG_BREAKPOINT
#define INVALID_DEFAULT_CASE default: assert("InvalidDefaultCase" && 0)
#define UNREACHABLE assert("Unreachable" && 0)

#endif