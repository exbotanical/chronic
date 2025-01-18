#ifndef JSON_UTILS_H
#define JSON_UTILS_H

#include "libhash/libhash.h"
#include "utils/retval.h"

/**
 * Parses a given JSON string and places all found key/value pairs in the given hash table.
 * Note: This function supports only a minimal subset of JSON e.g. no arrays and no nesting.
 *
 * @param json
 * @param pairs
 * @return retval_t indicating whether the op succeeded.
 */
retval_t parse_json(const char *json, hash_table *pairs);

/**
 * Escapes all control characters in the given JSON string.
 *
 * @param json
 * @return char* Returns a new string. The caller must call free on this string,
 * as it is stored on the heap.
 */
char *escape_json_string(const char *json);

#endif /* JSON_UTILS_H */
