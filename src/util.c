#include "util.h"

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <uuid/uuid.h>

#include "constants.h"
#include "libhash/libhash.h"
#include "logger.h"
#include "panic.h"
#include "regexpr.h"

#ifdef __MACH__
#  include <mach/clock.h>
#  include <mach/mach.h>
#endif

#ifndef UUID_STR_LEN
#  define UUID_STR_LEN 37
#endif

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

void
replace_tabs_with_spaces (char* str) {
  for (size_t i = 0; str[i] != '\0'; i++) {
    if (str[i] == '\t') {
      str[i] = ' ';
    }
  }
}

void*
xmalloc (size_t sz) {
  void* ptr;
  if ((ptr = malloc(sz)) == NULL) {
    xpanic("xmalloc failed to allocate memory");
  }

  return ptr;
}

void
get_time (struct timespec* ts) {
#ifdef __MACH__
  clock_serv_t    cclock;
  mach_timespec_t mts;
  host_get_clock_service(mach_host_self(), REALTIME_CLOCK, &cclock);
  clock_get_time(cclock, &mts);
  mach_port_deallocate(mach_task_self(), cclock);
  ts->tv_sec  = mts.tv_sec;
  ts->tv_nsec = mts.tv_nsec;
#else
  clock_gettime(CLOCK_REALTIME, ts);
#endif
}

char*
to_time_str_millis (struct timespec* ts) {
  struct tm* utc_time = gmtime(&ts->tv_sec);
  if (!utc_time) {
    perror("gmtime");
    return NULL;
  }

  char buffer[32];
  strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%S", utc_time);

  char result[64];
  snprintf(result, sizeof(result), "%s.%03ldZ", buffer, ts->tv_nsec / 1000000);

  return s_copy(result);
}

char*
to_time_str_secs (time_t ts) {
  struct tm* utc_time = gmtime(&ts);
  if (!utc_time) {
    perror("gmtime");
    return NULL;
  }

  char buffer[30];
  strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%S", utc_time);

  char result[40];
  snprintf(result, sizeof(result), "%s.%03ldZ", buffer, 0L);

  return s_copy(result);
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

// No arrays
retval_t
parse_json (const char* json, hash_table* pairs) {
  retval_t ret       = OK;

  char* mutable_json = s_copy_or_panic(json);

  char* start        = strchr(mutable_json, '{');
  char* end          = strrchr(mutable_json, '}');
  if (!start || !end || start >= end) {
    log_warn("%s\n", "Invalid JSON format");
    ret = ERR;
    goto done;
  }

  // Replace closing brace and skip next opening one
  *end = '\0';
  start++;

  char* pair_str = strtok(start, ",");
  while (pair_str) {
    char* colon = strchr(pair_str, ':');
    if (!colon) {
      log_warn("Invalid key-value pair: %s\n", pair_str);
      ret = ERR;
      goto done;
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

    pair_str = strtok(NULL, ",");
  }

done:
  free(mutable_json);

  return ret;
}

// Function to escape control characters in a JSON string
char*
escape_json_string (const char* input) {
  // Allocate enough memory to store the escaped string
  // Worst case: every character becomes `\uXXXX` (6 characters)
  size_t input_len      = strlen(input);
  size_t max_output_len = input_len * 6 + 1;
  char*  output         = xmalloc(max_output_len);

  char* out_ptr         = output;
  for (const char* in_ptr = input; *in_ptr; ++in_ptr) {
    unsigned char c = *in_ptr;
    if (c <= 0x1F) {  // Control characters
      out_ptr += sprintf(out_ptr, "\\u%04x", c);
    } else {
      *out_ptr++ = c;
    }
  }
  *out_ptr = '\0';

  return output;
}
