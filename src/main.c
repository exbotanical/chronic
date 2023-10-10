#define _POSIX_C_SOURCE 199309L

/*
The Barksdale crew is especially interesting. When Avon is in prison
and Stringer has taken over operations, Stringer is arranging faked suicides
and backdoor deals with un-friendly rivals in order to save what is a business
damaged by profound loss, or casualty, depending on how you look at it.

*/
#include "defs.h"

extern pid_t daemon_pid;
pid_t daemon_pid;

int main(int _argc, char** _argv) {
  // Become a daemon process
  if (daemonize() != 0) {
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

  write_to_log("log init\n");

  time_t start_time = time(NULL);
  time_t current_iter_time;
  long difftime;

  // Desired interval in seconds between loop iterations
  short stime = 60;

  write_to_log("setup. starting cron loop\n");

  while (true) {
    // Subtract the remainder of the current time to ensure the iteration
    // begins as close as possible to the desired interval boundary
    sleep((stime + 1) - (short)(time(NULL) % stime));

    current_iter_time = time(NULL);

    write_to_log("pid=%d\n", getpid());
    write_to_log("%s\n", "attempting to read jobs file");

    array_t* jobs = scan_jobs("./t/fixtures/valid", current_iter_time);
    write_to_log("scanned %d jobs\n", array_size(jobs));

    //   foreach (jobs, i) {
    //     Job *job = array_get(jobs, i);

    //     write_to_log("executing job with code %s\n", job->cmd);

    //     time_t rounded_timestamp =
    //         (current_iter_time + 30) / 60 *
    //         60;  // Adding 30 seconds before division rounds it

    //     write_to_log("comparing current iter time %d to next time %d",
    //                  current_iter_time, job->next);

    //     if (job->next == rounded_timestamp) {
    //       if ((job->pid = fork()) == 0) {
    //         setpgid(0, 0);
    //         execl("/bin/sh", "/bin/sh", "-c", job->cmd, NULL);
    //         write_to_log("%s\n", "exec in child process");
    //         exit(0);
    //       }
    //     }
    //   }

    //   write_to_log("%s\n", "sleeping...");

    //   foreach (jobs, i) {
    //     Job *job = array_get(jobs, i);
    //     reap(job);
    //   }

    //   foreach (jobs, i) {
    //     Job *job = array_get(jobs, i);
    //     if (job->status == EXITED) {
    //       write_to_log("%s\n",
    //                    fmt_str("Job %d has status %d", job->pid, job->ret));
    //     } else {
    //       write_to_log("Job %d is still running\n", job->pid);
    //     }
    // }
  }
}
