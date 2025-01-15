#include "cronentry.h"

#include <stdlib.h>

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
      panic("unhandled cadence %d - this is a bug\n", cadence);
    }
  }
}

cron_entry*
new_cron_entry (char* raw, time_t curr, crontab_t* ct, cadence_t cadence) {
  cron_entry* entry         = xmalloc(sizeof(cron_entry));
  char*       expr_override = get_cadence(cadence);

  if (expr_override) {
    entry->schedule = s_copy(expr_override);
    if (parse_schedule(entry) != OK) {
      log_error("Failed to parse entry schedule %s / %s\n", raw, expr_override);

      free(expr_override);
      free(entry);

      return NULL;
    }
    entry->cmd = s_copy(raw);
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
  free(entry->schedule);
  free(entry->cmd);
  free(entry);
}
