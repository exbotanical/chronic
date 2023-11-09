#include "job.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

#include "cronentry.h"
#include "defs.h"
#include "libutil/libutil.h"
#include "log.h"
#include "util.h"

// TODO: pass entry by value? Otherwise if entry is freed before we call fork,
// we're fucked
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

  char* home = ht_get(entry->parent->vars, HOMEDIR_ENVVAR);
  char* shell = ht_get(entry->parent->vars, SHELL_ENVVAR);

  if ((job->pid = fork()) == 0) {
    printlogf(
        "Writing log from child process pid=%d homedir=%s shell=%s cmd=%s\n",
        getpid(), home, shell, job->cmd);

    chdir(home);

    int r = execle(shell, shell, "-c", job->cmd, NULL, entry->parent->envp);
    printlogf("execle finished with %d\n", r);
    perror("execle");
    _exit(1);
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

    printlogf("set job with pid=%d to EXITED (status=%d)\n", job->pid, status);

    job->ret = status;
    job->status = EXITED;
    job->pid = -1;
    // TODO: cleanup fn
  }
}
