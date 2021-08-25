#include "hashing.h"

u32 hash_string(const char *str) {
    u32 result = 0;
    // @TODO better hash function
    while (*str) {
        result = result * 78 + (*str++) * 31;
    }
    return result;
}