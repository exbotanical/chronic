#define _POSIX_C_SOURCE 199309L

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "crontab.h"
#include "daemon.h"
#include "defs.h"
#include "job.h"
#include "log.h"
#include "utils.h"

extern pid_t daemon_pid;
pid_t daemon_pid;

const char* job_status_names[] = {X(PENDING), X(RUNNING), X(EXITED)};
// Desired interval in seconds between loop iterations
const short loop_interval = 60;

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

  job_queue = array_init();
  hash_table* db = ht_init(0);

  time_t start_time = time(NULL);
  write_to_log("cron daemon started at %ld...\n", start_time);

  time_t current_iter_time;

  DirConfig sys = {.is_root = true, .name = SYS_CRONTABS_DIR};
  DirConfig usr = {.is_root = false, .name = CRONTABS_DIR};

  while (true) {
    write_to_log("\n----------------\n");
    current_iter_time = time(NULL);

    // TODO: sys

    // CURR: 2023-10-21 15:06:28
    // ROUND: 2023-10-21 15:06:00
    // NEXT: 2023-10-21 15:07:00

    // SLEEP: 33
    // RUN
    // SLEEP 5

    // CURR: 2023-10-21 15:07:06
    // ROUND: 2023-10-21 15:07:00
    // NEXT: 2023-10-21 15:08:00

    // SLEEP: 55
    // RUN
    // SLEEP 5

    // CURR: 2023-10-21 15:08:06
    // ...

    // SLEEP: 55
    // RUN
    // SLEEP 5

    write_to_log("CURRENT ITER: %s\n", to_time_str(current_iter_time));
    scan_crontabs(db, usr, current_iter_time);

    time_t rounded_timestamp = round_ts(current_iter_time, loop_interval);

    unsigned short sleep_dur = get_sleep_duration(loop_interval, time(NULL));
    write_to_log("Sleeping for %d seconds...\n", sleep_dur);
    sleep(sleep_dur);

    // We must iterate the capacity here because hash table records are not
    // stored contiguously
    for (unsigned int i = 0; i < (unsigned int)db->capacity; i++) {
      ht_record* r = db->records[i];
      // If there's no record in this slot, continue
      if (!r) {
        continue;
      }

      Crontab* ct = r->value;

      foreach (ct->jobs, i) {
        Job* job = array_get(ct->jobs, i);
        write_to_log("[dbg] Have job: %s\n", job->cmd);

        write_to_log("NEXT: %s, CURR: %s\n", to_time_str(job->next),
                     to_time_str(rounded_timestamp));

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
