#ifndef LIBHASH_LIST_H
#define LIBHASH_LIST_H

#include <stdlib.h>

typedef struct node node_t;
struct node {
  int value;
  node_t *next;
};

static node_t LIST_SENTINEL_NODE = {
    .value = 0,
    .next = NULL,
};

static inline node_t *list_node_create_head(void) {
  return &LIST_SENTINEL_NODE;
}

node_t *list_node_create(const int value);
void list_prepend(node_t **head, int value);
void list_remove(node_t **head, int value);
void list_free(node_t *head);

#endif /* LIBHASH_LIST_H */
