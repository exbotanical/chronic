#ifndef CLI_H
#define CLI_H

#include <stdbool.h>

/**
 * CLI configuration options.
 */
typedef struct {
  bool use_syslog;
  char* log_file;
} CliOptions;

extern CliOptions opts;

/**
 * Initializes the CLI configuration.
 */
void cli_init(int argc, char** argv);

#endif /* CLI_H */
