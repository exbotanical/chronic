#define _BSD_SOURCE 1  // For `gethostname`

#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <unistd.h>

#include "cli.h"
#include "constants.h"
#include "cronentry.h"
#include "crontab.h"
#include "daemon.h"
#include "job.h"
#include "log.h"
#include "opt-constants.h"
#include "usr.h"
#include "util.h"

pid_t  daemon_pid;
char   hostname[SMALL_BUFFER];
user_t usr;

array_t* job_queue;
array_t* mail_queue;

const char* job_state_names[] = {X(PENDING), X(RUNNING), X(EXITED)};

// Desired interval in seconds between loop iterations
const short loop_interval     = 60;

CliOptions opts               = {0};
user_t     usr                = {0};

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

  // Become a daemon process
  if (daemonize() != OK) {
    printf("daemonize fail");
    exit(EXIT_FAILURE);
  }

  logger_init();

  usr.uid   = getuid();
  usr.root  = usr.uid == 0;
  usr.uname = getpwuid(usr.uid)->pw_name;
  printlogf(
    "running as %s (uid=%d, root=%s)\n",
    usr.uname,
    usr.uid,
    usr.root ? "y" : "n"
  );

  daemon_lock();  // TODO: check before daemonize

  setup_sig_handlers();

  job_queue         = array_init();
  mail_queue        = array_init();
  daemon_pid        = getpid();

  hash_table* db    = ht_init(0);

  time_t start_time = time(NULL);

  printlogf(
    "cron daemon (pid=%d) started at %s\n",
    daemon_pid,
    to_time_str(start_time)
  );

  time_t current_iter_time;

  DirConfig sys_dir = {.is_root = true, .path = SYS_CRONTABS_DIR};
  DirConfig usr_dir = {.is_root = false, .path = CRONTABS_DIR};

  update_db(db, start_time, &usr_dir, &sys_dir, NULL);
  init_reap_routine();

  while (true) {
    printlogf("\n----------------\n");

    // Take advantage of our sleep time to run reap routines
    signal_reap_routine();

    unsigned short sleep_dur = get_sleep_duration(loop_interval, time(NULL));
    printlogf("Sleeping for %d seconds...\n", sleep_dur);
    sleep(sleep_dur);

    current_iter_time        = time(NULL);
    time_t rounded_timestamp = round_ts(current_iter_time, loop_interval);

    printlogf(
      "Current iter time: %s (rounded to %s)\n",
      to_time_str(current_iter_time),
      to_time_str(rounded_timestamp)
    );

    run_jobs(db, rounded_timestamp);

    update_db(db, current_iter_time, &usr_dir, &sys_dir, NULL);
  }

  exit(1);
}
