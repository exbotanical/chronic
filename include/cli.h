#ifndef CLI_H
#define CLI_H

#include <stdbool.h>

/**
 * Command-line interface configuration options.
 */
typedef struct {
  /* log file path, mutually exclusive with syslog */
  char* log_file;
  /* use syslog, mutually exclusive with specified log file */
  bool  syslog;
} cli_opts;

/**
 * Initializes the CLI configuration and parses the argument vector immediately.
 */
void cli_init(int argc, char** argv);

#endif /* CLI_H */
