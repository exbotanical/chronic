#include "defs.h"

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
  job->enqueued = false;
  job->next = cron_next(job->expr, curr);

  return job;
}

void renew_jobs(array_t* jobs, time_t curr) {
  foreach (jobs, i) {
    Job* job = array_get(jobs, i);

    write_to_log("Updating time for job %d\n", job->id);

    job->next = cron_next(job->expr, curr);
    job->status = PENDING;
  }
}

void renew_job(Job* job, time_t curr) {
  write_to_log("Updating time for job %d\n", job->id);

  job->next = cron_next(job->expr, curr);
  job->status = PENDING;
}
