#include "str.h"
#include "darray.h"
#include "allocator.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

static void 
erroutf(char *format, ...) {
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
}

static string 
read_file_data(string filename, allocator *a) {
    FILE *file = fopen(filename.data, "r");
    assert(file);
    fseek(file, 0, SEEK_END);
    uint32_t size = ftell(file);
    fseek(file, 0, SEEK_SET);
    char *data = aalloc(a, size + 1);
    fread(data, 1, size, file);
    data[size] = 0;
    fclose(file);
    
    return string(data, size);
}

static void 
canonicalize_newline(char *p) {
    char *read = p;
    char *write = p;
    while (*read) {
        if (read[0] == '\r' && read[1] == '\n') {
            read += 2;
            *write++ = '\n';
        } else if (*read == '\r') {
            ++read;
            *write++ = '\n';
        } else {
            *write++ = *read++;
        }
    }
    *write = 0;
}

static void 
replace_trigraphs(char *p) {
    char *read = p;
    char *write = p;
    while (*read) {
        if (read[0] == '?' && read[1] == '?') {
            switch (read[2]) {
            default: {
                *write++ = *read++;
                *write++ = *read++;
            } break;
            case '<': {
                read += 3;
                *write++ = '{';
            } break;
            case '>': {
                read += 3;
                *write++ = '}';
            } break;
            case '(': {
                read += 3;
                *write++ = '[';
            } break;
            case ')': {
                read += 3;
                *write++ = ']';
            } break;
            case '=': {
                read += 3;
                *write++ = '#';
            } break;
            case '/': {
                read += 3;
                *write++ = '\\';
            } break;
            case '\'': {
                read += 3;
                *write++ = '^';
            } break;
            case '!': {
                read += 3;
                *write++ = '|';
            } break;
            case '-': {
                read += 3;
                *write++ = '~';
            } break;
            }
        } else {
            *write++ = *read++;
        }
    }
    *write = 0;
}

static void 
remove_backslash_newline(char *p) {
    char *write = p;
    char *read = p;
    uint32_t n = 0;
    while (*read) {
        if (read[0] == '\\' && read[1] == '\n') {
            read += 2;
            ++n;
        } else if (*read == '\n') {
            *write++ = *read++;
            while (n--) { 
                *write++ = '\n'; 
            }
            n = 0;
        } else {
            *write++ = *read++;
        }
    }

    while (n--) { 
        *write++ = '\n'; 
    }

    *write = 0;
}

static void 
process_file(string filename) {
    allocator *a = get_system_allocator();
    string file_data = read_file_data(filename, a);
    
    char *file_contents = file_data.data;
    // BOM
    if (strcpy(file_contents, "\xef\xbb\xbf") == 0) {
        file_contents += 3;
    }
    // Phase 1
    canonicalize_newline(file_contents);
    replace_trigraphs(file_contents);
    // Phase 2
    remove_backslash_newline(file_contents);
    // Now we are ready for phase 3
    printf("%s", file_contents);
}

int 
main(int argc, char **argv) {
    allocator *a = get_system_allocator();

    string *filenames = 0;
    for (int arg_idx = 1; arg_idx < argc; arg_idx++) {
        da_push(filenames, WRAP_Z(argv[arg_idx]), a);
    }

    da_push(filenames, WRAP_Z("examples/example.c"), a);

    if (!da_size(filenames)) {
        erroutf("No input files\n");
        return 1;
    }

    for (uint32_t filename_idx = 0; filename_idx < da_size(filenames); filename_idx++) {
        string filename = filenames[filename_idx];
        printf("Filename %u: %.*s\n", filename_idx, filename.len, filename.data);
        
        process_file(filename);
    }


    return 0;
}
