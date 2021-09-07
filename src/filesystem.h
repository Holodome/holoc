#pragma once
#include "general.h"

typedef struct FileID {
    u32 value;
} FileID;

typedef struct SourceLocation {
    FileID file_id;
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

const char *get_file_name(FileID id);
const FileData *get_file_data(FileID id);
// @TODO limit on name length
FileID get_file_id_for_buffer(const char *buf, uptr buf_sz, const char *name);
FileID get_file_id_for_str(const char *str, const char *name);
FileID get_file_id_for_filename(const char *filename);
