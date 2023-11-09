#ifndef JOB_H
#define JOB_H

#include <time.h>

#include "cronentry.h"
#include "libhash/libhash.h"

typedef enum {
  PENDING,
  RUNNING,
  EXITED,
} JobStatus;

typedef struct {
  char *cmd;
  pid_t pid;
  JobStatus status;
  int ret;
} Job;

void reap_job(Job *job);

void enqueue_job(CronEntry *entry);

#endif /* JOB_H */
