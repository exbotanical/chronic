#include "list.h"

node_t *list_node_create(const int value) {
  node_t *n = (node_t *)malloc(sizeof(node_t));
  n->value = value;
  return n;
}

void list_prepend(node_t **head, int value) {
  // TODO: xmalloc
  node_t *new_node = (node_t *)malloc(sizeof(node_t));
  new_node->value = value;

  node_t *tmp = *head;
  *head = new_node;
  new_node->next = tmp;
}

void list_remove(node_t **head, int value) {
  node_t *current = *head;
  node_t *prev = NULL;

  while (current != NULL) {
    if (current->value == value) {
      if (prev == NULL) {
        *head = current->next;
      } else {
        prev->next = current->next;
      }
      free(current);
      return;
    }
    prev = current;
    current = current->next;
  }
}

void list_free(node_t *head) {
  node_t *headp = head;
  node_t *tmp;
  while (headp != &LIST_SENTINEL_NODE) {
    tmp = headp;
    headp = headp->next;
    free(tmp);
  }
}
