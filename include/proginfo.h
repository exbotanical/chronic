#ifndef PROGINFO_H
#define PROGINFO_H

#include <fcntl.h>
#include <time.h>

// TODO: Ensure all time is at least long

typedef struct {
  pid_t  pid;
  time_t start;
  char   version[32];
} prog_info_t;

extern prog_info_t proginfo;

#endif /* PROGINFO_H */
