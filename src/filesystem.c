#include "filesystem.h"

#include <stdio.h>

#include "strings.h"
#include "memory.h"
#include "hashing.h"

#define FILESYSTEM_HASH_SIZE 128

typedef struct FilesystemFile {
    char *name;
    u32 hash; // id
    b32 is_data_initialized;
    FileData data;
    
    struct FilesystemFile *next;
} FilesystemFile;

typedef struct Filesystem {
    MemoryArena arena;

    FilesystemFile *file_hash[FILESYSTEM_HASH_SIZE];    
} Filesystem;

static Filesystem filesystem;

static FilesystemFile *get_file_hash(u32 hash_value) {
    u32 hash_slot = hash_value & (FILESYSTEM_HASH_SIZE - 1);
    
    FilesystemFile *file = filesystem.file_hash[hash_slot];
    for (;;) {
        if (file && file->hash == hash_value) {
            break;
        }  
        
        if (file && file->next) {
            file = file->next;
        } else {
            file = arena_alloc_struct(&filesystem.arena, FilesystemFile);
            LLIST_ADD(filesystem.file_hash[hash_slot], file);
            break;
        } 
    }
    
    return file;
}

static FilesystemFile *create_file(const char *name) {
    u32 name_hash = hash_string(name);
    
    FilesystemFile *file = get_file_hash(name_hash);
    assert(file);
    if (file->hash) {
        erroutf("[ERROR] File with name '%s' already exists\n", name);
    } else {
        file->hash = name_hash;
        file->name = arena_alloc_str(&filesystem.arena, name);
    }
    return file;
}

const char *get_file_name(FileID id) {
    const char *result = 0;
    FilesystemFile *file = get_file_hash(id.value);
    if (file) {
        assert(file->name);
        result = file->name;
    }
    return result;
}

const FileData *get_file_data(FileID id) {
    FileData *result = 0;
    FilesystemFile *file = get_file_hash(id.value);
    if (file && file->is_data_initialized) {
        result = &file->data;
    }
    return result;
}

FileID get_file_id_for_buffer(const char *buf, uptr buf_sz, const char *name) {
    FileID id = {0};
    FilesystemFile *file = create_file(name);
    if (file) {
        file->is_data_initialized = TRUE;
        file->data.data = arena_copy(&filesystem.arena, buf, buf_sz);
        file->data.size = buf_sz;
        
        id.value = file->hash;
    }
    return id;
}

FileID get_file_id_for_str(const char *str, const char *name) {
    return get_file_id_for_buffer(str, str_len(str), name);
}

FileID get_file_id_for_filename(const char *filename) {
    FileID id = {0};
    FilesystemFile *file = create_file(filename);
    // @TODO all loading is done in advance for now
    if (file) {
        file->is_data_initialized = TRUE;
        FILE *file_obj = fopen(filename, "rb");
        fseek(file_obj, 0, SEEK_END);
        uptr len = ftell(file_obj);
        fseek(file_obj, 0, SEEK_SET);
        file->data.size = len;
        file->data.data = arena_alloc(&filesystem.arena, len);
        fread(file->data.data, 1, len, file_obj);
        
        id.value = file->hash;
    }
    return id;
}
