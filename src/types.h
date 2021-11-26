/*
Author: Holodome
Date: 21.11.2021
File: src/types.h
Version: 0
*/
#ifndef TYPES_H
#define TYPES_H

#include <stdint.h>
#include <stdarg.h>
#include <stdbool.h>

#ifdef INTERNAL_BUILD
#define STR_CREATED_AT_INFO
#endif

typedef struct string {
    char *data;
    uint32_t    len;
#ifdef STR_CREATED_AT_INFO
    char *created_at;
#endif 
} string;

// Structure used for type-safe storing of hashes
typedef struct {
    uint32_t value;
} str_hash;

// Id of file in file registry.
typedef struct {
    uint32_t value;
} file_id;

#endif 