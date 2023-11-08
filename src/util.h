#ifndef UTIL_H
#define UTIL_H

#include <time.h>

#define X(e) [e] = #e

typedef enum { OK, ERR } Retval;

/**
 * xmalloc is a malloc wrapper that exits the program if out of memory
 */
void *xmalloc(size_t sz);

// char *create_uuid(void);

inline static time_t round_ts(time_t ts, short unit) {
  return (ts + unit / 2) / unit * unit;
}

inline static unsigned short get_sleep_duration(short interval, time_t now) {
  // Subtract the remainder of the current time to ensure the iteration
  // begins as close as possible to the desired interval boundary
  unsigned short sleep_time = interval - (short)(now % interval);

  return sleep_time;
}

#endif /* UTIL_H */
