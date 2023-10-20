#include "job.h"

#include "defs.h"
#include "log.h"
#include "parser.h"
#include "utils.h"

Job* new_job(char* raw, time_t curr, char* uname) {
  Job* job = xmalloc(sizeof(Job));

  if (parse(job, raw) != OK) {
    write_to_log("Failed to parse job line %s\n", raw);
    return NULL;
  }

  job->pid = -1;
  job->ret = -1;
  job->status = PENDING;
  job->owner_uname = uname;
  job->next = cron_next(job->expr, curr);

  return job;
}

void renew_job(Job* job, time_t curr) {
  write_to_log("Updating time for job %d\n", job->id);

  job->next = cron_next(job->expr, curr);
  job->status = PENDING;
}

void run_job(Job* job) {
  array_push(job_queue, job);

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

    // TODO: remove from queue, add to waiters
    job->ret = status;
    job->status = EXITED;
    job->pid = -1;
    // TODO: cleanup fn
  }
}
