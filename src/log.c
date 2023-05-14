#include "defs.h"

#define LOG_BUFFER 2048

void write_to_log(char *str, ...) {
  va_list va;
  char buf[LOG_BUFFER];

  va_start(va, str);
  vsnprintf(buf, sizeof(buf), str, va);
  write(STDERR_FILENO, buf, strlen(buf));
  va_end(va);
}
