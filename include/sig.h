#ifndef SIG_H
#define SIG_H

/**
 * Sets up signal handlers e.g. SIGINT/SIGTERM so we can manage the daemon
 * process.
 */
void sig_handlers_init(void);

#endif /* SIG_H */
