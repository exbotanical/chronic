#include "hash.h"

#include <math.h>    // for pow
#include <string.h>  // for strlen

static const int H_PRIME_1 = 151;
static const int H_PRIME_2 = 163;

/**
 * Hash a given ASCII key, where `prime` is a prime number
 * larger than 128 (ASCII alphabet max)
 *
 * @param key
 * @param prime
 * @param capacity
 * @return unsigned int
 */
static unsigned int h_hash(const char *key, const int prime,
                           const int capacity) {
  long hash = 0;

  const size_t len_s = strlen(key);
  for (unsigned int i = 0; i < len_s; i++) {
    // convert the key to a large integer
    hash += (long)pow(prime, len_s - (i + 1)) * key[i];
    // reduce said large integer to a fixed range
    hash = hash % capacity;
  }

  return (unsigned int)hash;
}

/**
 * Resolve a hash from the given key, using open addressed
 * double-hashing. This method is adjusted contingent on the number of attempts
 * to resolve a hash without a collision. If no collisions have occurred, i == 0
 * and we resolve to `hash_a`. If a collision occurs, we modify the hash with
 * `hash_b`. Finally, if `hash_b` returns 0, the second term is reduced to 0,
 * causing the table to attempt inserting into the same index indefinitely. We
 * mitigate this behavior by adding 1 to the result of `hash_b`, ensuring it is
 * never 0.
 *
 * @param key
 * @param capacity
 * @param attempt Number of attempts made to generate a non-colliding hash.
 * @return unsigned int
 */
unsigned int h_compute_hash(const char *key, const int capacity,
                            const int attempt) {
  const unsigned int hash_a = h_hash(key, H_PRIME_1, capacity);
  unsigned int hash_b = h_hash(key, H_PRIME_2, capacity);

  // Prevent infinite cycling when hash_b == num buckets.
  if (hash_b % capacity == 0) hash_b = 1;
  return (hash_a + (attempt * hash_b)) % capacity;
}
