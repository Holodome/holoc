/*
Author: Holodome
Date: 10.10.2021
File: src/filesystem.h
Version: 0
*/
#pragma once 
#include "lib/general.h"

#include "lib/hashing.h" // Hash64

#define MAX_FILES 1024
#define MAX_FILEPATH_LENGTH 4096

struct Memory_Arena;

// File_ID is reference to record about file.
// We often need to access filename, but storing it as a string 
// is inefficient.
// Because of that, filesystem defines storage for all files accessed during program work
// and mainly, stores their paths and allows access to it by id. 
typedef struct {
    u64 opaque;
} File_ID;

// Complete information about some point in text file
typedef struct {
    File_ID file_id;
    u32 line;
    u32 symb;
} Src_Loc;

typedef struct {
    u64   data_size;
    void *data;
} File_Data;

typedef struct {
    File_Data data;
    bool is_valid;
} File_Data_Get_Result;

typedef struct File_Registry_Entry {
    u64  id;
    char path[MAX_FILEPATH_LENGTH];
    bool has_data;
    File_Data data;
} File_Registry_Entry;

typedef struct File_Registry {
    struct Memory_Arena *arena;
    
    // @NOTE(hl): Just continiously incremented - free lists could be added, but we are using hash table 
    // so there is little point in doing that
    u64 next_file_idx; 
    
    Hash_Table64         hash_table;
    File_Registry_Entry *files[MAX_FILES];
} File_Registry;

File_Registry *create_file_registry(struct Memory_Arena *arena);

File_ID register_file(File_Registry *fr, const char *filename);
File_Data_Get_Result get_file_data(File_Registry *fr, File_ID id);
const char *get_file_path(File_Registry *fr, File_ID id);