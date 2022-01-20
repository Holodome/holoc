#ifndef FILE_STORAGE_H
#define FILE_STORAGE_H

#include "types.h"

struct allocator;

typedef struct file {
    struct file *next;
    string name;
    string contents_init;
    string contents;
    bool has_pragma_once;
} file;

typedef struct file_storage {
    struct allocator *a;
    file *files;
} file_storage;

file *get_file(file_storage *fs, string name);

void report_error_internalv(char *filename, char *file_contents, uint32_t line,
                            uint32_t col, char *fmt, va_list args);
void report_error_internal(char *filename, char *file_contents, uint32_t line,
                           uint32_t col, char *fmt, ...);
void report_errorv(file *f, uint32_t line, uint32_t col, char *fmt,
                   va_list args);
void report_error(file *f, uint32_t line, uint32_t col, char *fmt, ...);
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
