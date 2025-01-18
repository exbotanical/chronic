#ifndef TIME_UTILS_H
#define TIME_UTILS_H

#include <time.h>

/**
 * Rounds a given timestamp to the nearest N unit.
 *
 * @param ts
 * @param unit e.g. nearest 1st, 10th, or 100th second, etc
 * @return time_t
 */
static inline time_t
round_ts (time_t ts, short unit) {
  return (ts + unit / 2) / unit * unit;
}

/**
 * Computes a duration that must be awaited to align on the next interval boundary.
 *
 * @param interval
 * @param now
 * @return unsigned short
 */
static inline unsigned short
get_sleep_duration (short interval, time_t now) {
  // Subtract the remainder of the current time to ensure the iteration
  // begins as close as possible to the desired interval boundary
  unsigned short sleep_time = interval - (short)(now % interval);

  return sleep_time;
}

/**
 * Sets the current time on the given timespec.
 *
 * @param ts
 */
void get_time(struct timespec *ts);

/**
 * Converts the given time_t into a  stringified timestamp.
 * Caller must call `free` on the returned char pointer.
 *
 * @param ts
 */
char *to_time_str_secs(time_t ts);

/**
 * Converts the given timespec into a  stringified timestamp.
 * Caller must call `free` on the returned char pointer.
 *
 * @param ts
 */
char *to_time_str_millis(struct timespec *ts);


/**
 * Converts seconds into a string with format:
 * `%d days, %d hours, %d minutes, %d seconds`.
 * Caller must call `free` on the returned char pointer.
 *
 * @param seconds
 */
char *pretty_print_seconds(double seconds);

#endif /* TIME_UTILS_H */
