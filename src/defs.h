#ifndef DEFS_H
#define DEFS_H

#include "libhash/libhash.h"
#include "libutil/libutil.h"

#ifndef SYS_CRONTABS_DIR
#define SYS_CRONTABS_DIR "/etc/cron.d"
#endif
#ifndef CRONTABS_DIR
#define CRONTABS_DIR "/var/spool/cron/crontabs"
#endif

#define ROOT_USER "root"
#define ROOT_UID 0

extern pid_t daemon_pid;
extern const char *job_status_names[];
extern array_t *job_queue;

#endif /* DEFS_H */
