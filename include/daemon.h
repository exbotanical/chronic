#ifndef DAEMON_H
#define DAEMON_H

#include "utils/retval.h"

/**
 * Daemonizes the current running process and detaches it from the TTY.
 *
 * @return retval_t
 */
retval_t daemonize(void);

/**
 * Creates a lockfile to deduplicate daemon instances and ensure a singleton
 * process. Fast-fails if the lockfile already exists.
 *
 * Note: Different lockfiles will exist depending on whether the user starts
 * this program as root. If a specific non-root user, the lockfile will use
 * their uname as stored in the user database.
 */
void daemon_lock(void);

/**
 * Shuts down the daemon and calls cleanup functions.
 * This is effectively our program exit routine.
 */
void daemon_shutdown(void);

#endif /* DAEMON_H */
