#include "defs.h"

#define BUFFER_SIZE 255

static int id_counter = 0;

static void set_next(Job *job, time_t curr) {
  job->next = cron_next(job->expr, curr);
}

static Job *make_job(char *raw, time_t curr) {
  Job *job = xmalloc(sizeof(Job));

  if (parse(job, raw) != OK) {
    write_to_log("Failed to parse job line %s\n", raw);
  }

  free(raw);
  job->pid = -1;
  job->ret = -1;
  job->id = id_counter++;
  job->run_id = NULL;
  job->status = PENDING;

  set_next(job, curr);

  return job;
}

void update_jobs(array_t *jobs, time_t curr) {
  foreach (jobs, i) {
    Job *job = array_get(jobs, i);
    write_to_log("checking job for update %d (status %s)\n", job->id,
                 job_status_names[job->status]);

    // only set next if the job has run
    if (job->status == EXITED) {
      write_to_log("Updating time for job %d\n", job->id);

      set_next(job, curr);
      job->status = PENDING;
    }
  }
}

array_t *scan_jobs(char *fpath, time_t curr) {
  array_t *lines = process_dir(fpath);
  array_t *jobs = array_init();

  foreach (lines, i) {
    char *line = array_get(lines, i);
    // TODO: handle
    Job *job = make_job(line, curr);

    free(line);
    array_push(jobs, job);
  }

  return jobs;
}

void run_job(Job *job) {
  job->run_id = create_uuid();

  if ((job->pid = fork()) == 0) {
    setpgid(0, 0);
    execl("/bin/sh", "/bin/sh", "-c", job->cmd, NULL);
    write_to_log("Writing log from child process %d\n", job->id);
    exit(0);
  }

  write_to_log("New job pid: %d for job %d\n", job->pid, job->id);

  job->status = RUNNING;
}

void reap_job(Job *job) {
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

    job->ret = status;
    job->status = EXITED;
    job->pid = -1;
    free(job->run_id);
    job->run_id = NULL;
  }
}
