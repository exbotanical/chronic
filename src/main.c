#define _POSIX_C_SOURCE 199309L

/*
The Barksdale crew is especially interesting. When Avon is in prison
and Stringer has taken over operations, Stringer is arranging faked suicides
and backdoor deals with un-friendly rivals in order to save what is a business
damaged by profound loss, or casualty, depending on how you look at it.

*/
#include "defs.h"

const char* job_status_names[] = {X(PENDING), X(RUNNING), X(EXITED)};

extern pid_t daemon_pid;
pid_t daemon_pid;

array_t* job_queue;

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

  time_t start_time = time(NULL);
  time_t current_iter_time;
  // long difftime;

  // Desired interval in seconds between loop iterations
  short stime = 60;

  write_to_log("[chronic] Initialized. pid=%d\n", getpid());

  job_queue = array_init();

  write_to_log("Scanning jobs\n");

  array_t* jobs = scan_jobs("./t/fixtures/runnable", start_time);
  write_to_log("Scanned %d jobs; starting loop...\n", array_size(jobs));

  while (true) {
    // Subtract the remainder of the current time to ensure the iteration
    // begins as close as possible to the desired interval boundary
    unsigned short sleep_time = (stime + 1) - (short)(time(NULL) % stime);

    sleep(sleep_time);
    write_to_log("Sleeping for %d seconds...\n", sleep_time);

    current_iter_time = time(NULL);
    time_t rounded_timestamp =
        (current_iter_time + 30) / 60 *
        60;  // Adding 30 seconds before division rounds it

    foreach (jobs, i) {
      Job* job = array_get(jobs, i);

      write_to_log(
          "current iter time %s, rounded time %s, next time %s - "
          "code: %s, schedule: %s\n",
          to_time_str(current_iter_time), to_time_str(rounded_timestamp),
          to_time_str(job->next), job->cmd, job->schedule);

      if (job->next == rounded_timestamp) {
        run_job(job);
      }
    }

    sleep(5);

    foreach (jobs, i) {
      Job* job = array_get(jobs, i);
      reap_job(job);
    }

    foreach (jobs, i) {
      Job* job = array_get(jobs, i);
      if (job->status == EXITED) {
        write_to_log("Job %d has status %d\n", job->id, job->ret);
      } else {
        write_to_log("Job %d is still running with status %s\n", job->pid,
                     job_status_names[job->status]);
      }
    }

    update_jobs(jobs, current_iter_time);
  }
}
