#ifndef CRON_ENTRY_H
#define CRON_ENTRY_H

#include <time.h>

#include "ccronexpr/ccronexpr.h"
#include "crontab.h"

#define HOURLY_EXPR         "0 * * * *"
#define DAILY_EXPR          "0 0 * * *"  // TODO: Run starting now
#define WEEKLY_EXPR         "0 0 * * 0"  // TODO: Run starting now
#define MONTHLY_EXPR        "0 0 1 * *"  // TODO: Run starting now

#define MAX_SCHEDULE_LENGTH 32
#define MAX_COMMAND_LENGTH  256

/**
 * Represents a single line (entry) in a crontab.
 */
typedef struct {
  /**
   * The pre-parsed cron expression.
   */
  cron_expr   *expr;
  /**
   * The original cron schedule specification in the entry.
   */
  char         schedule[MAX_SCHEDULE_LENGTH];
  /**
   * The command specified by the entry.
   */
  char         cmd[MAX_COMMAND_LENGTH];
  /**
   * The next execution time, relative to the current crond iteration.
   */
  time_t       next;
  /**
   * A unique identifier for the entry. Used for logging and non-critical
   * functions.
   */
  unsigned int id;
  /**
   * A pointer to the entry's parent crontab.
   */
  crontab_t   *parent;
} cron_entry;

/**
 * Parses the raw line from the crontab file as an entry for the provided
 * crontab object.
 *
 * @param raw The raw line/entry.
 * @param curr The current crond iteration time.
 * @param ct The crontab object to which this entry belongs. We pass in the
 *    crontab because we want the entry to have a pointer to its parent.
 * @param cadence Optional cadence override. Pass CADENCE_NA if none; otherwise,
 *    these are for expression parsing overrides such as hourly/daily/weekly.
 */
cron_entry *new_cron_entry(char *raw, time_t curr, crontab_t *ct, cadence_t cadence);

/**
 * Renews the `next` field on the given cron entry based on the current crond
 * iteration time.
 *
 * @param entry The cron entry to renew.
 * @param curr The current crond iteration time.
 */
void renew_cron_entry(cron_entry *entry, time_t curr);

/**
 * Deallocates a cron entry.
 */
void free_cron_entry(cron_entry *entry);

#endif /* CRON_ENTRY_H */
