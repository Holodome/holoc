#pragma once
#include "general.h"

typedef struct FileID {
    u32 value;
} FileID;

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

FileID get_id_for_buffer(const char *buf, )
FileID get_id_for_filename(const char *filename);

// Load file procedure
FileData *DEBUG_get_file_data(const char *filename);
// returns pointer to source location inside file contents
const char *get_file_at(SourceLocation loc);

#if 0
FileID file_id = get_id_for_filename("input.txt");
Tokenizer tokenizer = create_tokenizer(file_id);

FileID buffer_id = get_id_for_raw("a := 2; print a;", "test1");
Tokenizer tokenizer = create_tokenizer(file_id);

#endif 