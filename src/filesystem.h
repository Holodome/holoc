#pragma once
#include "general.h"

// Structure that can be used to retrieve text from filesystem, for example for error hanndling.
// See filesystem for details
typedef struct SourceLocation {
    const char *source_name;
    int line;
    int symb;  
} SourceLocation;
