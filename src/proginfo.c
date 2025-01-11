#define _DEFAULT_SOURCE 1  // For `gethostname`

#include "proginfo.h"

#include <string.h>
#include <unistd.h>

#include "globals.h"

static void
set_hostname (void) {
  if (gethostname(proginfo.hostname, sizeof(proginfo.hostname)) == 0) {
    proginfo.hostname[sizeof(proginfo.hostname) - 1] = 0;
  } else {
    proginfo.hostname[0] = 0;
  }
}

void
proginfo_init (time_t start_time) {
  memcpy(proginfo.version, CHRONIC_VERSION, strlen(CHRONIC_VERSION));
  proginfo.start = start_time;
  proginfo.pid   = getpid();
  set_hostname();
}
