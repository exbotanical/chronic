#ifndef LOGGER_H
#define LOGGER_H

#include <stdarg.h>
#include <sys/syslog.h>
#include <time.h>

#include "cli.h"

typedef enum {
  LOG_LEVEL_ERROR = LOG_ERR,
  LOG_LEVEL_WARN  = LOG_WARNING,
  LOG_LEVEL_INFO  = LOG_INFO,
  LOG_LEVEL_DEBUG = LOG_DEBUG,
} log_level;

#define log_error(fmt, ...) printlogf(LOG_LEVEL_ERROR, fmt, ##__VA_ARGS__)
#define log_warn(fmt, ...)  printlogf(LOG_LEVEL_WARN, fmt, ##__VA_ARGS__)
#define log_info(fmt, ...)  printlogf(LOG_LEVEL_INFO, fmt, ##__VA_ARGS__)
#define log_debug(fmt, ...) printlogf(LOG_LEVEL_DEBUG, fmt, ##__VA_ARGS__)

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
void printlogf(log_level lvl, const char* fmt, ...);

#endif /* LOGGER_H */
