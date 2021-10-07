// Author: Holodome
// Date: 24.08.2021 
// File: pkby/src/lib/filesystem.h
// Version: 0
// 
// Defines abstraction level over files.h file API.
// All files go through double inderection - first is the OS_File_Handle, which stores
// all data (mostly redundant) about file and has its own lifetime as an object.
// Second level of iderection is FileID, which is some value used to reference OS_File_Handle.
//
// This double inderection allows all handling of file openness and lifetimes being handled 
// by single system, which can make use of fast memory allocations and have other benefits.
//
// FileIDs can be used to get filename.
//
// This is supposed to be high-level API, so it does error reporting directly to console on its own
// (in contrast lower-level file API does not do console error reporting)
#pragma once
#include "lib/general.h"
#include "platform/files.h"
#include "platform/memory.h"

typedef struct {
    u64 value;
} FileID;

// Max joined length of filepath.
// Value is taken from linux specification, value on windows is smaller.
// But waht we do care about is it works
#define MAX_FILEPATH 4096

typedef struct Filepath_Part {
    char *name;
    struct Filepath_Part *next;
} Filepath_Part;

typedef struct {
    Filepath_Part first_part;
} Filepath;

Filepath create_filepath_from_filename(const char *filename, Memory_Arena *arena);
uptr fmt_filepath(char *bf, uptr bf_sz, Filepath *filepath);

// Initialize storage for filesystem
void init_filesystem(void);
// id.value != 0
bool fs_is_file_valid(FileID id);
// @NOTE The only way to create new FileID. So all files that need to be accounted in filesystem
// need to be abtained through this routine
FileID fs_open_file(const char *name, u32 mode);
// @NOTE The only way to delete the id
bool fs_close_file(FileID id);

// Return handle for file if it is open, 0 otherwise
OS_File_Handle *fs_get_handle(FileID id);
// Preferable way of getting file size. Caches result to minimize os calls
uptr fs_get_file_size(FileID id);
uptr fs_fmt_filename(char *bf, uptr bf_sz, FileID id);

FileID fs_get_stdout(void);
FileID fs_get_stderr(void);

/* 
@NOTE(hl): Debugging tool when we need to inspect some binary (or even string) data
In current debuggers, inspection of buffers of any kind is complicated.
This may help solve this problem, but still not the best solution
*/
void DBG_dump_file(const char *filename, const void *data, u64 data_size);
