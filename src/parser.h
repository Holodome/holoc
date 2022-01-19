#ifndef PARSER_H
#define PARSER_H

#include "types.h"

#define PARSER_SCOPE_HASH_SIZE 1024

struct bump_allocator;
struct c_type;
struct ast_builder;

typedef enum { _ } parser_object_flags;

typedef struct parser_object {
    struct parser_object *next;
    string name;
    struct c_type *type;
    uint32_t flags;
} parser_object;

typedef struct parser_scope {
    struct parser_scope *next;
} parser_scope;

typedef struct parser {
    struct bump_allocator *a;

    struct ast_builder *b;

    parser_object *globals;
    parser_scope *scope;
} parser;

#endif
