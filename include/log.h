#ifndef LOG_H
#define LOG_H

#include <time.h>

#include "cli.h"

/**
 * Initializes the logger using the CLI options to determine things such as
 * where the log output goes.
 *
 * TODO: sink struct - allow different logger impls
 */
void logger_init(void);

/**
 * Reinitializes the logger, provided the log file and/or file descriptor
 * was closed or invalidated.
 */
void logger_reinit(void);

/**
 * Closes any file descriptors used by the logger.
 */
void logger_close(void);

/**
 * Printf but for logs! Prints to the log fd we've configured in
 * `logger_init`.
 *
 * @param fmt The format string
 * @param ... Everything else
 */
void printlogf(const char *fmt, ...);

#endif /* LOG_H */
