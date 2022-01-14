#ifndef LLIST_H
#define LLIST_H

#define LLIST_ADD(_list, _node)  \
    do {                         \
        (_node)->next = (_list); \
        (_list) = (_node);       \
    } while (0)

#define _LLIST_GET_NEXT(_node, _sample) \
    *(void **)((char *)(_node) + ((_sample)->next - (_sample)))

#define LLIST_ADD_LAST(_list, _node)                    \
    do {                                                \
        if (!(_list)) {                                 \
            (_list) = (_node);                          \
        }                                               \
        for (void *temp = (_list); temp;                \
             temp = _LLIST_GET_NEXT(temp, _node)) {     \
            if (!_LLIST_GET_NEXT(temp, _node)) {        \
                _LLIST_GET_NEXT(temp, _node) = (_node); \
                break;                                  \
            }                                           \
        }                                               \
    } while (0)

#define LLIST_POP(_list)         \
    do {                         \
        (_list) = (_list)->next; \
    } while (0)

#endif
