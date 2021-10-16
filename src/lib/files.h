// Author: Holodome
// Date: 24.08.2021 
// File: pkby/src/lib/files.h
// Version: 0
//
// Defines os-agnostic API to access files on disk.
//
// @NOTE Filsystem API implements minimal interface for interacting with OS filesystem,
// it does not do streamed IO, like c standard library does (see stream.h).
//
// @TODO(hl): @CLEANUP Change from pointers to static structs. There is just little point in that,
//  because errors are never written to file, and we can make separate function for getting them
#pragma once
#include "lib/general.h"

struct Memory_Arena;

enum {
    // 0 in file id could mean not initialized, but this flag explicitly tells that file had errors
    // @NOTE It is still not clear how is better to provide more detailed error details 
    // (probably with use of return codes), but general the fact that file has errors is enough to 
    // stop execution process 
    FILE_HAS_NO_ERRORS_BIT       = 0x1,
    FILE_ERROR_NOT_FOUND_BIT     = 0x2,
    FILE_ERROR_ACCESS_DENIED_BIT = 0x4,
    FILE_IS_CLOSED_BIT           = 0x8,
    
    FILE_IS_WRITE_BIT            = 0x1000, // If not set, file is for reading
};

// Handle to file object. User has no way of understanding data stored in value
// Because this is not a id but handle, API makes use of static lifetime mechanic builtin in c.
// Everywhere it is used, pointer is passed, because the object itself is strored somewhere
typedef struct {
    // Handle is not guaranteed to have some meaning besides 0 - 
    // that is special value for invalid file handle
    u64 opaque;
    u32 flags;
} OS_File_Handle;

#define OS_IS_FILE_HANDLE_VALID(_handle) ((_handle).flags & FILE_HAS_NO_ERRORS_BIT)

OS_File_Handle os_open_file_read(const char *filename);
OS_File_Handle os_open_file_write(const char *filename);
bool os_close_file(OS_File_Handle handle);
// Get console out handle
uptr os_write_file(OS_File_Handle file, uptr offset, const void *bf, uptr bf_sz);
// Same as write file, but read
uptr os_read_file(OS_File_Handle file, uptr offset, void *bf, uptr bf_sz);
// read_file with advancing cursor
u64 os_get_file_size(OS_File_Handle handle);
// @NOTE(hl): Although stanadrd streams in most os's are handled the same way as files,
//  it has been decided to split them in API, because of large difference in logic of work
//  (file api does use offsets, which streams have no support of)
uptr os_write_stdout(void *bf, uptr bf_sz);
uptr os_write_stderr(void *bf, uptr bf_sz);

// Prefixed with fs to avoid collisions (fs for filesystems)
bool os_mkdir(const char *name);
bool os_rmdir(const char *name, bool is_recursive);
bool os_rm(const char *name);
uptr os_fmt_cwd(char *bf, uptr bf_sz);

typedef struct {
    u32 count;
    const char **entries;
} OS_Dir_Entries_Result;

void os_dir_entries(const char *dir, struct Memory_Arena *arena);

typedef struct {
    bool exists;
    bool is_directory;
    u32  size;
} OS_Stat;

OS_Stat os_stat(const char *filename);