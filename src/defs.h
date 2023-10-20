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

typedef enum {
  PENDING,
  RUNNING,
  EXITED,
} JobStatus;

#define X(e) [e] = #e

extern const char *job_status_names[];

typedef struct {
  char *schedule;
  char *cmd;
  time_t next;
  cron_expr *expr;
  pid_t pid;
  JobStatus status;
  int ret;
  unsigned int id;
  char *run_id;
  char *owner_uname;
  time_t mtime;
  bool enqueued;
} Job;

typedef struct {
  time_t mtime;
  array_t *jobs;
} Crontab;

// TODO: check dir mtime as well?
typedef struct {
  char *name;
  bool is_root;
} DirConfig;

typedef enum { OK, ERR } RETVAL;

// Basically whether we support seconds (7)
#define SPACES_BEFORE_CMD 5

/**
 * xmalloc is a malloc wrapper that exits the program if out of memory
 */
void *xmalloc(size_t sz);
RETVAL daemonize();
Job *new_job(char *raw, time_t curr, char *uname);
void strip_comment(char *str);
char *until_nth_of_char(const char *str, char c, int n);
RETVAL parse_cmd(char *line, Job *job, int count);
RETVAL parse(Job *job, char *line);
bool is_comment_line(const char *str);
bool should_parse_line(const char *line);
void reap_job(Job *job);
void renew_job(Job *job, time_t curr);
char *create_uuid(void);
void write_to_log(char *str, ...);
char *to_time_str(time_t t);
array_t *get_filenames(char *dpath);
Crontab *new_crontab(int crontab_fd, bool is_root, time_t curr_time,
                     time_t mtime, char *uname);
void scan_crontabs(hash_table *db, DirConfig dir_conf, time_t curr);

// e.g. Adding 30 seconds before division rounds it to nearest minute
inline static time_t round_ts(time_t ts, short unit) {
  return (ts + unit / 2) / unit * unit;
}

inline static unsigned short get_sleep_duration(short interval, time_t now) {
  // Subtract the remainder of the current time to ensure the iteration
  // begins as close as possible to the desired interval boundary
  unsigned short sleep_time = (interval + 1) - (short)(now % interval);

  return sleep_time;
}
#endif /* DEFS_H */
