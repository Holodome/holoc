#pragma once
#include "general.h"

// Structure that can be used to retrieve text from filesystem, for example for error hanndling.
// See filesystem for details
typedef struct SourceLocation {
    const char *source_name;
    int line;
    int symb;  
} SourceLocation;

typedef struct FileData {
    union {
        u8 *data;
        char *str;
    };
    uptr data_size;
} FileData;

// If file is not loaded, loads it and returns data.
// If it is already loaded, just return data
// 0 means error
// @TODO find a way to tell error 
FileData *get_file_data(const char *filename);
// returns pointer to source location inside file contents
const char *get_file_at(SourceLocation loc);