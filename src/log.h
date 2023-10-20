#ifndef LOG_H
#define LOG_H

#include <time.h>

void write_to_log(char *str, ...);
char *to_time_str(time_t t);

#endif /* LOG_H */
