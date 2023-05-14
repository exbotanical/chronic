#define _POSIX_C_SOURCE 199309L

/*
The Wire

The Barksdale crew is especially interesting. When Avon is in prison
and Stringer has taken over operations, Stringer is arranging faked suicides
and backdoor deals with un-friendly rivals in order to save what is a business
damaged by profound loss, or casualty, depending on how you look at it.

*/
#include "defs.h"

extern pid_t daemon_pid;
pid_t daemon_pid;

int main(int argc, char **argv) {
  if (!daemonize()) {
    printf("ERROR");
    exit(1);
  }

  int log_fd;
  if ((log_fd = open(".log", O_WRONLY | O_CREAT | O_APPEND, 0600)) < 0) {
    perror("open log");
    exit(errno);
  }

  fclose(stderr);
  dup2(log_fd, STDERR_FILENO);

  time_t t1 = time(NULL);
  time_t t2;
  long difftime;

  // Desired interval in seconds between loop iterations
  short stime = 60;

  while (true) {
    // Subtract the remainder of the current time to ensure the iteration begins
    // as close to possible to the desired interval boundary
    sleep((stime + 1) - (short)(time(NULL) % stime));

    t2 = time(NULL);

    write_to_log("pid=%d\n", getpid());
    write_to_log("%s\n", "attempting to read jobs file");

    array_t *jobs = scan_jobs("jobs");
    write_to_log("%s\n", "scanned jobs");

    foreach (jobs, i) {
      Job *job = array_get(jobs, i);

      write_to_log("executing job with code %s\n", job->code);

      if ((job->pid = fork()) == 0) {
        setpgid(0, 0);
        execl("/bin/sh", "/bin/sh", "-c", job->code, NULL);
        write_to_log("%s\n", "exec in child process");
        exit(0);
      }
    }

    write_to_log("%s\n", "sleeping...");

    foreach (jobs, i) {
      Job *job = array_get(jobs, i);
      reap(job);
    }

    foreach (jobs, i) {
      Job *job = array_get(jobs, i);
      if (job->status == EXITED) {
        write_to_log("%s\n",
                     fmt_str("Job %d has status %d", job->pid, job->ret));
      } else {
        write_to_log("Job %d is still running\n", job->pid);
      }
    }
  }
}

// close(STDIN_FILENO);
// close(STDOUT_FILENO);
// close(STDERR_FILENO);

// if (open("/dev/null", O_WRONLY) != 0) {
//   return false;
// }

// if (dup2(0, STDERR_FILENO) != STDERR_FILENO) {
//   return false;
// }

// close(0);

// become the session leader of a new process group
// so we can control all processes using the gid
// if (setsid() == -1) {
//   perror("setsid");
//   return false;
// }

// if (chdir("/") != 0) {
//   return false;
// }

// umask(0);
