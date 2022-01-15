#include "types.h"
#include "llist.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#define TEST_CASE(_func) { printf("test: " #_func "\n"); assert(_func()); }

typedef struct node {
    int value;
    struct node *next;
} node;

bool
test_appending_front(void) {
    int array[] = {1, 2, 3, 4, 5};
    node *nodes = 0;
    for (int i = sizeof(array) / sizeof(*array) - 1; i >= 0; --i) {
        node *n = malloc(sizeof(node));
        n->value = array[i];
        LLIST_ADD(nodes, n);
    }

    bool result = true;
    for (int i = 0; (unsigned)i < sizeof(array) / sizeof(*array); ++i) {
        if (!nodes || nodes->value != array[i]) {
            result = false;
            break;
        }
        nodes = nodes->next;
    }
    return result;
}

bool
test_appending_back(void) {
    int array[] = {1, 2, 3, 4, 5};
    node *nodes = 0;
    for (int i = 0; (unsigned)i < sizeof(array) / sizeof(*array); ++i) {
        node *n = calloc(sizeof(node), 1);
        n->value = array[i];
        LLIST_ADD_LAST(nodes, n);
    }

    bool result = true;
    for (int i = 0; (unsigned)i < sizeof(array) / sizeof(*array); ++i) {
        if (!nodes || nodes->value != array[i]) {
            result = false;
            break;
        }
        nodes = nodes->next;
    }
    return result;
}

bool 
test_constructor(void) {
    int array[] = {1, 2, 3, 4, 5};
    linked_list_constructor c = {0};
    for (int i = 0; (unsigned)i < sizeof(array) / sizeof(*array); ++i) {
        node *n = calloc(sizeof(node), 1);
        n->value = array[i];
        LLISTC_ADD_LAST(&c, n);
    }

    node *nodes = c.first;
    bool result = true;
    for (int i = 0; (unsigned)i < sizeof(array) / sizeof(*array); ++i) {
        if (!nodes || nodes->value != array[i]) {
            result = false;
            break;
        }
        nodes = nodes->next;
    }
    return result;
}

int
main(int argc, char **argv) {
    TEST_CASE(test_appending_front);
    TEST_CASE(test_appending_back);
    TEST_CASE(test_constructor);
    return 0;
}

