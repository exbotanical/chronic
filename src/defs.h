#ifndef DEFS_H
#define DEFS_H

#include <fcntl.h>

#include "libhash/libhash.h"
#include "libutil/libutil.h"

#ifndef SYS_CRONTABS_DIR
#define SYS_CRONTABS_DIR "/etc/cron.d"
#endif
#ifndef CRONTABS_DIR
#define CRONTABS_DIR "/var/spool/cron/crontabs"
#endif

#define ROOT_UNAME "root"
#define ROOT_UID 0

#define ALL_PERMS 07777
#define OWNER_RW_PERMS 0600

#define CHRONIC_VERSION "0.0.1"

extern pid_t daemon_pid;
extern const char *job_status_names[];
extern array_t *job_queue;

#endif /* DEFS_H */
