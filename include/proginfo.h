#ifndef PROGINFO_H
#define PROGINFO_H

#include <fcntl.h>
#include <time.h>

#include "constants.h"

// TODO: Ensure all time is at least long

typedef struct {
  pid_t  pid;
  time_t start;
  char   version[TINY_BUFFER];
  char   hostname[SMALL_BUFFER];
} proginfo_t;

void set_proginfo(time_t start_time);

#endif /* PROGINFO_H */
