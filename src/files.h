// filesystem.c
//
// Defines os-agnostic API to access files on disk.
//
// @NOTE Filsystem API implements minimal interface for interacting with OS filesystem,
// it does not do streamed IO, like c standard library does (see stream.h).
//
// @NOTE It is an ongoing experiment to create API that does not use 
// cursors implictly; But it is really a double edged sword - because it handling text files becomes easy
// (like with use of cursors) then binary files become tedious and the other way around.
// It seems, however, that having api for specialized for binary files is more benefitial due to the
// fact that text files are mostly used with streams (see stream.h) and binary files are processed by hand
// for each format
#pragma once
#include "general.h"

enum {
    // 0 in file id could mean not initialized, but this flag explicitly tells that file had errors
    // @NOTE It is still not clear how is better to provide more detailed error details 
    // (probably with use of return codes), but general the fact that file has errors is enough to 
    // stop execution process 
    FILE_FLAG_HAS_ERRORS   = 0x1,
    // File mode is appending - API disallows mixing appending and offset-based IO 
    FILE_FLAG_IS_APPENDING = 0x2,
    // Is standard stream 
    FILE_FLAG_IS_ST        = 0x4,
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
    FILE_MODE_READ_APPEND,
    FILE_MODE_WRITE_APPEND,
};

void os_open_file(OSFileHandle *handle, const char *filename, u32 mode);
b32 os_close_file(OSFileHandle *id);
// Get console out handle
// @NOTE in most OSs writing to file and console is the same.
// This API makes use of same paradigm
OSFileHandle *os_get_stdout_file(void);
OSFileHandle *os_get_stderr_file(void);
OSFileHandle *os_get_stdin_file(void);
// Write bf_sz bytes to file with offset.
// Unlike stdio fwrite, offset is explicitly specified.
// If offset = UINT32_MAX, no offset is done (used in standard streams)
// @TODO API is not clear. By having way of accessing IO streams, we basiaclly provide
// user with opportunity ot do IO on the file end, which is what this API tries actually not to do
// But behaviour of IO of file end is not clear because in all other cases offsets are used
// and behaviour will not be as expected.
// Solution could be to create special function for accessing file end, and, for example, 
// have flag in file structure that indicates what mode it is used so user could be warned on
// incorrect usage
uptr os_write_file(OSFileHandle *file, uptr offset, const void *bf, uptr bf_sz);
// Same as write file, but read
uptr os_read_file(OSFileHandle *file, uptr offset, void *bf, uptr bf_sz);
// read_file with advancing cursor
uptr os_get_file_size(OSFileHandle *);
// Is handle valid.
b32 os_is_file_valid(OSFileHandle *);
// Prefixed with fs to avoid collisions (fs for filesystems)
b32 os_mkdir(const char *name);
b32 os_rmdir(const char *name, b32 is_recursive);
b32 os_rm(const char *name);
uptr os_fmt_cwd(char *bf, uptr bf_sz);