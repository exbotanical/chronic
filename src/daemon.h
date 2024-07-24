#ifndef DAEMON_H
#define DAEMON_H

#include "util.h"

/**
 * Daemonizes the current running process and detaches it from the TTY.
 *
 * @return Retval
 */
Retval daemonize(void);

// TODO: docs
void daemon_lock(void);

void setup_sig_handlers(void);

#endif /* DAEMON_H */
