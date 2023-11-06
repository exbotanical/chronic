#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <unistd.h>

#include "cli.h"
#include "cronentry.h"
#include "crontab.h"
#include "daemon.h"
#include "defs.h"
#include "job.h"
#include "log.h"
#include "reaper.h"
#include "util.h"

const char* job_status_names[] = {X(PENDING), X(RUNNING), X(EXITED)};
// Desired interval in seconds between loop iterations
const short loop_interval = 60;

pid_t daemon_pid;
array_t* job_queue;

CliOptions opts = {0};

int main(int argc, char** argv) {
  cli_init(argc, argv);

  // TODO: check if instance already running

  // Become a daemon process
  if (daemonize() != OK) {
    printf("daemonize fail");
    exit(EXIT_FAILURE);
  }

  // TODO: handle signals
  logger_init(&opts);

  job_queue = array_init();
  daemon_pid = getpid();

  hash_table* db = ht_init(0);

  time_t start_time = time(NULL);

  printlogf("cron daemon (pid=%d) started at %s\n", daemon_pid,
            to_time_str(start_time));

  time_t current_iter_time;

  DirConfig sys = {.is_root = true, .name = SYS_CRONTABS_DIR};
  DirConfig usr = {.is_root = false, .name = CRONTABS_DIR};

  update_db(db, start_time, &usr, NULL);
  // TODO: sys
  pthread_t reaper_thread_id;
  pthread_create(&reaper_thread_id, NULL, &reap_routine, NULL);

  while (true) {
    printlogf("\n----------------\n");

    // Take advantage of our sleep time to run reap routines
    signal_reap_routine();

    unsigned short sleep_dur = get_sleep_duration(loop_interval, time(NULL));
    printlogf("Sleeping for %d seconds...\n", sleep_dur);
    sleep(sleep_dur);

    current_iter_time = time(NULL);
    time_t rounded_timestamp = round_ts(current_iter_time, loop_interval);

    printlogf("Current iter time: %s (rounded to %s)\n",
              to_time_str(current_iter_time), to_time_str(rounded_timestamp));

    // We must iterate the capacity here because hash table records are not
    // stored contiguously
    if (db->count > 0) {
      for (unsigned int i = 0; i < (unsigned int)db->capacity; i++) {
        ht_record* r = db->records[i];

        // If there's no record in this slot, continue
        if (!r) {
          continue;
        }

        Crontab* ct = r->value;
        foreach (ct->entries, i) {
          CronEntry* entry = array_get(ct->entries, i);
          printlogf("[dbg] Have entry: %s. Next %s == Curr %s\n", entry->cmd,
                    to_time_str(entry->next), to_time_str(rounded_timestamp));

          if (entry->next == rounded_timestamp) {
            enqueue_job(entry);
          }
        }
      }
    }

    update_db(db, current_iter_time, &usr, NULL);
  }

  printlogf("AFTER THE LOOP\n");
  exit(1);
}
