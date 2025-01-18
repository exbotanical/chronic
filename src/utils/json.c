#include "utils/json.h"

#include <stdio.h>
#include <string.h>

#include "libutil/libutil.h"
#include "logger.h"
#include "utils/string.h"
#include "utils/xmalloc.h"

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
escape_json_string (const char* json) {
  // Allocate enough memory to store the escaped string
  // Worst case: every character becomes `\uXXXX` (6 characters)
  size_t json_len       = strlen(json);
  size_t max_output_len = json_len * 6 + 1;
  char*  output         = xmalloc(max_output_len);

  char* out_ptr         = output;
  for (const char* in_ptr = json; *in_ptr; ++in_ptr) {
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
