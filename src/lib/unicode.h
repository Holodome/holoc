/*
Author: Holodome
Date: 15.10.2021
File: src/lib/unicode.h
Version: 0
*/
#ifndef UNICODE_H
#define UNICODE_H

#include "lib/general.h"

u8 *utf8_encode(u8 *utf8, u32 codepoint);
u32 utf8_decode(const u8 **utf8);

u16 *utf16_encode(u16 *utf16, u32 codepoint);
u32 utf16_decode(const u16 *utf16);

#endif