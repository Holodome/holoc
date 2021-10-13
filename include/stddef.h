/*
Author: Holodome
Date: 11.10.2021
File: include/stddef.h
Version: 0
*/
#ifndef STDDEF_H
#define STDDEF_H

typedef unsigned long long int size_t;
typedef long long int ptrdiff_t;
#define NULL ((void *)0)
typedef long long int max_align_t;
typedef unsigned int wchar_t;
#define offsetof(_struct, _field) ((size_t)( &(((_struct) *)0)->(_field) ))

#endif