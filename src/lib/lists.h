/*
Author: Holodome
Date: 25.08.2021 
File: src/lib/lists.h
Version: 3

Macros that can be used to produce differnt linked lists' functionality
@TODO(hl): Doubly-linked lists are not tested, they probably need to make a copy of added node
    because of macros nature.
*/
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

//
// Circular doubly-linked list with sentinel
//

#define CDLIST_ITER(_sentinel, _it) \
for ((_it) = (_sentinel)->next; (_it) != (_sentinel); (_it) = (_it)->next)

#define CDLIST_INIT(_sentinel) \
do {\
(_sentinel)->next = (_sentinel); \
(_sentinel)->prev = (_sentinel); \
} while (0);
#define CDLIST_ADD(_sentinel, _node) \
do { \
(_node)->next = (_sentinel)->next; \
(_node)->prev = (_sentinel); \
(_node)->next->prev = (_node); \
(_node)->prev->next = (_node); \
} while (0);
#define CDLIST_ADD_LAST(_sentinel, _node) \
do { \
(_node)->next = (_sentinel); \
(_node)->prev = (_sentinel)->prev; \
(_node)->next->prev = (_node); \
(_node)->prev->next = (_node); \
} while (0);
#define CDLIST_REMOVE(_node)\
do {\
(_node)->prev->next = (_node)->next;\
(_node)->next->prev = (_node)->prev;\
} while (0);

//
// Doubly-linked list
//

#define DLIST_ITER(_first, _it) \
for ((_it) = (_first); (_it); (_it) = (_it)->next)
#define DLIST_ITER_BACK(_last, _it) \
for ((_it) = (_last); (_it); (_it) = (_it)->prev)

#define DLIST_ADD_BEFORE(_list, _node, _new_node) \
do { \
    (_new_node)->next = (_node); \
    if ((_node)->prev == 0) { \
        (_new_node)->prev = 0; \
        (_list)->first = (_new_node); \
    } else { \
        (_new_node)->prev = (_node)->prev; \
        (_node)->prev->next = (_new_node); \
    } \
    (_node)->prev = (_new_node); \
} while (0); 

#define DLIST_ADD_AFTER(_list, _node, _new_node) \
do { \
    (_new_node)->prev = (_node); \
    if ((_node)->next == 0) { \
        (_new_node)->next = 0; \
        (_list)->last = (_new_node); \
    } else { \
        (_new_node)->next = (_node)->next; \
        (_node)->next->prev = (_new_node); \
    } \
    (_node)->next = (_new_node); \
} while (0);

#define DLIST_ADD(_list, _node) \
do { \
    if ((_list)->first == 0) { \
        (_list)->first = (_node); \
        (_list)->first = (_node); \
        (_node)->next = 0; \
        (_node)->prev = 0; \
    } else { \
        DLIST_ADD_BEFORE(_list, (_list)->first, _node);\
    } \
} while (0);

#define DLIST_ADD_LAST(_list, _node) \
do { \
    if ((_list)->last == 0) { \
        (_list)->first = (_node); \
        (_list)->last = (_node); \
        (_node)->next = 0; \
        (_node)->prev = 0; \
    } else {\
        DLIST_ADD_AFTER(_list, (_list)->last, _node);\
    }\
} while (0);

#define DLIST_REMOVE(_list, _node) \
do { \
    if ((_node)->prev == 0) { \
        (_list)->first = (_node)->next; \
    } else {\
        (_node)->prev->next = (_node)->next; \
    }\
    if ((_node)->next == 0) {\
        (_list)->last = (_node)->prev;\
    } else {\
        (_node)->next->prev = (_node)->prev;\
    }\
} while (0);