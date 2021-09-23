//
// filepath.h
//
// Abstraction over filesystem paths.
// S
// Storing paths as plain text is inefficient in basically eny operations besides file opening -
// it doesn't allow fast knowing of parents't directory, and file escape characters of different OS's
// can differ.
// 
// Main problm with storing paths as strings is that different strings can point to the same path.
// This is due to inclusion of . and .. characters and, mostly, existence of both relative and absolute paths.
// Filepath constructs absolute filepath, which, in fact 
#pragma once

#include "general.h"
#include "memory.h"

// Max joined length of filepath.
// Value is taken from linux specification, value on windows is smaller.
// But waht we do care about is it works
// Having this constant allows us a
#define MAX_FILEPATH 4096

typedef struct FilepathPart {
    char *name;
    struct FilepathPart *next;
} FilepathPart;

typedef struct {
    u8 *bf;
    uptr bf_sz;
    FilepathPart first_part;
} Filepath;

Filepath create_filepath_from_filename(const char *filename, MemoryArena *arena);
uptr fmt_filepath(char *bf, uptr bf_sz, Filepath *filepath);