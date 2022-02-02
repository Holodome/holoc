#ifndef LLIST_H
#define LLIST_H

// Addign element to the front of the linked list
#define LLIST_ADD(_list, _node)  \
    do {                         \
        (_node)->next = (_list); \
        (_list)       = (_node); \
    } while (0)

// Helper macro used to get the 'next' field of the _node value
// using _sample as example instance of node structure
// _node can be void *
// _sample must be type that has 'next' field
#define _LLIST_GET_NEXT(_node, _sample) \
    *(void **)((char *)(_node) + ((char *)&(_sample)->next - (char *)(_sample)))

// Quick and dirty way to append to the end of the linked list
// doing this in loop is not advised, use linked_list_constructor
// instead
#define LLIST_ADD_LAST(_list, _node)                                                \
    do {                                                                            \
        if (!(_list)) {                                                             \
            (_list) = (_node);                                                      \
        } else {                                                                    \
            for (void *temp = (_list); temp; temp = _LLIST_GET_NEXT(temp, _node)) { \
                if (!_LLIST_GET_NEXT(temp, _node)) {                                \
                    _LLIST_GET_NEXT(temp, _node) = (_node);                         \
                    break;                                                          \
                }                                                                   \
            }                                                                       \
        }                                                                           \
    } while (0)

// Pops first element from the linked list
#define LLIST_POP(_list)         \
    do {                         \
        (_list) = (_list)->next; \
    } while (0)

// Helper structure used to make appending elements to the end of the linked
// list easier to create, initialize both fields for correspodning values, or 0
// to create new
typedef struct {
    void *first;
    void *last;
} linked_list_constructor;

// Macro used to append to the end of the linked_list_constructor
#define LLISTC_ADD_LAST(_c, _node)                        \
    do {                                                  \
        if (!(_c)->first) {                               \
            (_c)->first = (_node);                        \
        }                                                 \
        if ((_c)->last) {                                 \
            _LLIST_GET_NEXT((_c)->last, _node) = (_node); \
        }                                                 \
        (_c)->last = (_node);                             \
    } while (0)

#endif
