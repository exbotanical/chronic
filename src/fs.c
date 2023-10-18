
#include "defs.h"

static array_t* process_file(char* path, char* uname) {
  unsigned int max_entries;
  unsigned int max_lines;

  if (strcmp(uname, ROOT_USER) == 0) {
    max_entries = 65535;
  } else {
    max_entries = MAXENTRIES;
  }

  max_lines = max_entries * 10;

  FILE* fd;
  char buf[RW_BUFFER];

  array_t* lines = NULL;

  if ((fd = fopen(path, "r")) != NULL) {
    lines = array_init();

    while (fgets(buf, sizeof(buf), fd) != NULL && --max_lines) {
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

  fclose(fd);

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

        return NULL;
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
