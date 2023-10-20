#define _POSIX_C_SOURCE 199309L

#include <fcntl.h>

#include "defs.h"

extern pid_t daemon_pid;
pid_t daemon_pid;

const char* job_status_names[] = {X(PENDING), X(RUNNING), X(EXITED)};

// Desired interval in seconds between loop iterations
const short loop_interval = 60;

array_t* job_queue;

void reap_job(Job* job) {
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
    // TODO: cleanup fn
  }
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
  write_to_log("cron daemon started at %ld...\n", start_time);

  time_t current_iter_time;

  DirConfig sys = {.is_root = true, .name = SYS_CRONTABS_DIR};
  DirConfig usr = {.is_root = false, .name = CRONTABS_DIR};

  while (true) {
    current_iter_time = time(NULL);

    time_t rounded_timestamp = round_ts(current_iter_time, loop_interval);

    unsigned short sleep_dur = get_sleep_duration(loop_interval, time(NULL));
    write_to_log("Sleeping for %d seconds...\n", sleep_dur);
    sleep(sleep_dur);

    scan_crontabs(db, sys, usr, current_iter_time);

    for (unsigned int i = 0; i < (unsigned int)db->capacity; i++) {
      Crontab* ct = db->records[i]->value;
      foreach (ct->jobs, i) {
        Job* job = array_get(ct->jobs, i);
        write_to_log("[dbg] Have job: %s\n", job->cmd);

        if (job->next == rounded_timestamp) {
          run_job(job);
        }
      }
    }

    sleep(5);

    foreach (job_queue, i) {
      Job* job = array_get(job_queue, i);
      reap_job(job);
    }
  }
}

// The Barksdale crew is especially interesting. When Avon is in prison
// and Stringer has taken over operations, Stringer is arranging faked suicides
// and backdoor deals with un-friendly rivals in order to save what is a
// business damaged by profound loss, or casualty, depending on how you look at
// it.
