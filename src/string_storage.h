// String storage is the structure that is used for storing and accessing strings throughout
// the translation. Strings are one of the more complex parts of memory management. Their
// lifetime is hard to guess, and reallocations and assignment of same string to different
// objects happen all the time. C++ advocates to solve this problem with RAII, where each
// individual instance of string has its own memory storage. This, of course is a valid
// solution but can be a bit overkill, considering that forces extensive use of RAII throughout
// the project.
// And in C this would be very unpractical to write.
// So, instead there is simple but effective solution.
// We use bump-allocated chunks for string storage. Each new allocation bumps the pointer of
// buffer, where strings are written.
// This way all strings have the same lifetime, which is equal to lifetime of the string
// storage itself. This may seem like a downside, but there are several factors that make this
// approach viable, despite loss of some memory because of unused strings. In compilers
// specifically, strings are generally used in two different cases. One is to identify objects,
// and these are used during all translation stages, more often as hashes, but still requiring
// string views for giving errors. The other one are rarely used during translation, and are
// later stored in executable. These are string literals. These two types of strings have to
// persist during lifetime of whole program and so don't suffer from inability to be
// reallocated and freed. There are, however other types of strings that don't persist. These
// usually are used for temporary storage during certain compiler stage, like, for example, in
// preprocessing. Preprocessor can concatenate identifiers, join string literals. These cases
// don't apply well to strategy of bump allocation of strings.
// But these do form only the small percent of number of strings in general program, so we
// consider these unimportant. Memory is the cheapest part of computer anyway, and it won't
// suffer from a few kilobytes, which is the memory required for strings.
#ifndef STRING_STORAGE_H
#define STRING_STORAGE_H

#include "general.h"

#define STRING_STORAGE_CHUNK_SIZE (1u << 20)

struct allocator;

typedef struct ss_chunk {
    struct ss_chunk *next;

    char buf[STRING_STORAGE_CHUNK_SIZE];
    uint32_t buf_used;
} ss_chunk;

typedef struct string_storage {
    struct allocator *a;

    ss_chunk *chunk_stack;

    bool ticket;
} string_storage;

string_storage *get_string_storage(void);
char *ss_get_write_ptr(uint32_t write_size);
void ss_complete_write(uint32_t nbytes_written);

#endif
