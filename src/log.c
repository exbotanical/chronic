#include "log.h"

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "config.h"
#include "globals.h"
#include "libutil/libutil.h"
#include "proginfo.h"
#include "utils.h"

#define LOG_GUARD     \
  if (log_fd == -1) { \
    return;           \
  }

#define LOG_HEADER TIMESTAMP_FMT " %%s " DAEMON_IDENT ": "

static int log_fd = -1;

// Logging utils vlog, printlogf based on those in M Dillon's dcron
static void
vlog (int fd, const char *fmt, va_list va) {
  char         buf[LARGE_BUFFER];
  // Used to omit the header in the case of multi-line logs
  static short suppress_header = 0;

  time_t     ts                = time(NULL);
  struct tm *ts_info           = localtime(&ts);

  unsigned int buflen, headerlen = 0;
  buf[0] = 0;

  if (!suppress_header) {
    char header[SMALL_BUFFER];

    if (strftime(header, sizeof(header), LOG_HEADER, ts_info)) {
      if ((headerlen = snprintf(buf, sizeof(header), header, proginfo.hostname)) >= sizeof(header)) {
        headerlen = sizeof(header) - 1;
      }
    }
  }

  if ((buflen = vsnprintf(buf + headerlen, sizeof(buf) - headerlen, fmt, va) + headerlen) >= sizeof(buf)) {
    buflen = sizeof(buf) - 1;
  }

  write(fd, buf, buflen);
  // If the log doesn't end with a newline, omit the header next time (next
  // line is a continuation of that log entry)
  suppress_header = (buf[buflen - 1] != '\n');
}

static void
logger_open (char *log_file) {
  if ((log_fd = open(log_file, O_WRONLY | O_CREAT | O_APPEND, OWNER_RW_PERMS)) >= 0) {
    fclose(stderr);
    dup2(log_fd, STDERR_FILENO);
  } else {
    perror("open");
    exit(errno);
  }
}

void
printlogf (const char *fmt, ...) {
  LOG_GUARD
  va_list va;
  va_start(va, fmt);
  vlog(log_fd, fmt, va);
  va_end(va);
}

void
logger_init (void) {
  if (opts.log_file) {
    logger_open(opts.log_file);
  }
}

void
logger_reinit (void) {
  struct stat fd_stat;
  struct stat file_stat;

  // Check for invalidated fd
  if (fstat(log_fd, &fd_stat) < 0) {
    logger_init();
  }

  if (opts.log_file) {
    // Was the log file removed? Or do the fd and file have different inodes now?
    if (stat(opts.log_file, &file_stat) < 0 || fd_stat.st_ino != file_stat.st_ino) {
      logger_init();
    }
  }
}

void
logger_close (void) {
  LOG_GUARD
  close(log_fd);
}
