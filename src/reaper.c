#include "reaper.h"

#include <stdbool.h>

#include "constants.h"
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

    printlogf("in reaper thread\n");
    foreach (job_queue, i) {
      Job* job = array_get(job_queue, i);
      // TODO: null checks everywhere
      printlogf("[job %s] Attempting to reap job with status %s\n", job->ident,
                job_status_names[job->status]);
      reap_job(job);

      // We need to be careful to only remove RESOLVED jobs, else
      // we'll end up with zombley processes
      if (job->status == RESOLVED) {
        printlogf("[job %s] Removing RESOLVED job\n", job->ident);
        array_remove(job_queue, i);
      }
    }

    pthread_mutex_unlock(&mutex);
  }

  return NULL;
}
