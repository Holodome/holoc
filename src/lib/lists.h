// Author: Holodome
// Date: 25.08.2021 
// File: pkby/src/lib/hashing.h
// Version: 1
// 
#pragma once
#include "general.h"

#define STACK_ITER(_list, _it) \
for ((_it) = (_list); (_it); (_it) = (_it)->next)
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

#define DLIST_ITER(_first, _it) \
for ((_it) = (_first); (_it); (_it) = (_it)->next)
#define DLIST_ITER_BACK(_last, _it) \
for ((_it) = (_last); (_it); (_it) = (_it)->prev)

#define DLIST_ADD_BEFORE(_first, _last, _node, _new_node) \
do { \
    (_new_node)->next = (_node); \
    if ((_node)->prev == 0) { \
        (_new_node)->prev = 0; \
        (_first) = (_new_node); \
    } else { \
        (_new_node)->prev = (_node)->prev; \
        (_node)->prev->next = (_new_node); \
    } \
    (_node)->prev = (_new_node); \
} while (0); 

#define DLIST_ADD_AFTER(_first, _last, _node, _new_node) \
do { \
    (_new_node)->prev = (_node); \
    if ((_node)->next == 0) { \
        (_new_node)->next = 0; \
        (_last) = (_new_node); \
    } else { \
        (_new_node)->next = (_node)->next; \
        (_node)->next->prev = (_new_node); \
    } \
    (_node)->next = (_new_node); \
} while (0);

#define DLIST_ADD(_first, _last, _node) \
do { \
    if ((_first) == 0) { \
        (_first) = (_node); \
        (_last) = (_node); \
        (_node)->next = 0; \
        (_node)->prev = 0; \
    } else { \
        DLIST_ADD_BEFORE(_first, _list, _first, _node);\
    } \
} while (0);

#define DLIST_ADD_LAST(_first, _last, _node) \
do { \
    if ((_last) == 0) { \
        DLIST_ADD(_first, _last, _node);\
    } else {\
        DLIST_ADD_AFTER(_first, _last, _last, _node);\
    }\
} while (0);

#define DLIST_REMOVE(_first, _last, _node) \
do { \
    if ((_node)->prev == 0) { \
        (_first) = (_node)->next; \
    } else {\
        (_node)->prev->next = (_node)->next; \
    }\
    if ((_node)->next == 0) {\
        (_last) = (_node)->prev;\
    } else {\
        (_node)->next->prev = (_node)->prev;\
    }\
} while (0);