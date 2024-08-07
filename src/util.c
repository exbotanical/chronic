#include "util.h"

#include <dirent.h>
#include <stdlib.h>
#include <uuid/uuid.h>

#include "log.h"
#include "opt-constants.h"

void*
xmalloc (size_t sz) {
  void* ptr;
  if ((ptr = malloc(sz)) == NULL) {
    printlogf("[xmalloc::%s] failed to allocate memory\n", __func__);
    exit(1);
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

  return s_copy(uuid);
}

array_t*
get_filenames (char* dirpath) {
  DIR* dir;

  if ((dir = opendir(dirpath)) != NULL) {
    printlogf("scanning dir %s\n", dirpath);
    struct dirent* den;

    array_t* file_names = array_init();

    while ((den = readdir(dir)) != NULL) {
      // Skip pointer files
      if (s_equals(den->d_name, ".") || s_equals(den->d_name, "..")) {
        continue;
      }

      printlogf("found file %s/%s\n", dirpath, den->d_name);
      array_push(file_names, s_copy(den->d_name));
    }

    closedir(dir);
    return file_names;
  } else {
    perror("opendir");
    printlogf("unable to scan dir %s\n", dirpath);
  }

  return NULL;
}
