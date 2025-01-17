#include "cli.h"

#include "commander/commander.h"
#include "globals.h"
#include "panic.h"

typedef struct {
  int logopt;
} clicontext;

/**
 * Sets the log file CLI option.
 *
 * Optionally specify a path to which logs will be written.
 */
static void
setopt_logfile (command_t* self) {
  clicontext* ctx = (clicontext*)self->data;
  if (ctx->logopt > 0) {
    xpanic(
      "cannot set log file when syslog is being used. the two are mutually "
      "exclusive; choose one"
    );
  }
  ctx->logopt   = 1;

  opts.log_file = s_copy_or_panic((char*)self->arg);
}

static void
setopt_syslog (command_t* self) {
  clicontext* ctx = (clicontext*)self->data;
  if (ctx->logopt > 0) {
    xpanic(
      "cannot set syslog opt when log file is being used. the two are mutually "
      "exclusive; choose one"
    );
  }
  ctx->logopt = 1;

  opts.syslog = true;
}

void
cli_init (int argc, char** argv) {
  command_t  cmd;
  clicontext ctx;

  command_init(&cmd, argv[0], CHRONIC_VERSION);
  cmd.data   = (void*)&ctx;
  ctx.logopt = 0;

  command_option(&cmd, "-L", "--log-file [path]", "log to specified file", setopt_logfile);
  command_option(&cmd, "-S", "--syslog", "log to syslog", setopt_syslog);

  command_parse(&cmd, argc, argv);
  command_free(&cmd);
}
