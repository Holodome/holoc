#include "str.h"
#include "darray.h"
#include "my_assert.h"
#include "bump_allocator.h"

#include "lexer.h"
#include "string_storage.h"
#include "file_registry.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Strcture holding all possible porgramm settings that are set via command line arguments
// and default ones
typedef struct {
    // Filenames that are compiled
    /* da */ string *source_filenames; 
    // Include directories
    /* da */ string *include_dirs;
    // SOA of defines. If define has no value specified (via command line option)
    // , it will have default value of 1
    /* da */ string *defines;
    /* da */ string *define_values;
    char *output_filename;
    bool print_usage;
    bool verbose;
    bool only_preprocess;
} program_settings;
 
static void 
usage(int status) {
    fprintf(stderr, "holoc [ -o <path> ] <file>\n");
    exit(status);
}

static void 
define(program_settings *settings, char *zdef) {
    string def = stringz(zdef);
    string_find_result equal_sign_find = string_find(def, '=');
    char *ident = 0;
    char *definition = "1";
    if (equal_sign_find.is_found) {
        uint32_t definition_len = def.len - equal_sign_find.idx;
        uint32_t ident_len = def.len - definition_len;
        definition = calloc(definition_len, 1);
        memcpy(definition, zdef + equal_sign_find.idx, definition_len);
        ident = calloc(ident_len, 1);
        memcpy(ident, zdef, ident_len);
    } else {
        ident = strdup(zdef);
    }
    assert(ident);
    // Test that ident is a proper identifier
    // TODO: Support for function-like macros definition
    for (char *test = ident;
         *test;
         ++test) {
        if ((test == ident && !is_ident_start(*test)) || (test != ident && !is_ident(*test))) {
            fprintf(stderr, "Non-identifier passed to -D: %s\n", zdef);
            goto end;
        }
    }
    da_push(settings->define_values, stringz(definition));
    da_push(settings->defines, stringz(ident));
end: (void)0;
}
 
static void 
init_program_settings(program_settings *settings) {
    // Default include paths
    da_push(settings->include_dirs, WRAP_Z("/usr/local/include"));
    da_push(settings->include_dirs, WRAP_Z("/usr/include"));
}

static bool
parse_command_line_arguments(int argc, char **argv, program_settings *settings) {
    bool result = false;
    
    int cursor = 1;
    while (cursor < argc) {
        char *option = argv[cursor];
        if (!strncmp(option, "-I", 2)) {
            char *path_str = strdup(option + 2);
            da_push(settings->include_dirs, stringz(path_str));
        } else if (!strncmp(option, "-D", 2)) {
            define(settings, option + 2);
        } else if (!strcmp(option, "--help")) {
            usage(0);
        } else if (!strcmp(option, "-v")) {
            settings->verbose = true;
        } else if (!strcmp(option, "-E")) {
            settings->only_preprocess = true;
        }
        da_push(settings->source_filenames, stringz(option));
        ++cursor;  
    }
    
    return result;
}

int 
main(int argc, char **argv) {
    program_settings settings = {0};
    init_program_settings(&settings);
    if (!parse_command_line_arguments(argc, argv, &settings)) {
        usage(1);
    }
    
    if (settings.print_usage) {
        usage(0);
    } 
   
    string filename = WRAP_Z("tests/test.c");
    bump_allocator general_allocator = {0};
    file_registry *fr = file_registry_init(da_size(settings.include_dirs), settings.include_dirs, 128, &general_allocator);
    string_storage *ss = string_storage_init(&general_allocator);
    c_lexer *lexer = c_lexer_init(fr, ss, filename);
    
    if (settings.only_preprocess) {
        goto end;
    }

end:
    return 0;
}
