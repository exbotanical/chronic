#ifndef LIBHASH_H
#define LIBHASH_H

#include "list.h"

#define HT_DEFAULT_CAPACITY 53
#define HS_DEFAULT_CAPACITY 53

/**
 * A free function that will be invoked a hashmap value any time it is removed.
 *
 * @param value
 * @return typedef
 */
typedef void free_fn(void *value);

/**
 * A hash table entry i.e. key / value pair
 */
typedef struct {
  char *key;
  void *value;
} ht_entry;

/**
 * A hash table
 */
typedef struct {
  /**
   * Max number of entries which may be stored in the hash table. Adjustable.
   * Calculated as the first prime subsequent to the base capacity.
   */
  unsigned int capacity;

  /**
   * Base capacity (used to calculate load for resizing)
   */
  unsigned int base_capacity;

  /**
   * Number of non-NULL entries in the hash table
   */
  unsigned int count;

  /**
   * The hash table's entries
   */
  ht_entry **entries;

  /**
   * Either a free_fn* or NULL; if set, this function pointer will be invoked
   * with hashmap values that are being removed so the caller may free them
   * however they want.
   */
  free_fn *free_value;

  node_t *occupied_buckets;
} hash_table;

/**
 * Initialize a new hash table with a size of `max_size`
 *
 * @param max_size The hash table capacity
 * @param free_value See free_fn
 * @return hash_table*
 */
hash_table *ht_init(int base_capacity, free_fn *free_value);

/**
 * Insert a key, value pair into the given hash table.
 *
 * @param ht
 * @param key
 */
void ht_insert(hash_table *ht, const char *key, void *value);

/**
 * Search for the entry corresponding to the given key
 *
 * @param ht
 * @param key
 */
ht_entry *ht_search(hash_table *ht, const char *key);

/**
 * Eagerly retrieve the value inside of the entry stored at the given key.
 * Will segfault if the key entry does not exist.
 *
 * @param ht
 * @param key
 */
void *ht_get(hash_table *ht, const char *key);

/**
 * Delete a hash table and deallocate its memory
 *
 * @param ht Hash table to delete
 */
void ht_delete_table(hash_table *ht);

/**
 * Delete a entry for the given key `key`. Because entries
 * may be part of a collision chain, and removing them completely
 * could cause infinite lookup attempts, we replace the deleted entry
 * with a NULL sentinel entry.
 *
 * @param ht
 * @param key
 *
 * @return 1 if a entry was deleted, 0 if no entry corresponding
 * to the given key could be found
 */
int ht_delete(hash_table *ht, const char *key);

#define HT_ITER_START(ht)                \
  node_t *head = ht->occupied_buckets;   \
  while (!list_is_sentinel_node(head)) { \
    ht_entry *entry = ht->entries[head->value];

#define HT_ITER_END  \
  head = head->next; \
  }

typedef struct {
  /**
   * Max number of keys which may be stored in the hash set. Adjustable.
   * Calculated as the first prime subsequent to the base capacity.
   */
  unsigned int capacity;

  /**
   * Base capacity (used to calculate load for resizing)
   */
  unsigned int base_capacity;

  /**
   * Number of non-NULL keys in the hash set
   */
  unsigned int count;

  /**
   * The hash set's keys
   */
  char **keys;
} hash_set;

/**
 * Initialize a new hash set with a size of `max_size`
 *
 * @param max_size The hash set capacity
 * @return hash_set*
 */
hash_set *hs_init(int base_capacity);

/**
 * Insert a key into the given hash set.
 *
 * @param hs
 * @param key
 */
void hs_insert(hash_set *hs, const void *key);

/**
 * Check whether the given hash set contains a key `key`
 *
 * @param hs
 * @param key
 * @return 1 for true, 0 for false
 */
int hs_contains(hash_set *hs, const char *key);

/**
 * Delete a hash set and deallocate its memory
 *
 * @param hs Hash set to delete
 */
void hs_delete_set(hash_set *hs);

/**
 * Delete the given key `key`.
 *
 * @param hs
 * @param key
 *
 * @return 1 if a key was deleted, 0 if the given key did not already exist
 * in the set
 */
int hs_delete(hash_set *hs, const char *key);

#endif /* LIBHASH_H */
