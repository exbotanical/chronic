#ifndef CRONENTRY_H
#define CRONENTRY_H

#include <time.h>

#include "ccronexpr/ccronexpr.h"

typedef struct {
  cron_expr *expr;
  char *schedule;
  char *cmd;
  char *owner_uname;
  time_t next;
  unsigned int id;
} CronEntry;

CronEntry *new_cron_entry(char *raw, time_t curr, char *uname);

void renew_cron_entry(CronEntry *entry, time_t curr);

#endif /* CRONENTRY_H */
