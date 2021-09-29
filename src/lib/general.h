// Author: Holodome
// Date: 21.08.2021 
// File: pkby/src/lib/general.h
// Version: 0
//
// Provides data types and macros that are widely used throughout the program
#pragma once 
#ifndef INTERNAL_BUILD
#define INTERNAL_BUILD 1
#endif

// Check what os we have

#define OS_WINDOWS 0
#define OS_MACOS   0
#define OS_IPHONE  0
#define OS_LINUX   0

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
#undef  OS_WINDOWS
#define OS_WINDOWS 1
#elif defined(__APPLE__)
#include <TargetConditionals.h>
#if TARGET_IPHONE_SIMULATOR
#undef  OS_IPHONE
#define OS_IPHONE 1
#elif TARGET_OS_IPHONE
#undef  OS_IPHONE
#define OS_IPHONE 1
#elif TARGET_OS_MAC
#undef  OS_MACOS
#define OS_MACOS 1
#else
#error Unknown Apple platform
#endif
#elif defined(__linux__)
#undef OS_LINUX
#define OS_LINUX 1
#elif defined(__unix__)
#error !
#else
#error Unknown os
#endif

#if defined(_POSIX_VERSION)
#define OS_POSIX 1
#else
#define OS_POSIX 0
#endif 

#define COMPILER_MSVC  0
#define COMPILER_LLVM  0
#define COMPILER_GCC   0

// Detect compiler
// @NOTE(hl): Check for clang first because when compiling clang on windows, it likes to define _MSC_VER as well.
#if defined(__clang__)
#undef  COMPILER_LLVM
#define COMPILER_LLVM 1
#elif defined(_MSC_VER)
#undef  COMPILER_MSVC
#define COMPILER_MSVC 1
#elif defined(__GUNC__)
#undef  COMPILER_GCC
#define COMPILER_GCC 1
#else
#error Unsupported compiler
#endif

#include <stdint.h>
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;
typedef u32 b32;
typedef u8 b8;
// @NOTE C has strange notion of size_t and uintptr_t being different types. 
// Not to compilcate code until we need to, single type was introduced.
// @NOTE currently code assumes platform being 64 bit, but it can be changed in future
typedef uintptr_t uptr;
typedef float f32;
typedef double f64;
#define TRUE 1
#define FALSE 0
#define TO_BOOL(_expr) ((_expr) ? TRUE : FALSE)
#define ARRAY_SIZE(_a) ((uptr)(sizeof(_a) / sizeof(*(_a))))
#define CT_ASSERT(_expr) _Static_assert(_expr, "Assertion " #_expr " failed")
#define KB(_b) ((uptr)(_b) << 10)
#define MB(_b) (KB(_b) << 10)
#define IS_POW2(_n) ( ( (_n) & ((_n) - 1) ) == 0 )
#include <stdarg.h>
#define STRUCT_FIELD(_struct, _field) (((_struct *)0)->_field)
#define STRUCT_OFFSET(_struct, _field) ((uptr)((u8 *)(&STRUCT_FIELD(_struct, _field))))
#define NOT_IMPLEMENTED DBG_BREAKPOINT
#define INVALID_DEFAULT_CASE assert(!"InvalidDefaultCase")
#define UNREACHABLE assert(!"Unreachable")
// this is defined to separate developer assertions that are used by optimizer and for
// code sanity validation, and assertion calls that should be replaced with proper error handling
// so in proper code there should be no lazy asserts
#define lazy_assert assert
// Macro that should be used in unit testing system.
#define TEST_CASE(_name)

#define ATTR(...) __attribute__(__VA_ARGS__)

#include "my_assert.h"