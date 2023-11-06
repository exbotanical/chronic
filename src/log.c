#include "log.h"
#define _BSD_SOURCE 1

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>

#include "defs.h"
#include "libutil/libutil.h"

#define LOG_BUFFER 2048
#define SMALL_BUFFER 256
// TODO: Allow override
#define TIMESTAMP_FMT "%Y-%m-%d %H:%M:%S"
// #define TIMESTAMP_FMT "%b %e %H:%M:%S"
#define LOG_HEADER TIMESTAMP_FMT " %%s " LOG_IDENT ": "

char hostname[SMALL_BUFFER];

static bool use_syslog = false;

// Logging utils vlog, printlogf based on those in M Dillon's dcron
static void vlog(int fd, const char *fmt, va_list va) {
  char buf[LOG_BUFFER];
  // Used to omit the header in the case of multi-line logs
  static short suppress_header = 0;

  if (use_syslog) {
    vsnprintf(buf, sizeof(buf), fmt, va);
    // TODO: levels; using INFO(6) for now
    syslog(6, "%s", buf);
  } else {
    time_t ts = time(NULL);
    struct tm *ts_info = localtime(&ts);

    unsigned int buflen, headerlen = 0;
    buf[0] = 0;

    if (!suppress_header) {
      char header[SMALL_BUFFER];

      if (strftime(header, sizeof(header), LOG_HEADER, ts_info)) {
        if (gethostname(hostname, sizeof(hostname)) == 0) {
          hostname[sizeof(hostname) - 1] = 0;
        } else {
          hostname[0] = 0;
        }

        if ((headerlen = snprintf(buf, sizeof(header), header, hostname)) >=
            sizeof(header)) {
          headerlen = sizeof(header) - 1;
        }
      }
    }

    if ((buflen = vsnprintf(buf + headerlen, sizeof(buf) - headerlen, fmt, va) +
                  headerlen) >= sizeof(buf)) {
      buflen = sizeof(buf) - 1;
    }

    write(fd, buf, buflen);
    // If the log doesn't end with a newline, omit the header next time (next
    // line is a continuation of that log entry)
    suppress_header = (buf[buflen - 1] != '\n');
  }
}

void printlogf(const char *fmt, ...) {
  va_list va;

  va_start(va, fmt);
  vlog(STDERR_FILENO, fmt, va);
  va_end(va);
}

void logger_init(CliOptions *opts) {
  // TODO: handle signals
  if (opts->log_file) {
    use_syslog = false;

    int log_fd;
    if ((log_fd = open(opts->log_file, O_WRONLY | O_CREAT | O_APPEND,
                       OWNER_RW_PERMS)) >= 0) {
      fclose(stderr);
      dup2(log_fd, STDERR_FILENO);
    } else {
      perror("open");
      // fdprintf()
      exit(errno);
    }

  } else {
    use_syslog = true;

    fclose(stderr);
    // Redirect stderr to stdout, which itself has been redirected (in
    // daemonize) to /dev/null - thus 2> /dev/null
    dup2(STDOUT_FILENO, STDERR_FILENO);

    // TODO: just learn about this...
    // TODO: check if syslog, else try journal, and finally LOG_CONS
    openlog(LOG_IDENT, LOG_CONS | LOG_PID, LOG_CRON);
  }
}

char *to_time_str(time_t t) {
  char msg[512];
  struct tm *time_info = localtime(&t);
  strftime(msg, sizeof(msg), TIMESTAMP_FMT, time_info);

  return fmt_str("%s", msg);
}
