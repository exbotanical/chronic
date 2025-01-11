#include "cronentry.h"

#include <stdlib.h>

#include "log.h"
#include "parser.h"
#include "utils.h"

unsigned int id_counter = 0;

cron_entry*
new_cron_entry (char* raw, time_t curr, crontab_t* ct) {
  cron_entry* entry = xmalloc(sizeof(cron_entry));

  if (parse_entry(entry, raw) != OK) {
    printlogf("[ERROR] Failed to parse entry line %s\n", raw);
    return NULL;
  }

  entry->parent = ct;
  entry->next   = cron_next(entry->expr, curr);
  entry->id     = ++id_counter;

  return entry;
}

void
renew_cron_entry (cron_entry* entry, time_t curr) {
  printlogf("Updating time for entry %d\n", entry->id);

  entry->next = cron_next(entry->expr, curr);
}

void
free_cron_entry (cron_entry* entry) {
  free(entry->expr);
  free(entry->schedule);
  free(entry->cmd);
  free(entry);
}
