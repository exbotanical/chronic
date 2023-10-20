#define _POSIX_C_SOURCE 199309L

#include <fcntl.h>

#include "defs.h"
#define ROOT_UID 0

/*
 * Because crontab/at files may be owned by their respective users we
 * take extreme care in opening them.  If the OS lacks the O_NOFOLLOW
 * we will just have to live without it.  In order for this to be an
 * issue an attacker would have to subvert group CRON_GROUP.
 */
#ifndef O_NOFOLLOW
#define O_NOFOLLOW 0
#endif

typedef struct {
  time_t mtime;
  char* name;
  bool is_root;
} DirConfig;

typedef struct {
  time_t mtime;
  array_t* jobs;
} Crontab;

extern pid_t daemon_pid;
pid_t daemon_pid;

const char* job_status_names[] = {X(PENDING), X(RUNNING), X(EXITED)};

// Desired interval in seconds between loop iterations
const short loop_interval = 60;

array_t* job_queue;

// e.g. Adding 30 seconds before division rounds it to nearest minute
static inline time_t round_ts(time_t ts, short unit) {
  return (ts + unit / 2) / unit * unit;
}

static void sleep_for(short interval) {
  // Subtract the remainder of the current time to ensure the iteration
  // begins as close as possible to the desired interval boundary
  unsigned short sleep_time = (interval + 1) - (short)(time(NULL) % interval);

  write_to_log("Sleeping for %d seconds...\n", sleep_time);
  sleep(sleep_time);
}

int get_crontab_fd_if_valid(char* fpath, char* uname, time_t last_mtime) {
  int not_ok = OK - 1;
  struct passwd* pw = NULL;
  int crontab_fd = not_ok;
  struct stat statbuf;

  // No user found in passwd
  if ((pw = getpwnam(uname)) == NULL) {
    write_to_log("user %s not found\n", uname);
    goto done;
  }

  // Open in readonly, non-blocking mode. Don't follow symlinks.
  // Not following symlinks is largely for security
  if ((crontab_fd = open(fpath, O_RDONLY | O_NONBLOCK | O_NOFOLLOW, 0)) < OK) {
    write_to_log("cannot read %s\n", fpath);
    goto done;
  }

  if (fstat(crontab_fd, &statbuf) < OK) {
    write_to_log("fstat failed for fd %d (file %s)\n", crontab_fd, fpath);
    goto done;
  }

  if (last_mtime >= statbuf.st_mtime) {
    write_to_log("file has not changed (last mtime: %ld, curr mtime: %ld)\n",
                 fpath, last_mtime, statbuf.st_mtime);
    goto done;
  }

  if (!S_ISREG((&statbuf)->st_mode)) {
    write_to_log("file %s not regular");
    goto done;
  }

  if ((statbuf.st_mode & 07777) != 0600) {
    write_to_log("file %s has invalid permissions (r/w by owner only)");
    goto done;
  }

  // For non-root, check that the given user matches the file owner
  if (statbuf.st_uid != ROOT_UID &&
      (pw == NULL || statbuf.st_uid != pw->pw_uid ||
       strcmp(uname, pw->pw_name) != 0)) {
    write_to_log("file %s not owned by specified user %s (it's owned by %s)\n",
                 fpath, uname, pw->pw_name);
    goto done;
  }

  // Ensure there aren't any hard links
  if (statbuf.st_nlink != 1) {
    write_to_log("file %s has hard symlinks (%d)\n", fpath, statbuf.st_nlink);
    goto done;
  }

done:
  return crontab_fd;
}

static Job* make_job(char* raw, time_t curr, char* uname) {
  Job* job = xmalloc(sizeof(Job));

  if (parse(job, raw) != OK) {
    write_to_log("Failed to parse job line %s\n", raw);
    return NULL;
  }

  job->pid = -1;
  job->ret = -1;
  job->status = PENDING;
  job->owner_uname = uname;
  job->enqueued = false;

  set_next(job, curr);

  return job;
}

