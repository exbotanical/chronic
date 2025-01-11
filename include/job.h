#ifndef JOB_H
#define JOB_H

#include <time.h>

#include "cronentry.h"
#include "libhash/libhash.h"

/**
 * Represents all possible job states.
 */
typedef enum {
  /**
   * The job is ready but has not been executed yet.
   */
  PENDING,
  /**
   * The job has been executed but not awaited for a result.
   */
  RUNNING,
  /**
   * The job has executed and its result has been collected. Any child processes
   * associated with the job have been cleaned up.
   */
  EXITED
} job_state;

/**
 * Represents all possible job types.
 */
typedef enum {
  /**
   * The job represents a command specified in a crontab entry.
   */
  CRON,
  /**
   * The job represents reporting that needs to be done regarding a completed
   * cron job.
   */
  MAIL
} job_type;

/**
 * Represents a job executed by the daemon.
 */
typedef struct {
  /**
   * A unique identifier for the job (UUID v4).
   */
  char     *ident;
  /**
   * The command to be executed.
   */
  char     *cmd;
  /**
   * The process id of the job when running. Starts as -1.
   */
  pid_t     pid;
  /**
   * The current job state.
   */
  job_state state;
  /**
   * The username or email address to whom results will be reported.
   * This is set by the MAILTO variable in the corresponding crontab.
   * If MAILTO is not present, this will be set to the owning user's username.
   */
  char     *mailto;
  /**
   * The job type.
   */
  job_type  type;
  /**
   * The return status of the job, once executed. Starts as -1.
   */
  int       ret;

  time_t next_run;
} job_t;

#define X(e) [e] = #e
extern const char *job_state_names[];

/**
 * Initializes the job reap routine. This is a daemon thread that awaits job
 * child processes and cleans them up.
 * It works while the main daemon loop is paused for sleeping.
 */
void init_reap_routine(void);

/**
 * Signals the reap routine to wake up and begin processing RUNNING state jobs.
 * This encapsulates a condition variable that we leverage to ensure the reaper
 * only does work while the main program is blocked and not doing anything
 * useful.
 */
void signal_reap_routine(void);

/**
 * Iterates the crontab db and runs any job whose `next` timestamp matches the
 * current rounded timestamp.
 *
 * @param db A hash table of eligible jobs, in the form
 * HashTable<char*, crontab_t*>
 * @param ts The current rounded time.
 */
void try_run_jobs(hash_table *db, time_t ts);

/**
 * Deallocates a job with type CRON.
 */
void free_cronjob(job_t *job);

/**
 * Deallocates a job with type MAIL.
 */
void free_mailjob(job_t *job);

#endif /* JOB_H */
