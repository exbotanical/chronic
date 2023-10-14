#include "defs.h"

static char* get_after_nth_c(const char* str, char c, int n) {
  int count = 0;
  while (*str) {
    if (*str == c && ++count == n) return (char*)str;
    str++;
  }

  return NULL;
}

static RETVAL parse_line(char* line, Job* job, int count) {
  job->schedule = line;
  job->cmd = get_after_nth_c(line, ' ', line[0] == '@' ? 1 : count);

  if (!job->cmd) {
    return ERR;
  }

  *job->cmd = '\0';
  ++job->cmd;
  job->cmd = s_trim(job->cmd);

  return OK;
}

RETVAL parse(Job* job, char* line) {
  char* line_cp = s_trim(line);  // TODO: free

  char* comment_start = strchr(line_cp, '#');

  if (comment_start) {
    *comment_start = '\0';
  }

  if (parse_line(line_cp, job, SPACES_BEFORE_CMD) != OK) {
    return ERR;
  }

  cron_expr* expr = malloc(sizeof(cron_expr));
  const char* err = NULL;
  cron_parse_expr(job->schedule, expr, &err);

  if (err) {
    printf("error parsing cron expression: %s\n\n", err);
    return ERR;
  }

  job->expr = expr;
  return OK;
}
