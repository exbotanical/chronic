#include "utils/time.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "libutil/libutil.h"
#include "logger.h"

#ifdef __MACH__
#  include <mach/clock.h>
#  include <mach/mach.h>
#endif

void
get_time (struct timespec* ts) {
#ifdef __MACH__
  clock_serv_t    cclock;
  mach_timespec_t mts;
  host_get_clock_service(mach_host_self(), REALTIME_CLOCK, &cclock);
  clock_get_time(cclock, &mts);
  mach_port_deallocate(mach_task_self(), cclock);
  ts->tv_sec  = mts.tv_sec;
  ts->tv_nsec = mts.tv_nsec;
#else
  clock_gettime(CLOCK_REALTIME, ts);
#endif
}

char*
to_time_str_secs (time_t ts) {
  struct tm* utc_time = gmtime(&ts);
  if (!utc_time) {
    log_warn("gmtime call failed (reason: %s)\n", strerror(errno));
    return NULL;
  }

  char buffer[30];
  strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%S", utc_time);

  char result[40];
  snprintf(result, sizeof(result), "%s.%03ldZ", buffer, 0L);

  return s_copy(result);
}

char*
to_time_str_millis (struct timespec* ts) {
  struct tm* utc_time = gmtime(&ts->tv_sec);
  if (!utc_time) {
    log_warn("gmtime call failed (reason: %s)\n", strerror(errno));
    return NULL;
  }

  char buffer[32];
  strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%S", utc_time);

  char result[64];
  snprintf(result, sizeof(result), "%s.%03ldZ", buffer, ts->tv_nsec / 1000000);

  return s_copy(result);
}

char*
pretty_print_seconds (double seconds) {
  int          ns           = (int)seconds;
  unsigned int days         = ns / 86400;
  ns                       %= 86400;
  unsigned int hours        = ns / 3600;
  ns                       %= 3600;
  unsigned int minutes      = ns / 60;
  ns                       %= 60;
  unsigned int rem_seconds  = ns;

  char* s = s_fmt("%d days, %d hours, %d minutes, %d seconds", days, hours, minutes, rem_seconds);
  return s;
}