static array_t* process_crontab(int crontab_fd, bool is_root, time_t curr_time,
                                char* uname) {
  FILE* file;

  if (!(file = fdopen(crontab_fd, "r"))) {
    write_to_log("fdopen on crontab_fd %d failed\n", crontab_fd);
    // TODO: perrors
    return NULL;
  }

  unsigned int max_entries;
  unsigned int max_lines;

  if (is_root) {
    max_entries = 65535;
  } else {
    max_entries = MAXENTRIES;
  }

  max_lines = max_entries * 10;

  FILE* fd;
  char buf[RW_BUFFER];

  array_t* jobs = array_init();
  while (fgets(buf, sizeof(buf), fd) != NULL && --max_lines) {
    char* ptr = buf;
    int len;

    // Skip whitespace and newlines
    while (*ptr == ' ' || *ptr == '\t' || *ptr == '\n') ++ptr;

    // Remove trailing newline
    len = strlen(ptr);
    if (len && ptr[len - 1] == '\n') ptr[--len] = 0;

    // If that's it or the entire line is a comment, skip
    if (*ptr == 0 || *ptr == '#') continue;

    if (--max_entries == 0) break;

    // TODO: handle
    Job* job = make_job(ptr, curr_time, uname);
    array_push(jobs, job);
  }

  fclose(fd);

  return jobs;
}

// We HAVE to make a brand new db each time, else we will not be able to tell if
// a file was deleted
static void scan_jobs(hash_table* db, DirConfig _sys_dir, DirConfig usr_dir) {
  array_t* fnames = get_filenames(usr_dir.name);

  hash_table* new_db = ht_init(0);

  foreach (fnames, i) {
    // TODO: get actual username
    char* fname = array_get(fnames, i);
    Crontab* fc = ht_get(db, fname);

    char* fpath;
    if (!(fpath = fmt_str("%s/%s", usr_dir.name, fname))) {
      write_to_log("failed to concatenate as %s/%s\n", usr_dir.name, fname);
      continue;
    }

    int crontab_fd;
    if (!fc) {
      if ((crontab_fd = get_crontab_fd_if_valid(fpath, fname, 0)) < OK) {
        continue;
      }

      fc = malloc(sizeof(Crontab));
      fc->jobs = array_init();
      fc->mtime = 0;

      // TODO: free existing values
      ht_insert(new_db, fname, fc);
    } else {
      if ((crontab_fd = get_crontab_fd_if_valid(fpath, fname, fc->mtime)) <
          OK) {
        continue;
      }
    }

    fc->jobs = process_crontab(crontab_fd, false, 0, fname);
  }

  // TODO: free
  db = new_db;
}

static void run_job(Job* job) {
  array_push(job_queue, job);

  job->enqueued = true;
  job->run_id = create_uuid();

  if ((job->pid = fork()) == 0) {
    setpgid(0, 0);
    execl("/bin/sh", "/bin/sh", "-c", job->cmd, NULL);
    write_to_log("Writing log from child process %d\n", job->id);
    exit(0);
  }

  write_to_log("New job pid: %d for job %d\n", job->pid, job->id);

  // ONLY free enqueued
  job->status = RUNNING;
}

static void reap() {
  foreach (job_queue, i) {
    Job* job = array_get(job_queue, i);
    reap_job(job);
  }
}

int main(int _argc, char** _argv) {
  // Become a daemon process
  if (daemonize() != OK) {
    printf("daemonize fail");
    exit(EXIT_FAILURE);
  }

  // tmp
  int log_fd;
  if ((log_fd = open(".log", O_WRONLY | O_CREAT | O_APPEND, 0600)) < 0) {
    perror("open log");
    exit(errno);
  }

  close(STDERR_FILENO);
  dup2(log_fd, STDERR_FILENO);

  job_queue = array_init();
  hash_table* db = ht_init(0);

  time_t start_time = time(NULL);
  time_t current_iter_time;

  DirConfig sys = {.is_root = true, .mtime = 0, .name = SYS_CRONTABS_DIR};
  DirConfig usr = {.is_root = false, .mtime = 0, .name = CRONTABS_DIR};

  while (true) {
    current_iter_time = time(NULL);

    time_t rounded_timestamp = round_ts(current_iter_time, loop_interval);

    sleep_for(loop_interval);
    scan_jobs(db, sys, usr);

    for (unsigned int i = 0; i < db->capacity; i++) {
      Crontab* ct = db->records[i]->value;
      foreach (ct->jobs, i) {
        Job* job = array_get(ct->jobs, i);
        printf("job: %s\n", job->cmd);

        if (job->next == rounded_timestamp) {
          run_job(job);
        }
      }
    }

    sleep(5);

    reap();
  }
}

/*
The Barksdale crew is especially interesting. When Avon is in prison
and Stringer has taken over operations, Stringer is arranging faked suicides
and backdoor deals with un-friendly rivals in order to save what is a business
damaged by profound loss, or casualty, depending on how you look at it.

*/
