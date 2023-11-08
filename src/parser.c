#include "parser.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "libhash/libhash.h"
#include "libutil/libutil.h"
#include "log.h"
#include "regexpr.h"
#include "strdup/strdup.h"

// Basically whether we support seconds (7)
#define SPACES_BEFORE_CMD 5

// TODO: static fns
bool is_comment_line(const char* str) {
  while (isspace((unsigned char)*str)) {
    str++;
  }

  return *str == '#';
}

bool should_parse_line(const char* line) {
  if (strlen(line) < (SPACES_BEFORE_CMD * 2) + 1 || (*line == 0) ||
      is_comment_line(line)) {
    return false;
  }

  return true;
}

char* until_nth_of_char(const char* str, char c, int n) {
  int count = 0;
  while (*str) {
    if (*str == c && ++count == n) {
      // If it's just an empty space...
      if (*(str + 1) == '\0') return NULL;
      return (char*)str;
    }
    str++;
  }

  return NULL;
}

void strip_comment(char* str) {
  char* comment_start = strchr(str, '#');

  if (comment_start) {
    *comment_start = '\0';
  }
}

Retval parse_cmd(char* line, CronEntry* entry, int count) {
  char* line_cp = s_copy(line);

  entry->cmd = until_nth_of_char(line_cp, ' ', line_cp[0] == '@' ? 1 : count);
  if (s_nullish(entry->cmd)) {
    return ERR;
  }

  // TODO: free
  entry->schedule = line_cp;

  // splits the string in two; note: shared memory block so only one `free` may
  // be called
  *entry->cmd = '\0';
  ++entry->cmd;
  // TODO: free
  entry->cmd = s_trim(entry->cmd);

  return OK;
}

Retval parse_entry(CronEntry* entry, char* line) {
  char* line_cp = s_trim(line);
  strip_comment(line_cp);

  if (parse_cmd(line_cp, entry, SPACES_BEFORE_CMD) != OK) {
    free(line_cp);
    return ERR;
  }

  free(line_cp);

  cron_expr* expr = malloc(sizeof(cron_expr));
  const char* err = NULL;
  cron_parse_expr(entry->schedule, expr, &err);

  if (err) {
    printlogf("error parsing cron expression: %s\n\n", err);
    return ERR;
  }

  entry->expr = expr;
  return OK;
}

ParseLineResult parse_line(char* ptr, int max_entries) {
  // TODO: move?
  if (max_entries == 1) return DONE;

  unsigned int len;

  // Skip whitespace and newlines
  while (*ptr == ' ' || *ptr == '\t' || *ptr == '\n') ++ptr;

  // Remove trailing newline
  len = strlen(ptr);
  if (len && ptr[len - 1] == '\n') ptr[--len] = 0;

  // If that's it or the entire line is a comment, skip
  if (!should_parse_line(ptr)) return SKIP_LINE;

  return ENTRY;
}
