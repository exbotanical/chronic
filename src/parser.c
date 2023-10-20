#include <ctype.h>

#include "defs.h"

// TODO: static fns
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

RETVAL parse_cmd(char* line, Job* job, int count) {
  char* line_cp = s_copy(line);

  job->cmd = until_nth_of_char(line_cp, ' ', line_cp[0] == '@' ? 1 : count);
  if (s_nullish(job->cmd)) {
    return ERR;
  }

  // TODO: free
  job->schedule = line_cp;

  // splits the string in two; note: shared memory block so only one `free` may
  // be called
  *job->cmd = '\0';
  ++job->cmd;
  // TODO: free
  job->cmd = s_trim(job->cmd);

  return OK;
}

// TODO: what if the line itself is invalid? wbt job?
RETVAL parse(Job* job, char* line) {
  char* line_cp = s_trim(line);
  strip_comment(line_cp);

  if (parse_cmd(line_cp, job, SPACES_BEFORE_CMD) != OK) {
    free(line_cp);
    return ERR;
  }

  free(line_cp);

  cron_expr* expr = malloc(sizeof(cron_expr));
  const char* err = NULL;
  cron_parse_expr(job->schedule, expr, &err);

  if (err) {
    write_to_log("error parsing cron expression: %s\n\n", err);
    return ERR;
  }

  job->expr = expr;
  return OK;
}

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
