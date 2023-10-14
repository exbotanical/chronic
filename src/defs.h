#ifndef DEFS_H
#define DEFS_H

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <pcre.h>
#include <pwd.h>
#include <signal.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include "ccronexpr/ccronexpr.h"
#include "libhash/libhash.h"
#include "libutil/libutil.h"

#ifndef SYS_CRONTABS
#define SYS_CRONTABS "/etc/cron.d"
#endif
#ifndef CRONTABS
#define CRONTABS "/var/spool/cron/crontabs"
#endif

#define MAXENTRIES 256
#define RW_BUFFER 1024

typedef enum {
  PENDING,
  RUNNING,
  EXITED,
} JobStatus;

#define X(e) [e] = #e

static const char *job_status_names[] = {X(PENDING), X(RUNNING), X(EXITED)};

typedef struct {
  char *schedule;
  char *cmd;
  time_t next;
  cron_expr *expr;
  pid_t *pid;
  JobStatus status;
  int ret;
  unsigned int id;
  char *run_id;
} Job;

typedef enum { OK, ERR } RETVAL;

// Basically whether we support seconds (7)
#define SPACES_BEFORE_CMD 5

/**
 * xmalloc is a malloc wrapper that exits the program if out of memory
 */
void *xmalloc(size_t sz);
RETVAL daemonize();
array_t *process_dir(char *dpath);
RETVAL parse(Job *job, char *line);
void reap_job(Job *job);
array_t *scan_jobs(char *fpath, time_t curr);
void update_jobs(array_t *jobs, time_t curr);
void run_job(Job *job);
char *create_uuid(void);
void write_to_log(char *str, ...);
char *to_time_str(time_t *t);

#endif /* DEFS_H */
