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

#define X(e) [e] = #e

#ifndef SYS_CRONTABS_DIR
#define SYS_CRONTABS_DIR "/etc/cron.d"
#endif
#ifndef CRONTABS_DIR
#define CRONTABS_DIR "/var/spool/cron/crontabs"
#endif

/*
 * Because crontab/at files may be owned by their respective users we
 * take extreme care in opening them.  If the OS lacks the O_NOFOLLOW
 * we will just have to live without it.  In order for this to be an
 * issue an attacker would have to subvert group CRON_GROUP.
 */
#ifndef O_NOFOLLOW
#define O_NOFOLLOW 0
#endif

#define ALL_PERMS 07777
#define OWNER_RW_PERMS 0600

#define ROOT_USER "root"
#define ROOT_UID 0

#define MAXENTRIES 256
#define RW_BUFFER 1024
// Basically whether we support seconds (7)
#define SPACES_BEFORE_CMD 5

typedef enum {
  PENDING,
  RUNNING,
  EXITED,
} JobStatus;

typedef enum { OK, ERR } RETVAL;

typedef struct {
  cron_expr *expr;
  char *schedule;
  char *cmd;
  char *owner_uname;
  time_t next;
  pid_t pid;
  JobStatus status;
  int ret;
  unsigned int id;
} Job;

typedef struct {
  array_t *jobs;
  time_t mtime;
} Crontab;

// TODO: check dir mtime as well?
typedef struct {
  char *name;
  bool is_root;
} DirConfig;

extern const char *job_status_names[];
extern array_t *job_queue;

#endif /* DEFS_H */
