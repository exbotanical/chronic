#include "parser.h"

#include <dirent.h>
#include <pwd.h>

#include "ccronexpr/ccronexpr.h"

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

  return OK;
}

time_t parse(time_t curr, Job* job, char* line) {
  char* line_cp = s_trim(line);  // TODO: free

  char* comment_start = strchr(line_cp, '#');

  if (comment_start) {
    *comment_start = '\0';
  }

  if (parse_line(line_cp, job, SPACES_BEFORE_CMD) != OK) {
    return ERR;
  }

  cron_expr expr;
  const char* err = NULL;
  cron_parse_expr(job->schedule, &expr, &err);

  if (err) {
    printf("error parsing cron expression: %s\n", err);
    return ERR;
  }

  return cron_next(&expr, curr);
}

array_t* process_file(char* path, char* uname) {
  unsigned int max_entries;
  unsigned int max_lines;

  if (strcmp(uname, "root") == 0) {
    max_entries = 65535;
  } else {
    max_entries = MAXENTRIES;
  }

  max_lines = max_entries * 10;

  FILE* fp;
  char buf[RW_BUFFER];

  array_t* lines = NULL;

  if ((fp = fopen(path, "r")) != NULL) {
    lines = array_init();

    while (fgets(buf, sizeof(buf), fp) != NULL && --max_lines) {
      char* ptr = buf;
      int len;

      // Skip whitespace and newlines
      while (*ptr == ' ' || *ptr == '\t' || *ptr == '\n') ++ptr;

      // Remove trailing newline
      len = strlen(ptr);
      if (len && ptr[len - 1] == '\n') ptr[--len] = 0;

      // If that's it or the entire line is a comment, skip
      if (*ptr == 0 || *ptr == '#') continue;

      if (--max_entries == 0) break;

      array_push(lines, s_copy(ptr));
    }
  }

  fclose(fp);

  return lines;
}

array_t* process_dir(char* dpath) {
  DIR* dir;

  if ((dir = opendir(dpath)) != NULL) {
    struct dirent* den;
    char* path;

    array_t* all_lines = array_init();

    while ((den = readdir(dir)) != NULL) {
      // Skip pointer files
      if (s_equals(den->d_name, ".") || s_equals(den->d_name, "..")) {
        continue;
      }

      char* path;
      if (!(path = fmt_str("%s/%s", dpath, den->d_name))) {
        printf("UH OH\n");

        return;
      }

      // TODO: perms check
      // TODO: root vs user
      array_t* lines = process_file(path, den->d_name);
      if (!lines) {
        printf("failed to process file");
        continue;
      }

      array_t* tmp = array_concat(all_lines, lines);
      array_free(lines);
      array_free(all_lines);

      all_lines = tmp;
    }
    closedir(dir);
    return all_lines;
  } else {
    printf("unable to scan directory %s\n", dpath);
  }

  return NULL;
}
