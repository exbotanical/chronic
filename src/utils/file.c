#include "utils/file.h"

#include <dirent.h>
#include <errno.h>
#include <string.h>

#include "libutil/libutil.h"
#include "logger.h"
#include "utils/regex.h"

static inline bool
is_pointer_file (const char* fname) {
  return s_equals(fname, ".") || s_equals(fname, "..");
}

static inline bool
should_skip_file (const char* fname, const char* regex) {
  return regex && !match_string(fname, regex);
}

array_t*
get_filenames (char* dirpath, const char* regex) {
  DIR* dir;
  if ((dir = opendir(dirpath)) != NULL) {
    array_t* file_names = array_init_or_panic();
    log_debug("scanning dir %s\n", dirpath);

    struct dirent* den;
    while ((den = readdir(dir)) != NULL) {
      if (is_pointer_file(den->d_name)) {
        continue;
      }

      if (should_skip_file(den->d_name, regex)) {
        log_debug("skipping file %s (did not match regex %s)\n", den->d_name, regex);
        continue;
      }

      log_debug("found file %s/%s\n", dirpath, den->d_name);

      array_push_or_panic(file_names, s_copy_or_panic(den->d_name));
    }

    closedir(dir);
    return file_names;
  } else {
    log_warn("unable to scan dir %s (reason: %s)\n", dirpath, strerror(errno));
  }

  return NULL;
}

bool
file_exists (const char* path) {
  struct stat buf;
  return stat(path, &buf) == 0;
}
