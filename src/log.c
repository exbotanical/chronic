#include "log.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "libutil/libutil.h"

#define LOG_BUFFER 2048
#define TIMESTAMP_FMT "%Y-%m-%d %H:%M:%S"

void write_to_log(char *str, ...) {
  va_list va;
  char buf[LOG_BUFFER];

  va_start(va, str);

  char *s = fmt_str("%s %s", to_time_str(time(NULL)), str);
  vsnprintf(buf, sizeof(buf), s, va);

  write(STDERR_FILENO, buf, strlen(buf));

  free(s);
  va_end(va);
}

char *to_time_str(time_t t) {
  char msg[512];
  struct tm *time_info = localtime(&t);
  strftime(msg, sizeof(msg), TIMESTAMP_FMT, time_info);

  return fmt_str("%s", msg);
}
