/*
Author: Holodome
Date: 10.10.2021
File: src/filesystem.h
Version: 0

System to store information about files that are opened during the compilation process.
Caches data, stores inclusion

@TODO(hl): Strip slashes at the end and start
*/
#ifndef FILE_REGISTRY_H
#define FILE_REGISTRY_H
#include "lib/general.h"

#include "lib/hashing.h" // Hash64

#define MAX_FILES 1024
#define MAX_FILEPATH_LENGTH 4096
#define MAX_INCLUDE_NESTING 16
#define FILE_REGISTRY_SRC_LOC_HASH_SIZE 8192

struct Compiler_Ctx;
struct Memory_Arena;

// File_ID is reference to record about file.
// We often need to access filename, but storing it as a string 
// is inefficient.
// Because of that, filesystem defines storage for all files accessed during program work
// and mainly, stores their paths and allows access to it by id. 
typedef struct {
    u64 opaque;
} File_ID;

// One of the hard problems is how to store the source location info
// Source location info should include filename (if file), line number(if file), symbol number
// for files - in line, for macros - number inside macro definition
// It is very tempting to use the information generated by the compiler - liek the inclusion 
// and macro exapnsion stack, in the way, for example, clang does - it shows 
// where each macro has beend defined and where each file was incuded
// The easy way would be to have each location have its own stack, 
// having [`max file include depth` + `max macro nesting`] - but that would result in too
// much memory waste.
// So the idea becomes to store it is still a stack, but pointer-connected.
// It raises new problem - how do we actually find that several child nodes are referring to the same 
// parent. This can actually be done, if we find some way of hashing all parents
// 
// So the purpose of file registry becomes to store the locations, with each different from another
// This is implemented using internal chaining hash table - so the size can be tweaked 
// depending on some information about expected number of individual locations (using size of file maybe?)
// but allows infinite expanding in case.
// 
// For now, the hash if calculated using simply by applying crc32 to source bytes of Src_Loc tree 
// traversal
enum {
    SRC_LOC_FILE      = 0x1,
    SRC_LOC_MACRO     = 0x2,
    SRC_LOC_MACRO_ARG = 0x3,
};

typedef struct Src_Loc {
    u8      kind;
    File_ID file; // for files
    u32     line; // for files
    u32     symb;    
    const struct Src_Loc *declared_at; // for macros
    const struct Src_Loc *parent;
    
    u32             hash_value;
    struct Src_Loc *next;  // Internal chaining in hash table
} Src_Loc;

typedef struct {
    u64   size;
    void *data;
} File_Data;

typedef struct {
    File_Data data;
    bool is_valid;
} File_Data_Get_Result;

typedef struct File_Registry_Entry {
    u64  id;
    char path     [MAX_FILEPATH_LENGTH]; // Actual path
    char path_init[MAX_FILEPATH_LENGTH]; // Path supplied by user at creation
    bool      has_data;
    File_Data data;
} File_Registry_Entry;

typedef struct File_Registry {
    struct Compiler_Ctx *ctx;
    
    u32          include_seach_paths_count;
    const char **include_search_paths;
    
    // @NOTE(hl): Just continiously incremented - free lists could be added, but we are using hash table 
    // so there is little point in doing that
    u64 next_file_idx; 
    
    Hash_Table64         hash_table;
    File_Registry_Entry *files[MAX_FILES];
    
    Src_Loc *src_loc_hash[FILE_REGISTRY_SRC_LOC_HASH_SIZE];
} File_Registry;

File_Registry *create_file_registry(struct Compiler_Ctx *ctx);

// File_ID register_file(File_Registry *fr, const char *filename);
File_ID register_file(File_Registry *fr, const char *filename, File_ID current_file);
// Src_Loc *get_new_loc(File_Registry *fr, Src_Loc *parent); 
const Src_Loc *get_src_loc_file(File_Registry *fr, File_ID id, u32 line, u32 symb, const Src_Loc *parent);
const Src_Loc *get_src_loc_macro(File_Registry *fr, Src_Loc *declared_at, u32 symb, const Src_Loc *parent);
const Src_Loc *get_src_loc_macro_arg(File_Registry *fr, u32 symb, const Src_Loc *parent);
File_Data_Get_Result get_file_data(File_Registry *fr, File_ID id);
const char *get_file_path(File_Registry *fr, File_ID id);

#endif
