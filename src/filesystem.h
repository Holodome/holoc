// filesystem.h
//
// Defines os-agnostic API to access files on disk.
//
// @NOTE API here is different from c standard library, which uses streams for same operations
// Input and utput functions in this API also take offset parmeter, which defines cursor in file
// This is because this API should just provide the minimal functionality to access system files, 
// while C libary provides multi-purpose API
#pragma once
#include "general.h"

// Handle to file object. User has no way of understanding data stored in value
typedef struct FileID {
    u64 value;
} FileID;

typedef struct SourceLocation {
    int line;
    int symb;  
} SourceLocation;

enum {
    FILE_MODE_READ,
    FILE_MODE_WRITE
};

// Get handle for file
FileID create_file(const char *filename, u32 mode);
b32 destroy_file(FileID id);
// Get console out handle
// @NOTE in most OSs writing to file and console is the same.
// This API makes use of same paradigm
FileID get_stdout_file(void);
FileID get_stderr_file(void);
FileID get_stdin_file(void);
// Write bf_sz bytes to file with offset.
// Unlike stdio fwrite, offset is explicitly specified.
// If offset = UINT32_MAX, no offset is done (used in standard streams)
uptr write_file(FileID file, uptr offset, const void *bf, uptr bf_sz);
// Same as write file, but read
uptr read_file(FileID file, uptr offset, void *bf, uptr bf_sz);

uptr get_file_size(FileID);
// Is handle valid.
b32 is_file_valid(FileID);
// Return filename for given id
uptr fmt_filename(char *bf, uptr bf_sz, FileID id);