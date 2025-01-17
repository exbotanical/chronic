#include "cronentry.h"

#include <stdlib.h>
#include <string.h>

#include "logger.h"
#include "panic.h"
#include "parser.h"
#include "util.h"

static unsigned int id_counter = 0;

static inline char*
get_cadence (cadence_t cadence) {
  switch (cadence) {
    case CADENCE_HOURLY: return HOURLY_EXPR;
    case CADENCE_DAILY: return DAILY_EXPR;
    case CADENCE_WEEKLY: return WEEKLY_EXPR;
    case CADENCE_MONTHLY: return MONTHLY_EXPR;
    case CADENCE_NA: return NULL;
    default: {
      xpanic("unhandled cadence %d - this is a bug\n", cadence);
    }
  }
}

static retval_t
copy_schedule (cron_entry* entry, const char* schedule_override) {
  size_t len = strlen(schedule_override);
  if (len > MAX_SCHEDULE_LENGTH) {
    log_warn("invalid schedule length for override (must be %d, was %s)\n", MAX_SCHEDULE_LENGTH, len);
    return ERR;
  }
  memcpy(entry->schedule, schedule_override, len);
  entry->schedule[len] = '\0';

  return OK;
}

static retval_t
copy_command (cron_entry* entry, const char* command) {
  size_t len = strlen(command);
  if (len > MAX_COMMAND_LENGTH) {
    log_warn("invalid command length for override (must be %d, was %s)\n", MAX_COMMAND_LENGTH, len);
    return ERR;
  }
  memcpy(entry->cmd, command, len);
  entry->cmd[len] = '\0';

  return OK;
}

cron_entry*
new_cron_entry (char* raw, time_t curr, crontab_t* ct, cadence_t cadence) {
  cron_entry* entry             = xmalloc(sizeof(cron_entry));
  char*       schedule_override = get_cadence(cadence);

  if (schedule_override) {
    copy_schedule(entry, schedule_override);
    if (copy_schedule(entry, schedule_override) != OK || copy_command(entry, raw) != OK || parse_expr(entry) != OK) {
      log_error("Failed to parse entry schedule %s / %s\n", raw, schedule_override);

      free(entry);
      return NULL;
    }
  } else if (parse_entry(entry, raw) != OK) {
    log_error("Failed to parse entry line %s\n", raw);
    free(entry);
    return NULL;
  }

  entry->parent = ct;
  entry->next   = cron_next(entry->expr, curr);
  entry->id     = ++id_counter;

  return entry;
}

void
renew_cron_entry (cron_entry* entry, time_t curr) {
  log_debug("Updating time for entry %d\n", entry->id);

  entry->next = cron_next(entry->expr, curr);
}

void
free_cron_entry (cron_entry* entry) {
  free(entry->expr);
  free(entry);
}
