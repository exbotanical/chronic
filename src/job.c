#include "defs.h"

#define BUFFER_SIZE 255

static int id_counter = 0;

static void set_next(Job *job, time_t curr) {
  job->next = cron_next(job->expr, curr);
}

static Job *make_job(char *raw, time_t curr) {
  Job *job = xmalloc(sizeof(Job));

  if (parse(job, raw) != OK) {
    write_to_log("Failed to parse job line %s\n", raw);
  }

  job->pid = -1;
  job->ret = -1;
  job->id = id_counter++;
  job->run_id = NULL;
  job->status = PENDING;
  job->owner_path = NULL;

  set_next(job, curr);

  return job;
}

static inline bool ts_cmp(struct timespec a, struct timespec b) {
  return a.tv_sec == b.tv_sec && a.tv_nsec == b.tv_nsec;
}

static bool is_ge(time_t a, time_t b) { return a >= b; }

void update_jobs(array_t *jobs, time_t curr) {
  foreach (jobs, i) {
    Job *job = array_get(jobs, i);
    write_to_log("checking job for update %d (status %s)\n", job->id,
                 job_status_names[job->status]);

    struct stat statbuf;
    if (stat(job->owner_path, &statbuf) < OK) {
      write_to_log("failed to stat crontabs file %s\n", job->owner_path);
    }

    write_to_log("file %s -> file.mtime: %ld, start_time: %ld\n",
                 job->owner_path, statbuf.st_mtime, job->mtime);
    if (is_ge(statbuf.st_mtime, job->mtime)) {
      write_to_log("crontab %s has been modified\n", job->owner_path);
      job->mtime = statbuf.st_mtime;
    }

    // only set next if the job has run
    if (job->status == EXITED) {
      write_to_log("Updating time for job %d\n", job->id);

      set_next(job, curr);
      job->status = PENDING;
    }
  }
}

array_t *scan_jobs(char *dpath, time_t curr) {
  DIR *dir;
  array_t *jobs = array_init();

  if ((dir = opendir(dpath)) != NULL) {
    struct dirent *den;

    while ((den = readdir(dir)) != NULL) {
      // Skip pointer files
      if (s_equals(den->d_name, ".") || s_equals(den->d_name, "..")) {
        continue;
      }

      char *path;
      if (!(path = fmt_str("%s/%s", dpath, den->d_name))) {
        write_to_log("UH OH\n");

        return NULL;
      }

      // TODO: perms check
      // TODO: root vs user
      unsigned int max_entries;
      unsigned int max_lines;

      if (strcmp(den->d_name, ROOT_USER) == 0) {
        max_entries = 65535;
      } else {
        max_entries = MAXENTRIES;
      }

      max_lines = max_entries * 10;

      FILE *fd;
      char buf[RW_BUFFER];

      struct stat statbuf;
      if (stat(path, &statbuf) < OK) {
        // TODO: handle
        continue;
      }

      if ((fd = fopen(path, "r")) != NULL) {
        while (fgets(buf, sizeof(buf), fd) != NULL && --max_lines) {
          char *ptr = buf;
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
          Job *job = make_job(ptr, curr);
          job->owner_path = path;
          job->mtime = statbuf.st_mtime;
          array_push(jobs, job);
        }
      }

      fclose(fd);
    }

    closedir(dir);

  } else {
    write_to_log("unable to scan directory %s\n", dpath);
  }

  return jobs;
}

void run_job(Job *job) {
  job->run_id = create_uuid();

  if ((job->pid = fork()) == 0) {
    setpgid(0, 0);
    execl("/bin/sh", "/bin/sh", "-c", job->cmd, NULL);
    write_to_log("Writing log from child process %d\n", job->id);
    exit(0);
  }

  write_to_log("New job pid: %d for job %d\n", job->pid, job->id);

  job->status = RUNNING;
}

void reap_job(Job *job) {
  write_to_log("Running reap routine for job %d...\n", job->id);
  if (job->pid == -1) {
    return;
  }

  write_to_log("Attempting to reap process for job %d\n", job->id);

  int status;
  int r = waitpid(job->pid, &status, WNOHANG);

  write_to_log("Job %d waitpid result is %d (pid=%d)\n", job->id, r, job->pid);
  // -1 == error; 0 == still running; pid == dead
  if (r < 0 || r == job->pid) {
    if (r > 0 && WIFEXITED(status))
      status = WEXITSTATUS(status);
    else
      status = 1;

    job->ret = status;
    job->status = EXITED;
    job->pid = -1;
    free(job->run_id);
    job->run_id = NULL;
  }
}
