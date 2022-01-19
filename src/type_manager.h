#ifndef TYPE_MANAGER_H
#define TYPE_MANAGER_H

#include "types.h"

struct allocator;
struct c_type;

typedef struct type_manager {
    struct allocator *a;
} type_manager;

#endif
