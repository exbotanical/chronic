#include <fcntl.h>
#include <stdlib.h>
#include <string.h>

#include "libutil/libutil.h"
#include "tap.c/tap.h"
#include "tests.h"

char*
setup_test_directory () {
  char template[] = "/tmp/tap_test_dir.XXXXXX";
  // Need to copy because mkdtemp overwrites ^ which is locally scoped
  char* dirname   = s_copy(mkdtemp(template));

  if (dirname == NULL) {
    perror("mkdtemp failed");
    fail();
  }

  return dirname;
}

void
setup_test_file (const char* dirname, const char* filename, const char* content) {
  char path[256];
  snprintf(path, sizeof(path), "%s/%s", dirname, filename);

  int fd = open(path, O_CREAT | O_WRONLY, 0644);
  if (fd == -1) {
    perror("Failed to create test crontab file");
    exit(EXIT_FAILURE);
  }

  if (content) {
    write(fd, content, strlen(content));
  }

  close(fd);
}

void
modify_test_file (const char* dirname, const char* filename, const char* content) {
  char path[256];
  snprintf(path, sizeof(path), "%s/%s", dirname, filename);

  int fd = open(path, O_APPEND | O_WRONLY);
  if (fd == -1) {
    perror("Failed to open the test crontab file for modification");
    exit(EXIT_FAILURE);
  }

  write(fd, content, strlen(content));

  close(fd);
}

void
cleanup_test_directory (char* dirname) {
  rmdir(dirname);
}

void
cleanup_test_file (char* dirname, char* fname) {
  char path[256];
  snprintf(path, sizeof(path), "%s/%s", dirname, fname);
  unlink(path);
}

int
get_fd (char* fpath) {
  return open(fpath, O_RDONLY | O_NONBLOCK, 0);
}

bool
has_filename_comparator (void* el, void* compare_to) {
  return s_equals((char*)el, compare_to);
}
