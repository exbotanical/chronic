#ifndef JOB_H
#define JOB_H

#include <time.h>

#include "cronentry.h"
#include "libhash/libhash.h"

typedef enum { PENDING, RUNNING, EXITED } JobState;

typedef enum { CMD, MAIL } JobType;

typedef struct {
  char *ident;
  char *cmd;
  pid_t pid;
  JobState state;
  char *mailto;
  JobType type;
  int ret;
} Job;

void run_cronjob(CronEntry *entry);

void init_reap_routine(void);
void signal_reap_routine(void);

#endif /* JOB_H */
