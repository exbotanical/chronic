#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "hash.h"
#include "libhash.h"
#include "prime.h"
#include "strdup/strdup.h"

static ht_entry HT_SENTINEL_ENTRY = {NULL, NULL};

static void __ht_insert(hash_table *ht, const char *key, void *value);
static int __ht_delete(hash_table *ht, const char *key);
static void __ht_delete_table(hash_table *ht);

/**
 * Resize the hash table. This implementation has a set capacity;
 * hash collisions rise beyond the capacity and `ht_insert` will fail.
 * To mitigate this, we resize up if the load (measured as the ratio of
 * entries count to capacity) is less than .1, or down if the load exceeds
 * .7. To resize, we create a new table approx. 1/2x or 2x times the current
 * table size, then insert into it all non-deleted entries.
 *
 * @param ht
 * @param base_capacity
 * @return int
 */
static void ht_resize(hash_table *ht, int base_capacity) {
  if (base_capacity < HT_DEFAULT_CAPACITY) {
    base_capacity = HT_DEFAULT_CAPACITY;
  }

  hash_table *new_ht = ht_init(base_capacity, ht->free_value);

  for (unsigned int i = 0; i < ht->capacity; i++) {
    ht_entry *r = ht->entries[i];
    if (r != NULL && r != &HT_SENTINEL_ENTRY) {
      __ht_insert(new_ht, r->key, r->value);
    }
  }

  ht->base_capacity = new_ht->base_capacity;
  ht->capacity = new_ht->capacity;
  ht->count = new_ht->count;

  // Cannot free values - we may still be using them.
  ht_entry **tmp_entries = ht->entries;
  ht->entries = new_ht->entries;
  free(tmp_entries);

  // TODO: Figure out why list_free doesnt work but this does
  node_t *head = ht->occupied_buckets;
  node_t *tmp;

  while (!list_is_sentinel_node(head)) {
    tmp = head;
    head = head->next;
    free(tmp);
  }

  ht->occupied_buckets = new_ht->occupied_buckets;

  free(new_ht);
}

/**
 * Resize the table to a larger size, the first prime subsequent
 * to approx. 2x the base capacity.
 *
 * @param ht
 */
static void ht_resize_up(hash_table *ht) {
  const unsigned int new_capacity = ht->base_capacity * 2;
  ht_resize(ht, new_capacity);
}

/**
 * Resize the table to a smaller size, the first prime subsequent
 * to approx. 1/2x the base capacity.
 *
 * @param ht
 */
static void ht_resize_down(hash_table *ht) {
  const unsigned int new_capacity = ht->base_capacity / 2;
  ht_resize(ht, new_capacity);
}

/**
 * Initialize a new hash table entry with the given k, v pair
 *
 * @param k entry key
 * @param v entry value
 * @return ht_entry*
 */
static ht_entry *ht_entry_init(const char *k, void *v) {
  ht_entry *r = malloc(sizeof(ht_entry));
  r->key = strdup(k);
  r->value = v;

  return r;
}

/**
 * Delete a entry and deallocate its memory
 *
 * @param r entry to delete
 */
static void ht_delete_entry(ht_entry *r, free_fn *maybe_free_value) {
  free(r->key);
  if (maybe_free_value && r->value) {
    maybe_free_value(r->value);
    r->value = NULL;
  }
  free(r);
}

static void __ht_insert(hash_table *ht, const char *key, void *value) {
  if (ht == NULL) {
    return;
  }

  const unsigned int load = ht->count * 100 / ht->capacity;
  if (load > 70) {
    ht_resize_up(ht);
  }

  ht_entry *new_entry = ht_entry_init(key, value);

  unsigned int idx = h_compute_hash(new_entry->key, ht->capacity, 0);
  ht_entry *current_entry = ht->entries[idx];
  // If there was a hash collision, we need to perform double hashing and
  // partial linear probing by incrementing this index and hashing it until we
  // find a bucket.
  unsigned int i = 1;
  while (current_entry != NULL && current_entry != &HT_SENTINEL_ENTRY) {
    // If the keys match, then we've inserted this key before. Use this bucket.
    if (strcmp(current_entry->key, key) == 0) {
      ht_delete_entry(current_entry, NULL);
      ht->entries[idx] = new_entry;
      return;
    }

    idx = h_compute_hash(new_entry->key, ht->capacity, i);
    current_entry = ht->entries[idx];
    i++;
  }

  ht->entries[idx] = new_entry;
  list_prepend(&ht->occupied_buckets, idx);
  ht->count++;
}

static int __ht_delete(hash_table *ht, const char *key) {
  const unsigned int load = ht->count * 100 / ht->capacity;

  // TODO: const
  if (load < 30) {
    ht_resize_down(ht);
  }

  unsigned int i = 0;
  unsigned int idx = h_compute_hash(key, ht->capacity, i);

  ht_entry *current_entry = ht->entries[idx];
  while (current_entry != NULL && current_entry != &HT_SENTINEL_ENTRY) {
    if (strcmp(current_entry->key, key) == 0) {
      ht_delete_entry(current_entry, ht->free_value);
      ht->entries[idx] = &HT_SENTINEL_ENTRY;
      list_remove(&ht->occupied_buckets, idx);
      ht->count--;

      return 1;
    }

    idx = h_compute_hash(key, ht->capacity, ++i);
    current_entry = ht->entries[idx];
  }

  return 0;
}

static void __ht_delete_table(hash_table *ht) {
  for (unsigned int i = 0; i < ht->capacity; i++) {
    ht_entry *r = ht->entries[i];

    if (r != NULL && r != &HT_SENTINEL_ENTRY) {
      ht_delete_entry(r, ht->free_value);
    }
  }

  free(ht->entries);
  free(ht);
}

hash_table *ht_init(int base_capacity, free_fn *free_value) {
  if (!base_capacity) {
    base_capacity = HT_DEFAULT_CAPACITY;
  }

  hash_table *ht = malloc(sizeof(hash_table));
  ht->base_capacity = base_capacity;

  ht->capacity = next_prime(ht->base_capacity);
  ht->count = 0;
  ht->entries = calloc((size_t)ht->capacity, sizeof(ht_entry *));
  ht->free_value = free_value;
  ht->occupied_buckets = list_create_sentinel_node();
  return ht;
}

void ht_insert(hash_table *ht, const char *key, void *value) {
  __ht_insert(ht, key, value);
}

ht_entry *ht_search(hash_table *ht, const char *key) {
  unsigned int idx = h_compute_hash(key, ht->capacity, 0);
  ht_entry *current_entry = ht->entries[idx];

  unsigned int i = 1;
  while (current_entry != NULL && current_entry != &HT_SENTINEL_ENTRY) {
    if (strcmp(current_entry->key, key) == 0) {
      return current_entry;
    }

    idx = h_compute_hash(key, ht->capacity, i);
    current_entry = ht->entries[idx];
    i++;
  }

  return NULL;
}

void *ht_get(hash_table *ht, const char *key) {
  ht_entry *r = ht_search(ht, key);
  return r ? r->value : NULL;
}

void ht_delete_table(hash_table *ht) { __ht_delete_table(ht); }

int ht_delete(hash_table *ht, const char *key) { return __ht_delete(ht, key); }
