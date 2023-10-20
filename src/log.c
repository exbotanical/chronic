#include "log.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "libutil/libutil.h"

#define LOG_BUFFER 2048
#define TIMESTAMP_FMT "%Y-%m-%d %H:%M:%S"

void write_to_log(char *str, ...) {
  va_list va;
  char buf[LOG_BUFFER];

  va_start(va, str);
  vsnprintf(buf, sizeof(buf), str, va);
  write(STDERR_FILENO, buf, strlen(buf));
  va_end(va);
}

char *to_time_str(time_t t) {
  char msg[512];
  struct tm *time_info = localtime(&t);
  strftime(msg, sizeof(msg), TIMESTAMP_FMT, time_info);

  return fmt_str("%s", msg);
}
