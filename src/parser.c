#include "parser.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "libhash/libhash.h"
#include "libutil/libutil.h"
#include "logger.h"
#include "utils/regex.h"
#include "utils/string.h"
#include "utils/xmalloc.h"

// Basically whether we support seconds
#define SUPPORTED_CRONEXPR_COLS 5

bool
is_comment_line (const char* str) {
  while (isspace((unsigned char)*str)) {
    str++;
  }

  return *str == '#';
}

bool
should_parse_line (const char* line) {
  // 3 is the absolute minimum for a crontab line e.g.
  // K=V
  if ((*line == 0) || strlen(line) < 3 || is_comment_line(line)) {
    return false;
  }

  while (*line == ' ' || *line == '\t') {
    line++;
  }

  return (*line != 0);
}

void
strip_comment (char* str) {
  char* comment_start = strchr(str, '#');

  if (comment_start) {
    *comment_start = '\0';
  }
}

static int
get_end_of_schedule (const char* s) {
  bool         on    = false;
  unsigned int idx   = 0;
  unsigned int count = 0;

  unsigned int len   = strlen(s);

  while (len--) {
    if (count == SUPPORTED_CRONEXPR_COLS) {
      break;
    }

    if (s[idx] != ' ' && s[idx] != '\t') {
      on = true;
    } else if (on) {
      on = false;
      count++;
    }

    idx++;
  }

  return idx;
}

retval_t
parse_schedule (const char* s, char* dest) {
  if (!s) {
    return ERR;
  }

  unsigned int idx = get_end_of_schedule(s);
  if (idx == 0 || idx == strlen(s) || idx > MAX_SCHEDULE_LENGTH) {
    return ERR;
  }

  memcpy(dest, s, idx);
  dest[idx - 1] = '\0';

  return OK;
}

retval_t
parse_cmd (char* s, char* dest) {
  if (!s) {
    return ERR;
  }

  unsigned int len = strlen(s);

  unsigned int idx = get_end_of_schedule(s);
  if (idx == 0 || idx == len || idx > MAX_SCHEDULE_LENGTH) {
    return ERR;
  }

  s   += idx;
  len  = strlen(s);

  if (len > MAX_COMMAND_LENGTH) {
    return ERR;
  }

  memcpy(dest, s, len);
  dest[len] = '\0';

  return OK;
}

retval_t
parse_expr (cron_entry* entry) {
  cron_expr*  expr = xmalloc(sizeof(cron_expr));
  const char* err  = NULL;

  replace_tabs_with_spaces(entry->schedule);
  cron_parse_expr(entry->schedule, expr, &err);

  if (err) {
    log_warn("error parsing cron expression: %s\n\n", err);
    free(expr);
    return ERR;
  }

  entry->expr = expr;
  return OK;
}

retval_t
parse_entry (cron_entry* entry, char* line) {
  if (!line) {
    return ERR;
  }

  strip_comment(line);
  char* line_cp = s_trim(line);
  if (parse_schedule(line_cp, entry->schedule) != OK || parse_cmd(line_cp, entry->cmd) != OK) {
    free(line_cp);
    return ERR;
  }

  free(line_cp);

  return parse_expr(entry);
}

parse_ln_result
parse_line (char* ptr, int max_entries, hash_table* vars) {
  if (max_entries == 1) {
    return DONE;
  }

  unsigned int len;

  // Skip whitespace and newlines
  while (*ptr == ' ' || *ptr == '\t' || *ptr == '\n') {
    ++ptr;
  }

  // Remove trailing newline
  len = strlen(ptr);
  if (len && ptr[len - 1] == '\n') {
    ptr[--len] = 0;
  }

  // If that's it or the entire line is a comment, skip
  if (!should_parse_line(ptr)) {
    return SKIP_LINE;
  }

  if (match_variable(ptr, vars)) {
    return ENV_VAR_ADDED;
  }

  return ENTRY;
}
