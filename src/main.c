#define _BSD_SOURCE 1  // For `gethostname`

#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "api/ipc.h"
#include "cli.h"
#include "config.h"
#include "constants.h"
#include "cronentry.h"
#include "crontab.h"
#include "daemon.h"
#include "job.h"
#include "log.h"
#include "panic.h"
#include "usr.h"
#include "util.h"

pid_t  daemon_pid;
char   hostname[SMALL_BUFFER];
user_t usr;

hash_table* db;
array_t*    job_queue;
array_t*    mail_queue;

// Desired interval in seconds between loop iterations
const short loop_interval = 60;

cli_opts opts             = {0};
user_t   usr              = {0};

static void
set_hostname (void) {
  if (gethostname(hostname, sizeof(hostname)) == 0) {
    hostname[sizeof(hostname) - 1] = 0;
  } else {
    hostname[0] = 0;
  }
}

int
main (int argc, char** argv) {
  cli_init(argc, argv);
  set_hostname();

#ifndef VALGRIND
  // Become a daemon process
  if (daemonize() != OK) {
    panic("[%s@L%d] daemonize fail\n", __func__, __LINE__);
  }
#endif

  logger_init();

  usr.uid   = getuid();
  usr.root  = usr.uid == 0;
  usr.uname = getpwuid(usr.uid)->pw_name;
  printlogf("running as %s (uid=%d, root?=%s)\n", usr.uname, usr.uid, usr.root ? "y" : "n");

  daemon_lock();  // TODO: check before daemonize
  setup_sig_handlers();

  db                = ht_init_or_panic(0, (free_fn*)free_crontab);

  job_queue         = array_init_or_panic();
  mail_queue        = array_init_or_panic();

  daemon_pid        = getpid();

  time_t start_time = time(NULL);

  char* s_ts        = to_time_str(start_time);
  printlogf("cron daemon (pid=%d) started at %s\n", daemon_pid, s_ts);
  free(s_ts);

  time_t current_iter_time;

  dir_config sys_dir = {.is_root = true, .path = SYS_CRONTABS_DIR};
  dir_config usr_dir = {.is_root = false, .path = CRONTABS_DIR};

  db                 = update_db(db, start_time, &usr_dir, &sys_dir, NULL);

  init_reap_routine();
  init_ipc_server();

  while (true) {
    printlogf("\n----------------\n");

    // Take advantage of our sleep time to run reap routines
    signal_reap_routine();

    unsigned short sleep_dur = get_sleep_duration(loop_interval, time(NULL));
    printlogf("Sleeping for %d seconds...\n", sleep_dur);
    sleep(sleep_dur);

    current_iter_time        = time(NULL);
    time_t rounded_timestamp = round_ts(current_iter_time, loop_interval);

    char* c_ts               = to_time_str(current_iter_time);
    char* r_ts               = to_time_str(rounded_timestamp);
    printlogf("Current iter time: %s (rounded to %s)\n", c_ts, r_ts);
    free(c_ts);
    free(r_ts);

    try_run_jobs(db, rounded_timestamp);
    db = update_db(db, current_iter_time, &usr_dir, &sys_dir, NULL);
  }

  exit(EXIT_FAILURE);
}
