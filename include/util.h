#ifndef UTIL_H
#define UTIL_H

#include <stddef.h>
#include <time.h>

#include "libhash/libhash.h"
#include "libutil/libutil.h"

#define X(e) [e] = #e

// TODO: Remove
typedef enum { OK, ERR } retval_t;

/**
 * xmalloc is a malloc wrapper that exits the program if out of memory
 */
void *xmalloc(size_t sz);

static inline time_t
round_ts (time_t ts, short unit) {
  return (ts + unit / 2) / unit * unit;
}

static inline unsigned short
get_sleep_duration (short interval, time_t now) {
  // Subtract the remainder of the current time to ensure the iteration
  // begins as close as possible to the desired interval boundary
  unsigned short sleep_time = interval - (short)(now % interval);

  return sleep_time;
}

/**
 * Converts the given time_t into a stringified timestamp.
 * Caller must `free`.
 *
 * @param ts
 */
char *to_time_str(time_t ts);

/**
 * Converts seconds into a string with format:
 * `%d days, %d hours, %d minutes, %d seconds`.
 * Caller must `free`.
 *
 * @param seconds
 */
char *pretty_print_seconds(double seconds);

/**
 * Generates a v4 UUID. Must be freed by caller.
 */
char *create_uuid(void);

/**
 * Given a directory path, returns an array of all of the valid filenames
 * therein.
 *
 * @param dirpath
 */
array_t *get_filenames(char *dirpath);

bool file_exists(const char *path);

int parse_json(const char *json, hash_table *pairs);

#endif /* UTIL_H */