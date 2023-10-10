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

typedef enum { EXITED, PENDING } JobStatus;

typedef struct {
  char *schedule;
  char *cmd;
  time_t next;
  cron_expr *expr;
  pid_t *pid;
  JobStatus status;
  int ret;
} Job;

typedef enum { OK, ERR } RETVAL;

// Basically whether we support seconds (7)
#define SPACES_BEFORE_CMD 6

bool daemonize();

array_t *scan_jobs(char *fpath, time_t curr);

void write_to_log(char *str, ...);

pcre *regex_cache_get(hash_table *cache, const char *pattern);

bool regex_match(pcre *re, char *cmp);
array_t *regex_matches(pcre *re, char *cmp);

/**
 * xmalloc is a malloc wrapper that exits the program if out of memory
 */
void *xmalloc(size_t sz);

array_t *process_dir(char *dpath);
char *read_file(char *filepath);
bool parse(Job *job, char *line);
void reap(Job *job);

#endif /* DEFS_H */
