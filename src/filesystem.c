#include "filesystem.h"

#include <stdio.h>

#include "strings.h"
#include "memory.h"
#include "hashing.h"

typedef struct FilesystemFile {
    char *name;
    FileData data;
    
    struct FilesystemFile *next;
} FilesystemFile;

typedef struct Filesystem {
    MemoryArena arena;
    
    FilesystemFile *files;
} Filesystem;

static Filesystem filesystem;

FileData *get_file_data(const char *filename) {
    FileData *result = 0;
    
    FilesystemFile *file = 0;
    for (FilesystemFile *scan = filesystem.files;
         scan;
         scan = scan->next) {
        if (str_eq(scan->name, filename)) {
            file = scan;
            break;
        }        
    }
    
    if (file) {
        result = &file->data;
    } else {
        FILE *file_obj = fopen(filename, "rb");
        if (file_obj) {
            file = arena_alloc_struct(&filesystem.arena, FilesystemFile);
            file->name = arena_alloc_str(&filesystem.arena, filename);
            
            fseek(file_obj, 0, SEEK_END);
            uptr len = ftell(file_obj);
            fseek(file_obj, 0, SEEK_SET);
            file->data.data_size = len;
            file->data.data = arena_alloc(&filesystem.arena, len);
            fread(file->data.data, 1, len, file_obj);
           
            LLIST_ADD(filesystem.files, file);
            result = &file->data;
        } 
    }
    
    return result;
}

const char *get_file_at(SourceLocation loc) {
    return 0;
}
