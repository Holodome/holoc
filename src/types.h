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

typedef struct string {
    char    *data;
    uint32_t len;
} string;

#endif 
