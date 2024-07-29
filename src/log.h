#ifndef LOG_H
#define LOG_H

#include <time.h>

#include "cli.h"

/**
 * Initializes the logger using the CLI options to determine things such as
 * where the log output goes.
 */
void logger_init();

/**
 * Printf but for logs! Prints to the log fd we've configured in
 * `logger_init`.
 *
 * @param fmt The format string
 * @param ... Everything else
 */
void printlogf(const char *fmt, ...);

/**
 * Closes any file descriptors used by the logger.
 */
void logger_close(void);

#endif /* LOG_H */
