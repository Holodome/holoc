// Author: Holodome
// Date: 25.08.2021 
// File: pkby/src/lib/hashing.h
// Version: 1
// 
#pragma once
#include "general.h"

// @NOTE(hl): Although it was previously named LLIST_*,
// this implementations are more stack-based
#define STACK_ADD(_list, _node) \
do { \
(_node)->next = (_list); \
(_list) = (_node); \
} while (0); 
#define STACK_POP(_list) \
do { \
(_list) = (_list)->next; \
} while (0);

#define CDLIST_INIT(_list) \
do {\
(_list)->next = (_list); \
(_list)->prev = (_list); \
} while (0);
#define CDLIST_ADD(_list, _node) \
do { \
(_node)->next = (_list)->next; \
(_node)->prev = (_list); \
(_node)->next->prev = (_node); \
(_node)->prev->next = (_node); \
} while (0);
#define CDLIST_ADD_LAST(_list, _node) \
do { \
(_node)->next = (_list); \
(_node)->prev = (_list)->prev; \
(_node)->next->prev = (_node); \
(_node)->prev->next = (_node); \
} while (0);
#define CDLIST_REMOVE(_node)\
do {\
(_node)->prev->next = (_node)->next;\
(_node)->next->prev = (_node)->prev;\
} while (0);
