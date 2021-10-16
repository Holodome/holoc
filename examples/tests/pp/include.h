#define A_H
#include "a.h"

#undef A_H
#include "a.h"

int post = 10;

#ifdef A 
int adef = 100;
#else 
int andef = 100;
#endif