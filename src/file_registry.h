/*
Author: Holodome
Date: 20.11.2021
File: src/file_registry.h
Version: 0
*/
#ifndef FILE_REGISTRY_H
#define FILE_REGISTRY_H

#include "types.h"

struct bump_allocator;

// Lists all encoding aviable to file registry
typedef enum {
    FR_ENCODING_ASCII = 0x0,
    FR_ENCODING_UTF8  = 0x1,
} file_registry_encoding;

// Information about single file in file regsitry
typedef struct file_registry_entry {
    // Hash value, specifically this is hash of the 'path' 
    uint32_t hash;
    // Complete file path for access
    string path;
    // Path supplied during creation
    string path_init;
    // Data, it is read during initialization 
    // Data is correctly encoded utf8 sequency of bytes
    void     *data;
    uintptr_t data_size;
    // Linked list pointer
    struct file_registry_entry *next;
} file_registry_entry;

// Encapsulates all data and behavoiur related to accessing files in compiler via #include statements
typedef struct {
    struct bump_allocator *allocator;

    uint32_t include_paths_count;
    string  *include_paths;
    
    file_registry_entry *file_hash;
    uint32_t             file_hash_size;
} file_registry;

// Initializes file registry. 
file_registry *file_registry_init(uint32_t include_paths_count, string *include_paths,
                                  uint32_t file_hash_size, struct bump_allocator *allocator);
// Registers file in file registry, as specified in #include call
// C language has special rules of include search order. First, if file is searched in same folder
// as current file, from which it is being included - this is specified using current_id.
// Later, search is done in project-specific include paths.
// Then it is done in system search paths.
// NOTE: We dont't account special rules for <> header files, which is not defined by standard
// NOTE: Errors are reported in stderr
file_id file_registry_register(file_registry *fr, string filename,
                               file_id current_id); 

// Returns entry stored using given id, 0 otherwise
file_registry_entry *file_registry_get_entry(file_registry_entry *fr, file_id id);

#endif 
