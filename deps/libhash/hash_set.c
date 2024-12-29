#include <stdlib.h>
#include <string.h>

#include "hash.h"
#include "libhash.h"
#include "prime.h"
#include "strdup/strdup.h"

/**
 * Resize the hash set. This implementation has a set capacity;
 * hash collisions rise beyond the capacity and `hs_insert` will fail.
 * To mitigate this, we resize up if the load (measured as the ratio of keys
 * count to capacity) is less than .1, or down if the load exceeds .7. To
 * resize, we create a new set approx. 1/2x or 2x times the current set
 * size, then insert into it all non-deleted keys.
 *
 * @param hs
 * @param base_capacity
 * @return int
 */
static void hs_resize(hash_set *hs, const int base_capacity) {
  if (base_capacity < 0) {
    return;
  }

  hash_set *new_hs = hs_init(base_capacity);

  for (unsigned int i = 0; i < hs->capacity; i++) {
    const char *r = hs->keys[i];

    if (r != NULL) {
      hs_insert(new_hs, r);
    }
  }

  hs->base_capacity = new_hs->base_capacity;
  hs->count = new_hs->count;

  const unsigned int tmp_capacity = hs->capacity;
  hs->capacity = new_hs->capacity;
  new_hs->capacity = tmp_capacity;

  char **tmp_keys = hs->keys;
  hs->keys = new_hs->keys;
  new_hs->keys = tmp_keys;

  hs_delete_set(new_hs);
}

/**
 * Resize the set to a larger size, the first prime subsequent
 * to approx. 2x the base capacity.
 *
 * @param hs
 */
static void hs_resize_up(hash_set *hs) {
  const unsigned int new_capacity = hs->base_capacity * 2;
  hs_resize(hs, new_capacity);
}

/**
 * Resize the set to a smaller size, the first prime subsequent
 * to approx. 1/2x the base capacity.
 *
 * @param hs
 */
static void hs_resize_down(hash_set *hs) {
  const unsigned int new_capacity = hs->base_capacity / 2;
  hs_resize(hs, new_capacity);
}

/**
 * Delete a key and deallocate its memory
 *
 * @param r key to delete
 */
static void hs_delete_key(char *r) { free(r); }

hash_set *hs_init(int base_capacity) {
  if (!base_capacity) {
    base_capacity = HS_DEFAULT_CAPACITY;
  }

  hash_set *hs = malloc(sizeof(hash_set));
  hs->base_capacity = base_capacity;

  hs->capacity = next_prime(hs->base_capacity);
  hs->count = 0;
  hs->keys = calloc((size_t)hs->capacity, sizeof(char *));

  return hs;
}

void hs_insert(hash_set *hs, const void *key) {
  if (hs == NULL) {
    return;
  }

  const unsigned int load = hs->count * 100 / hs->capacity;
  if (load > 70) {
    hs_resize_up(hs);
  }

  void *new_entry = strdup(key);

  unsigned int idx = h_compute_hash(key, hs->capacity, 0);
  char *current_key = hs->keys[idx];

  unsigned int i = 1;
  // If there was a collision...
  while (current_key != NULL) {
    // Key already exists (update)
    if (strcmp(current_key, key) == 0) {
      return;
    }

    idx = h_compute_hash(new_entry, hs->capacity, i);
    current_key = hs->keys[idx];
    i++;
  }

  hs->keys[idx] = new_entry;
  hs->count++;
}

int hs_contains(hash_set *hs, const char *key) {
  unsigned int idx = h_compute_hash(key, hs->capacity, 0);
  char *current_key = hs->keys[idx];

  unsigned int i = 1;
  while (current_key != NULL) {
    if (strcmp(current_key, key) == 0) {
      return 1;
    }

    idx = h_compute_hash(key, hs->capacity, i);
    current_key = hs->keys[idx];
    i++;

    // If the capacity was set to the exact number of keys, we need to break
    // once we've checked all of them - there won't be any NULL keys
    if (i == hs->count + 1) {
      break;
    }
  }

  return 0;
}

void hs_delete_set(hash_set *hs) {
  for (unsigned int i = 0; i < hs->capacity; i++) {
    char *r = hs->keys[i];

    if (r != NULL) {
      hs_delete_key(r);
    }
  }

  free(hs->keys);
  free(hs);
}

int hs_delete(hash_set *hs, const char *key) {
  const unsigned int load = hs->count * 100 / hs->capacity;

  if (load < 10) {
    hs_resize_down(hs);
  }

  unsigned int i = 0;
  unsigned int idx = h_compute_hash(key, hs->capacity, i);

  char *current_key = hs->keys[idx];

  while (current_key != NULL) {
    if (strcmp(current_key, key) == 0) {
      hs_delete_key(current_key);
      hs->keys[idx] = NULL;

      hs->count--;

      return 1;
    }

    idx = h_compute_hash(key, hs->capacity, ++i);
    current_key = hs->keys[idx];
  }

  return 0;
}
