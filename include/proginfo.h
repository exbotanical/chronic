#ifndef PROGINFO_H
#define PROGINFO_H

#include <fcntl.h>
#include <time.h>

#include "constants.h"

/**
 * Program info and metadata.
 */
typedef struct {
  /**
   * The pid of this program once daemon.
   */
  pid_t            pid;
  /**
   * Time at which the program started.
   */
  struct timespec* start;
  /**
   * The current version of this program.
   */
  char             version[TINY_BUFFER];
  /**
   * The hostname of the system running the program.
   */
  char             hostname[SMALL_BUFFER];
} proginfo_t;

/**
 * Initializes the proginfo global.
 *
 * @param start_time
 */
void proginfo_init(struct timespec* start_time);

#endif /* PROGINFO_H */
