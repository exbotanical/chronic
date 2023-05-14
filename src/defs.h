#include <errno.h>
#include <fcntl.h>
#include <pcre.h>
#include <signal.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include "libhash/libhash.h"
#include "libutil/libutil.h"

typedef struct {
  char *code;
  pid_t pid;
  enum { PENDING, STARTED, EXITED } status;
  int ret;
} Job;

bool daemonize();

array_t *scan_jobs(char *fpath);

void write_to_log(char *str, ...);

pcre *regex_cache_get(hash_table *cache, const char *pattern);

bool regex_match(pcre *re, char *cmp);
array_t *regex_matches(pcre *re, char *cmp);

char *str_join(array_t *sarr, const char *delim);

typedef struct {
  hash_table *variables;
  hash_table *errors;
  array_t *expressions;
} cron_config;

cron_config parse_file(char *filepath);

/**
 * xmalloc is a malloc wrapper that exits the program if out of memory
 */
void *xmalloc(size_t sz);

char *read_file(char *filepath);
