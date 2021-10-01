// Author: Holodome
// Date: 24.08.2021 
// File: pkby/src/platform/files.h
// Version: 0
//
// Defines os-agnostic API to access files on disk.
//
// @NOTE Filsystem API implements minimal interface for interacting with OS filesystem,
// it does not do streamed IO, like c standard library does (see stream.h).
//
#pragma once
#include "lib/general.h"

enum {
    // 0 in file id could mean not initialized, but this flag explicitly tells that file had errors
    // @NOTE It is still not clear how is better to provide more detailed error details 
    // (probably with use of return codes), but general the fact that file has errors is enough to 
    // stop execution process 
    FILE_FLAG_HAS_ERRORS = 0x1,
    FILE_FLAG_ERROR_NOT_FOUND = 0x2,
    FILE_FLAG_ERROR_ACCESS_DENIED = 0x4,
    FILE_FLAG_IS_CLOSED    = 0x8,
    FILE_FLAG_NOT_OPERATABLE = FILE_FLAG_IS_CLOSED | FILE_FLAG_HAS_ERRORS,
};

// Handle to file object. User has no way of understanding data stored in value
// Because this is not a id but handle, API makes use of static lifetime mechanic builtin in c.
// Everywhere it is used, pointer is passed, because the object itself is strored somewhere
typedef struct {
    // Handle is not guaranteed to have some meaning besides 0 - 
    // that is special value for invalid file handle
    u64 handle;
    u32 flags;
} OSFileHandle;

enum {
    FILE_MODE_READ,
    FILE_MODE_WRITE,
};

void os_open_file(OSFileHandle *handle, const char *filename, u32 mode);
bool os_close_file(OSFileHandle *id);
// Get console out handle
uptr os_write_file(OSFileHandle *file, uptr offset, const void *bf, uptr bf_sz);
// Same as write file, but read
uptr os_read_file(OSFileHandle *file, uptr offset, void *bf, uptr bf_sz);
// read_file with advancing cursor
uptr os_get_file_size(OSFileHandle *);
// @NOTE(hl): Although stanadrd streams in most os's are handled the same way as files,
//  it has been decided to split them in API, because of large difference in logic of work
//  (file api does use offsets, which streams have no support of)
uptr os_write_stdout(void *bf, uptr bf_sz);
uptr os_write_stderr(void *bf, uptr bf_sz);

bool os_is_file_valid(OSFileHandle *);
// Prefixed with fs to avoid collisions (fs for filesystems)
bool os_mkdir(const char *name);
bool os_rmdir(const char *name, bool is_recursive);
bool os_rm(const char *name);
uptr os_fmt_cwd(char *bf, uptr bf_sz);