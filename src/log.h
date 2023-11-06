#ifndef LOG_H
#define LOG_H

#include <time.h>

#include "cli.h"

#define LOG_IDENT "crond"

void logger_init(CliOptions *opts);
void printlogf(const char *fmt, ...);
char *to_time_str(time_t t);

#endif /* LOG_H */
