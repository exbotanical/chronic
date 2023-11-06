#ifndef CLI_H
#define CLI_H

#include <stdbool.h>

typedef struct {
  bool use_syslog;
  char* log_file;
} CliOptions;

extern CliOptions opts;

void cli_init(int argc, char** argv);

#endif /* CLI_H */
