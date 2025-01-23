#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "api/ipc.h"
#include "cli.h"
#include "config.h"
#include "constants.h"
#include "cronentry.h"
#include "crontab.h"
#include "daemon.h"
#include "globals.h"
#include "job.h"
#include "logger.h"
#include "proginfo.h"
#include "sig.h"
#include "user.h"
#include "utils/time.h"
#include "utils/xpanic.h"
#include "workloads.h"

proginfo_t proginfo;

// Desired interval in seconds between loop iterations
const short loop_interval = 60;
hash_table* db;
array_t*    job_queue;
array_t*    mail_queue;

cli_opts opts = {0};
user_t   usr  = {0};

int
main (int argc, char** argv) {
  cli_init(argc, argv);

#ifndef VALGRIND
  // Become a daemon process
  if (daemonize() != OK) {
    xpanic("[%s@L%d] daemonize fail\n", __func__, __LINE__);
  }
#endif
  user_init();
  logger_init();

  log_info("running as %s (uid=%d, root?=%s)\n", usr.uname, usr.uid, usr.root ? "y" : "n");

  daemon_lock();
  sig_handlers_init();

  db         = ht_init_or_panic(0, (free_fn*)free_crontab);

  job_queue  = array_init_or_panic();
  mail_queue = array_init_or_panic();

  struct timespec ts;
  get_time(&ts);
  proginfo_init(&ts);

  char* s_ts = to_time_str_millis(&ts);
  log_info("cron daemon (pid=%d) started at %s\n", proginfo.pid, s_ts);
  free(s_ts);

  time_t start_time = ts.tv_sec;
  time_t current_iter_time;

  db = update_db(db, start_time, ALL_DIRS);

  reap_routine_init();
  ipc_init();

  while (true) {
    log_debug("\n%s\n", "----------------");

    // Take advantage of our sleep time to run reap routines
    signal_reap_routine();

    unsigned short sleep_dur = get_sleep_duration(loop_interval, time(NULL));
    log_debug("Sleeping for %d seconds...\n", sleep_dur);
    sleep(sleep_dur);

    current_iter_time        = time(NULL);
    time_t rounded_timestamp = round_ts(current_iter_time, loop_interval);

    char* c_ts               = to_time_str_secs(current_iter_time);
    char* r_ts               = to_time_str_secs(rounded_timestamp);
    log_debug("Current iter time: %s (rounded to %s)\n", c_ts, r_ts);
    free(c_ts);
    free(r_ts);

    try_run_jobs(db, rounded_timestamp);
    db = update_db(db, current_iter_time, ALL_DIRS);
  }

  exit(EXIT_FAILURE);
}
