#include "cli.h"

#include "commander/commander.h"
#include "constants.h"

static void setopt_logfile(command_t* self) {
  opts.log_file = (char*)self->arg;
}

void cli_init(int argc, char** argv) {
  command_t cmd;

  command_init(&cmd, argv[0], CHRONIC_VERSION);
  command_option(&cmd, "-L", "--log-file [path]",
                 "log to specified file instead of syslog", setopt_logfile);

  command_parse(&cmd, argc, argv);
}
