#include "reaper.h"

#include <stdbool.h>

#include "defs.h"
#include "job.h"
#include "log.h"

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

void signal_reap_routine(void) {
  pthread_mutex_lock(&mutex);
  pthread_cond_signal(&cond);
  pthread_mutex_unlock(&mutex);
}

void* reap_routine(void* _arg) {
  while (true) {
    pthread_mutex_lock(&mutex);
    pthread_cond_wait(&cond, &mutex);

    write_to_log("in reaper thread\n");
    foreach (job_queue, i) {
      Job* job = array_get(job_queue, i);
      write_to_log("Attempting to reap job with pid %d\n", job->pid);
      reap_job(job);

      // We need to be careful to only remove EXITED jobs, else
      // we'll end up with zombley processes
      if (job->status == EXITED) {
        array_remove(job_queue, i);
      }
    }

    pthread_mutex_unlock(&mutex);
  }

  return NULL;
}
