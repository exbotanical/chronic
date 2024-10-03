#include "cli.h"

#include "commander/commander.h"
#include "constants.h"
#include "panic.h"

/**
 * Sets the log file CLI option.
 *
 * Optionally specify a path to which logs will be written.
 */
static void
setopt_logfile (command_t* self) {
  opts.log_file = s_copy_or_panic((char*)self->arg);
}

void
cli_init (int argc, char** argv) {
  command_t cmd;

  command_init(&cmd, argv[0], CHRONIC_VERSION);
  command_option(&cmd, "-L", "--log-file [path]", "log to specified file", setopt_logfile);

  command_parse(&cmd, argc, argv);
  command_free(&cmd);
}
