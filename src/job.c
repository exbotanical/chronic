#include "job.h"

#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

#include "cronentry.h"
#include "defs.h"
#include "libutil/libutil.h"
#include "log.h"
#include "util.h"

static Job* new_job(CronEntry* entry) {
  Job* job = xmalloc(sizeof(Job));
  job->ret = -1;
  job->pid = -1;
  job->status = PENDING;
  job->cmd = s_copy(entry->cmd);  // TODO: free

  return job;
}

void enqueue_job(CronEntry* entry) {
  Job* job = new_job(entry);

  if ((job->pid = fork()) == 0) {
    setpgid(0, 0);
    execl("/bin/sh", "/bin/sh", "-c", job->cmd, NULL);
    printlogf("Writing log from child process %d\n", getpid());
    exit(0);
  }

  printlogf("New running job with pid %d\n", job->pid);

  job->status = RUNNING;
  array_push(job_queue, job);
}

void reap_job(Job* job) {
  // This shouldn't happen, but just in case...
  if (job->pid == -1 || job->status == EXITED) {
    return;
  }

  int status;
  int r = waitpid(job->pid, &status, WNOHANG);

  printlogf("Job with pid=%d waitpid result is %d\n", job->pid, r);
  // -1 == error; 0 == still running; pid == dead
  if (r < 0 || r == job->pid) {
    if (r > 0 && WIFEXITED(status)) {
      status = WEXITSTATUS(status);
    } else {
      status = 1;
    }

    printlogf("set job with pid=%d to EXITED\n", job->pid);

    job->ret = status;
    job->status = EXITED;
    job->pid = -1;
    // TODO: cleanup fn
  }
}
