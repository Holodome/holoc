// Author: Holodome
// Date: 21.08.2021 
// File: pkby/src/lib/general.h
// Version: 1
//
// Provides data types and macros that are widely used throughout the program
// Basiclly it is the language layer over c 
#pragma once 
#ifndef INTERNAL_BUILD
#define INTERNAL_BUILD 1
#endif

#define OS_WINDOWS 0
#define OS_MACOS   0
#define OS_IPHONE  0
#define OS_LINUX   0
#define OS_UNIX    0

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
#undef  OS_WINDOWS
#define OS_WINDOWS 1
#elif defined(__APPLE__)
#define OS_POSIX 1
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
#define OS_POSIX 1
#undef OS_LINUX
#define OS_LINUX 1
#elif defined(__unix__)
#define OS_POSIX 1
#undef OS_UNIX 
#define OS_UNIX 1
#else
#error Unknown os
#endif

#ifndef OS_POSIX
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

#include <float.h>
#include <stdint.h>
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;
// @NOTE(hl): Used both as size_t and uintptr_t. By definition, size of this type is not enforced
// In places where size matters, sized integer types should be used.
typedef uintptr_t uptr;
typedef float f32;
typedef double f64;
#ifndef __cplusplus
typedef u8 bool;
typedef u8 bool8;
typedef u32 bool32;
typedef u64 bool64;
#define true 1
#define false 0
#endif 
#define TO_BOOL(_expr) ((_expr) ? true : false)
// @NOTE(hl): Used to get size of statically or stack-allocated array: 
// int a[] = {1, 2, 3}; int len = ARRAY_SIZE(a); // 3
#define ARRAY_SIZE(_a) ((uptr)(sizeof(_a) / sizeof(*(_a))))

#ifndef __cplusplus
#define CT_ASSERT(_expr) char __ctassert ## __LINE__ [!!(_expr)]
// #define CT_ASSERT(_expr) _Static_assert(_expr, "Assertion " #_expr " failed")
#else 
#define CT_ASSERT(_expr) static_assert(_expr, "Assertion " #_expr " failed")
#endif 

#include <stdarg.h>
#define STRUCT_FIELD(_struct, _field) (((_struct *)0)->_field)
#define STRUCT_OFFSET(_struct, _field) ((uptr)((u8 *)(&STRUCT_FIELD(_struct, _field))))

#if COMPILER_LLVM || COMPILER_GCC
#define ATTR(...) __attribute__(__VA_ARGS__)
#else 
#define ATTR(...) 
#endif 

#include "my_assert.h"