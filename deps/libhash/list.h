#ifndef LIBHASH_LIST_H
#define LIBHASH_LIST_H

#include <stdbool.h>
#include <stdlib.h>

typedef struct node node_t;
struct node {
  int value;
  node_t *next;
};

node_t *list_create_sentinel_node(void);
bool list_is_sentinel_node(node_t *node);
node_t *list_node_create(const int value);
void list_prepend(node_t **head, int value);
void list_remove(node_t **head, int value);
void list_free(node_t *head);

#endif /* LIBHASH_LIST_H */
