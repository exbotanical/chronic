#include "logger.h"

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
#include "panic.h"
#include "proginfo.h"
#include "util.h"

#define LOG_LEVEL_FMT "[%s]"

//   "%Y-%m-%d %H:%M:%S - [%s] %%s crond: "
#define LOG_HEADER    "%s - " LOG_LEVEL_FMT " %s " DAEMON_IDENT ": "

#define LOG_GUARD     \
  if (log_fd == -1) { \
    return;           \
  }

#define Y(e) [e] = &(#e)[10]
static const char *log_level_names[] = {Y(LOG_LEVEL_ERROR), Y(LOG_LEVEL_WARN), Y(LOG_LEVEL_INFO), Y(LOG_LEVEL_DEBUG)};

static int log_fd = -1;

void
printlogf (log_level lvl, const char *fmt, ...) {
  va_list va;
  va_start(va, fmt);

  if (opts.syslog) {
    char buf[LARGE_BUFFER];
    vsnprintf(buf, LARGE_BUFFER, fmt, va);
    syslog(lvl, buf);
  } else {
    char         buf[LARGE_BUFFER];
    // Used to omit the header in the case of multi-line logs
    static short suppress_header = 0;

    struct timespec ts_info;
    get_time(&ts_info);

    unsigned int buflen, headerlen = 0;
    buf[0] = 0;

    if (!suppress_header) {
      char *ts = to_time_str_millis(&ts_info);

      if ((headerlen = snprintf(
             buf,
             SMALL_BUFFER,
             LOG_HEADER,
             ts,
             log_level_names[lvl],
             proginfo.hostname
           ))
          >= SMALL_BUFFER) {
        headerlen = SMALL_BUFFER - 1;
      }

      free(ts);
    }

    if ((buflen = vsnprintf(buf + headerlen, sizeof(buf) - headerlen, fmt, va) + headerlen) >= sizeof(buf)) {
      buflen = sizeof(buf) - 1;
    }

    write(log_fd, buf, buflen);
    // If the log doesn't end with a newline, omit the header next time (next
    // line is a continuation of that log entry)
    suppress_header = (buf[buflen - 1] != '\n');
  }

  va_end(va);
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

static void
syslog_open (void) {
  fclose(stderr);
  dup2(log_fd, STDERR_FILENO);
  openlog(DAEMON_IDENT, LOG_CONS | LOG_PID, LOG_CRON);
}

void
logger_init (void) {
  if (opts.log_file) {
    logger_open(opts.log_file);
  } else if (opts.syslog) {
    syslog_open();
  } else {
    opts.log_file = s_copy_or_panic(DEFAULT_LOGFILE_PATH);
    logger_open(opts.log_file);
  }
}

void
logger_reinit (void) {
  struct stat fd_stat;
  struct stat file_stat;

  if (opts.log_file) {
    // Check for invalidated fd
    if (fstat(log_fd, &fd_stat) < 0) {
      logger_init();
    }

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

  if (opts.log_file) {
    free(opts.log_file);
  }
}
