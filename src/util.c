#include "util.h"

#include <ctype.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <uuid/uuid.h>

#include "config.h"
#include "log.h"
#include "panic.h"
#include "util.h"

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
pretty_print_seconds (double seconds) {
  int          ns           = (int)seconds;
  unsigned int days         = ns / 86400;
  ns                       %= 86400;
  unsigned int hours        = ns / 3600;
  ns                       %= 3600;
  unsigned int minutes      = ns / 60;
  ns                       %= 60;
  unsigned int rem_seconds  = ns;

  char* s = s_fmt("%d days, %d hours, %d minutes, %d seconds", days, hours, minutes, rem_seconds);
  return s;
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

static char*
trim_whitespace (char* str) {
  while (isspace((unsigned char)*str)) {
    str++;
  }
  if (*str == '\0') {
    return str;
  }

  char* end = str + strlen(str) - 1;
  while (end > str && isspace((unsigned char)*end)) {
    end--;
  }

  *(end + 1) = '\0';
  return str;
}

// No arrays
int
parse_json (const char* json, hash_table* pairs) {
  char* mutable_json = s_copy_or_panic(json);

  char* start        = strchr(mutable_json, '{');
  char* end          = strrchr(mutable_json, '}');
  if (!start || !end || start >= end) {
    printlogf("Invalid JSON format\n");
    free(mutable_json);
    return -1;
  }

  // Replace closing brace and skip next opening one
  *end = '\0';
  start++;

  int   count    = 0;
  char* pair_str = strtok(start, ",");
  while (pair_str) {
    char* colon = strchr(pair_str, ':');
    if (!colon) {
      printlogf("Invalid key-value pair: %s\n", pair_str);
      free(mutable_json);
      return -1;
    }

    *colon      = '\0';
    char* key   = trim_whitespace(pair_str);
    char* value = trim_whitespace(colon + 1);

    if (*key == '"') {
      key++;
    }
    if (*value == '"') {
      value++;
    }
    if (key[strlen(key) - 1] == '"') {
      key[strlen(key) - 1] = '\0';
    }
    if (value[strlen(value) - 1] == '"') {
      value[strlen(value) - 1] = '\0';
    }

    ht_insert(pairs, s_copy(key), s_copy(value));

    count++;
    pair_str = strtok(NULL, ",");
  }

  free(mutable_json);
  return count;
}
