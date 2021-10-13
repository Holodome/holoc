/*
Author: Holodome
Date: 11.10.2021
File: include/stdint.h
Version: 0
*/
#ifndef STDINT_H
#define STDINT_H

typedef signed char  int8_t;
typedef signed short int16_t;
typedef signed int   int32_t;
typedef signed long  int64_t;

typedef int8_t  int_fast8_t;
typedef int16_t int_fast16_t;
typedef int32_t int_fast32_t;
typedef int64_t int_fast64_t;

typedef int8_t  int_least8_t;
typedef int16_t int_least16_t;
typedef int32_t int_least32_t;
typedef int64_t int_least64_t;

typedef int64_t intmax_t;
typedef int64_t intptr_t;

typedef unsigned char  uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int   uint32_t;
typedef unsigned long  uint64_t;

typedef uint8_t  uint_fast8_t;
typedef uint16_t uint_fast16_t;
typedef uint32_t uint_fast32_t;
typedef uint64_t uint_fast64_t;

typedef uint8_t  uint_least8_t;
typedef uint16_t uint_least16_t;
typedef uint32_t uint_least32_t;
typedef uint64_t uint_least64_t;

typedef uint64_t uintmax_t;
typedef uint64_t uintptr_t;

#include <limits.h>

#define INT8_WIDTH  8
#define INT16_WIDTH 16
#define INT32_WIDTH 32
#define INT64_WIDTH 64

#define INT_FAST8_WIDTH  8
#define INT_FAST16_WIDTH 16
#define INT_FAST32_WIDTH 32
#define INT_FAST64_WIDTH 64

#define INT_LEAST8_WIDTH  8
#define INT_LEAST16_WIDTH 16
#define INT_LEAST32_WIDTH 32
#define INT_LEAST64_WIDTH 64

#define INTPTR_WIDTH 64
#define INTMAX_WIDTH 64

#define INT8_MIN  SCHAR_MIN
#define INT16_MIN SHRT_MIN 
#define INT32_MIN INT_MIN
#define INT64_MIN LONG_MIN

#define INT_FAST8_MIN  INT8_MIN
#define INT_FAST16_MIN INT16_MIN 
#define INT_FAST32_MIN INT32_MIN
#define INT_FAST64_MIN INT64_MIN

#define INT_LEAST8_MIN  INT8_MIN
#define INT_LEAST16_MIN INT16_MIN 
#define INT_LEAST32_MIN INT32_MIN
#define INT_LEAST64_MIN INT64_MIN

#define INTPTR_MIN INT64_MIN
#define INTMAX_MIN INT64_MIN

#define INT8_MAX  SCHAR_MAX
#define INT16_MAX SHRT_MAX 
#define INT32_MAX INT_MAX
#define INT64_MAX LONG_MAX

#define INT_FAST8_MAX  INT8_MAX
#define INT_FAST16_MAX INT16_MAX 
#define INT_FAST32_MAX INT32_MAX
#define INT_FAST64_MAX INT64_MAX

#define INT_LEAST8_MAX  INT8_MAX
#define INT_LEAST16_MAX INT16_MAX 
#define INT_LEAST32_MAX INT32_MAX
#define INT_LEAST64_MAX INT64_MAX

#define INTPTR_MAX INT64_MAX
#define INTMAX_MAX INT64_MAX

#define UINT8_WIDTH  8
#define UINT16_WIDTH 16
#define UINT32_WIDTH 32
#define UINT64_WIDTH 64

#define UINT_FAST8_WIDTH  8
#define UINT_FAST16_WIDTH 16
#define UINT_FAST32_WIDTH 32
#define UINT_FAST64_WIDTH 64

#define UINT_LEAST8_WIDTH  8
#define UINT_LEAST16_WIDTH 16
#define UINT_LEAST32_WIDTH 32
#define UINT_LEAST64_WIDTH 64

#define UINTPTR_WIDTH 64
#define UINTMAX_WIDTH 64


#define UINT8_MAX  UCHAR_MAX
#define UINT16_MAX USHRT_MAX 
#define UINT32_MAX UINT_MAX
#define UINT64_MAX ULONG_MAX

#define UINT_FAST8_MAX  UINT8_MAX
#define UINT_FAST16_MAX UINT16_MAX 
#define UINT_FAST32_MAX UINT32_MAX
#define UINT_FAST64_MAX UINT64_MAX

#define UINT_LEAST8_MAX  UINT8_MAX
#define UINT_LEAST16_MAX UINT16_MAX 
#define UINT_LEAST32_MAX UINT32_MAX
#define UINT_LEAST64_MAX UINT64_MAX

#define INT8_C(_n)  (_n)
#define INT16_C(_n) (_n)
#define INT32_C(_n) (_n)
#define INT64_C(_n) (_n##ll)

#define UINT8_C(_n)  (_n##u)
#define UINT16_C(_n) (_n##u)
#define UINT32_C(_n) (_n##u)
#define UINT64_C(_n) (_n##llu)

#define INTMAX_C  INT64_C
#define UINTMAX_C UINT64_C

#endif