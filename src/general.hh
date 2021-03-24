#if !defined(GENERAL_HH)

#define CT_ASSERT(_expr) static_assert(_expr, "Assertion " #_expr " failed")

// Detect compiler
// Check for clang first because when compiling clang on windows, it likes to define _MSC_VER as well.
#if defined(__clang__)
#define COMPILER_LLVM 1
#elif defined(_MSC_VER)
#define COMPILER_MSVC 1
#elif defined(__GUNC__)
#define COMPILER_GCC 1
#else
#error Unsupported compiler
#endif

#ifndef COMPILER_MSVC
#define COMPILER_MSVC 0
#endif 
#ifndef COMPILER_LLVM
#define COMPILER_LLVM 0
#endif 
#ifndef COMPILER_GCC
#define COMPILER_GCC 0
#endif 

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdbool.h>
#include <assert.h>
#include <math.h>
#include <string.h>
#include <float.h>
#include <stdarg.h>
#include <ctype.h>
#include <locale.h>

typedef int8_t  i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef float  f32;
typedef double f64;

#define ARRAY_SIZE(_a) ((size_t)(sizeof(_a) / sizeof(*(_a))))

#define BYTES(_n) ((size_t)_n)
#define KILOBYTES(_n) (BYTES(_n) << 10) 
#define MEGABYTES(_n) (KILOBYTES(_n) << 10) 

#define GENERAL_HH 1
#endif
