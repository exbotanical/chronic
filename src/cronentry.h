#ifndef CRONENTRY_H
#define CRONENTRY_H

#include <time.h>

#include "ccronexpr/ccronexpr.h"
#include "crontab.h"

typedef struct {
  cron_expr *expr;
  char *schedule;
  char *cmd;
  time_t next;
  unsigned int id;
  Crontab *parent;
} CronEntry;

CronEntry *new_cron_entry(char *raw, time_t curr, Crontab *ct);

void renew_cron_entry(CronEntry *entry, time_t curr);

#endif /* CRONENTRY_H */
