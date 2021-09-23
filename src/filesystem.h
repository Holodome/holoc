//
// filesystem.h
// 
// Defines abstraction level over files.h file API.
// All files go through double inderection - first is the FileHandle, which stores
// all data (mostly redundant) about file and has its own lifetime as an object.
// Second level of iderection is FileID, which is some value used to reference FileHandle.
//
// This double inderection allows all handling of file openness and lifetimes being handled 
// by single system, which can make use of fast memory allocations and have other benefits.
//
// FileIDs can be used to get filename.
//
// This is supposed to be high-level API, so it does error reporting directly to console on its own
// (in contrast lower-level file API does not do console error reporting)
#pragma once
#include "general.h"
#include "files.h"
#include "filepath.h"

typedef struct {
    u64 value;
} FileID;

void init_filesystem(void);
b32 is_file_id_valid(FileID id);
FileID fs_get_id_for_filename(const char *filename);
FileHandle *fs_get_handle(FileID id);
// @NOTE The only way to create new FileID. So all files that need to be accounted in filesystem
// need to be abtained through this routine
FileID fs_open_file(const char *name, u32 mode);
// @NOTE The only way to delete the id
b32 fs_close_file(FileID id);

uptr fs_fmt_filename(char *bf, uptr bf_sz, FileID id);