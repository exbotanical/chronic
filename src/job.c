#include "defs.h"

#define BUFFER_SIZE 255

void set_next(Job *job, time_t curr) { job->next = cron_next(job->expr, curr); }

array_t *scan_jobs(char *fpath, time_t curr) {
  array_t *lines = process_dir(fpath);
  array_t *jobs = array_init();

  foreach (lines, i) {
    char *line = array_get(lines, i);
    Job *job = malloc(sizeof(Job));

    if (!parse(job, line)) {
      write_to_log("failed to parse job line %s", line);
    }
    write_to_log("done parsing line %s\n", line);

    free(line);

    array_push(jobs, job);
    job->pid = -1;
    job->ret = -1;
    job->status = PENDING;
    set_next(job, curr);
  }

  return jobs;
}
