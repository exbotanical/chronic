#include "cronentry.h"

#include "log.h"
#include "parser.h"
#include "util.h"

unsigned int id_counter = 0;

CronEntry* new_cron_entry(char* raw, time_t curr, char* uname) {
  CronEntry* entry = xmalloc(sizeof(CronEntry));

  // TODO: Lift out
  if (parse_entry(entry, raw) != OK) {
    printlogf("Failed to parse entry line %s\n", raw);
    return NULL;
  }

  entry->owner_uname = uname;
  entry->next = cron_next(entry->expr, curr);
  entry->id = ++id_counter;

  return entry;
}

void renew_cron_entry(CronEntry* entry, time_t curr) {
  printlogf("Updating time for entry %d\n", entry->id);

  entry->next = cron_next(entry->expr, curr);
}
