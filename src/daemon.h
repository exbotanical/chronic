#ifndef DAEMON_H
#define DAEMON_H

#include "util.h"

/**
 * Daemonizes the current running process and detaches it from the TTY.
 *
 * @return Retval
 */
Retval daemonize(void);

#endif /* DAEMON_H */
