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
process_file(string filename) {
    allocator *a = get_system_allocator();
    string file_data = read_file_data(filename, a);
}

int 
main(int argc, char **argv) {
    allocator *a = get_system_allocator();

    string *filenames = 0;
    for (int arg_idx = 1; arg_idx < argc; arg_idx++) {
        da_push(filenames, WRAP_Z(argv[arg_idx]), a);
    }

    da_push(filenames, WRAP_Z("exampes/example.c"), a);

    if (!da_size(filenames)) {
        erroutf("No input files\n");
        return 1;
    }

    for (uint32_t filename_idx = 0; filename_idx < da_size(filenames); filename_idx++) {
        string filename = filenames[filename_idx];
        printf("Filename %u: %.*s\n", filename_idx, filename.len, filename.data);
    }


    return 0;
}
