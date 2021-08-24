#include "filesystem.h"

#define FILESYSTEM_HASH_SIZE 128

typedef struct FilesystemHashEntry {
    
} FilesystemHashEntry;

// Filesystem provides hash-based way to retrieve text information from different sources.
// Altough it is named filesystem, generally it servers as hash table for getting text based on name string.
// It still can be used to get text file data.
// 
typedef struct Filesystem {
    FilesystemHashEntry hash[FILESYSTEM_HASH_SIZE];  
} Filesystem;

// static Filesystem filesystem;

