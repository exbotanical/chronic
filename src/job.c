#include "defs.h"

#define BUFFER_SIZE 255

array_t *scan_jobs(char *fpath) {
  char line[BUFFER_SIZE];
  FILE *file = fopen(fpath, "r");
  if (file == NULL) {
    perror("fopen");
    exit(1);
  }

  if (fgetc(file) == EOF) {
    return NULL;
  }

  fseek(file, 0, SEEK_SET);

  array_t *jobs = array_init();
  while (fgets(line, sizeof(line), file)) {
    Job *j = malloc(sizeof(Job));

    j->code = s_trim(line);
    j->pid = -1;
    j->ret = -1;
    j->status = PENDING;

    array_push(jobs, j);
  }

  fclose(file);
  return jobs;
}
