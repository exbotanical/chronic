#ifndef CRONENTRY_H
#define CRONENTRY_H

#include <time.h>

#include "ccronexpr/ccronexpr.h"
#include "crontab.h"

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
  char        *schedule;
  /**
   * The command specified by the entry.
   */
  char        *cmd;
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
  Crontab     *parent;
} CronEntry;

/**
 * Parses the raw line from the crontab file as an entry for the provided
 * crontab object.
 *
 * @param raw The raw line/entry.
 * @param curr The current crond iteration time.
 * @param ct The crontab object to which this entry belongs. We pass in the
 * crontab because we want the entry to have a pointer to its parent.
 */
CronEntry *new_cron_entry(char *raw, time_t curr, Crontab *ct);

/**
 * Renews the `next` field on the given cron entry based on the current crond
 * iteration time.
 *
 * @param entry The cron entry to renew.
 * @param curr The current crond iteration time.
 */
void renew_cron_entry(CronEntry *entry, time_t curr);

#endif /* CRONENTRY_H */
