#include "util.h"

#include <dirent.h>
#include <stdlib.h>
#include <uuid/uuid.h>

#include "log.h"
#include "opt-constants.h"
#include "panic.h"

void*
xmalloc (size_t sz) {
  void* ptr;
  if ((ptr = malloc(sz)) == NULL) {
    panic("xmalloc failed to allocate memory");
  }

  return ptr;
}

char*
to_time_str (time_t t) {
  char       msg[512];
  struct tm* time_info = localtime(&t);
  strftime(msg, sizeof(msg), TIMESTAMP_FMT, time_info);

  return s_fmt("%s", msg);
}

char*
create_uuid (void) {
  char uuid[UUID_STR_LEN];

  uuid_t bin_uuid;
  uuid_generate_random(bin_uuid);
  uuid_unparse(bin_uuid, uuid);

  return s_copy_or_panic(uuid);
}

array_t*
get_filenames (char* dirpath) {
  DIR* dir;
  // TODO: Only if modified
  if ((dir = opendir(dirpath)) != NULL) {
    printlogf("scanning dir %s\n", dirpath);
    struct dirent* den;

    array_t* file_names = array_init_or_panic();

    while ((den = readdir(dir)) != NULL) {
      // Skip pointer files
      if (s_equals(den->d_name, ".") || s_equals(den->d_name, "..")) {
        continue;
      }

      printlogf("found file %s/%s\n", dirpath, den->d_name);

      array_push_or_panic(file_names, s_copy_or_panic(den->d_name));
    }

    closedir(dir);
    return file_names;
  } else {
    perror("opendir");
    printlogf("unable to scan dir %s\n", dirpath);
  }

  return NULL;
}
