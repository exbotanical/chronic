#ifndef JOB_H
#define JOB_H

#include "defs.h"

Job *new_job(char *raw, time_t curr, char *uname);

void renew_job(Job *job, time_t curr);

void reap_job(Job *job);

// e.g. Adding 30 seconds before division rounds it to nearest minute
void run_job(Job *job);

#endif /* JOB_H */
