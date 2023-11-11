#ifndef JOB_H
#define JOB_H

#include <time.h>

#include "cronentry.h"
#include "libhash/libhash.h"

typedef enum { PENDING, RUNNING, EXITED, MAIL_RUNNING, RESOLVED } JobStatus;

typedef struct {
  char *cmd;
  pid_t pid;
  pid_t mailer_pid;
  JobStatus status;
  int ret;
  char *mailto;
  char *ident;
} Job;

void reap_job(Job *job);

void enqueue_job(CronEntry *entry);

void run_mailjob(Job *mailjob);

#endif /* JOB_H */
