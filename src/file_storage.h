#ifndef FILE_STORAGE_H
#define FILE_STORAGE_H

#include "types.h"

struct allocator;

typedef struct file {
    struct file *next;
    // typically name inside #include
    string name;
    // full system path
    string full_path;
    // Initial buffer for file contents
    string contents_init;
    // Shrinked buffer with file contents
    string contents;

    bool has_pragma_once;
} file;

typedef struct file_storage {
    // Allocator used internally in file storage. 
    struct allocator *a;
    // Linked list of files.
    // TODO: Should probably be hash table?
    file *files;

    string *include_paths; // da
} file_storage;

// file storage is made global. This, most obvoiuslt, prevents asynchronous
// lock-free file reads. But it has been decided to be unimportant.
file_storage *get_file_storage(void);
void fs_add_default_include_paths(void);
void fs_add_include_paths(string *paths, uint32_t path_count);

file *fs_get_file(string name, file *current_file);

// These are functions used internally in file_storage but we may want to use
// them elsewhere
//
// Does in-place replacement of \n\r and \r to \n
char *canonicalize_newline(char *p);
// Replaces trigraphs with corresponding tokens
char *replace_trigraphs(char *p);
// Removes \ followed by \n, while keeping original logical lines
char *remove_backslash_newline(char *p);

#endif
