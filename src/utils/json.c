#include "utils/json.h"

#include <ctype.h>
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

  // Replace closing brace and skip the opening one
  *end = '\0';
  start++;

  while (*start) {
    // Skip whitespace
    while (*start && isspace((unsigned char)*start)) {
      start++;
    }

    // Parse key
    if (*start != '"') {
      log_warn("%s\n", "Expected key to start with a double quote");
      ret = ERR;
      goto done;
    }
    char* key_start = ++start;
    while (*start && (*start != '"' || *(start - 1) == '\\')) {
      start++;
    }

    if (*start != '"') {
      log_warn("%s\n", "Malformed JSON: Missing closing quote for key");
      ret = ERR;
      goto done;
    }
    *start    = '\0';  // Null-terminate the key
    char* key = key_start;

    // Skip to the colon
    start++;
    while (*start && isspace((unsigned char)*start)) {
      start++;
    }
    if (*start != ':') {
      log_warn("%s\n", "Expected colon after key");
      ret = ERR;
      goto done;
    }
    start++;

    // Parse value
    while (*start && isspace((unsigned char)*start)) {
      start++;
    }
    char* value_start;
    if (*start == '"') {
      value_start = ++start;
      while (*start && (*start != '"' || *(start - 1) == '\\')) {
        start++;
      }
      if (*start != '"') {
        log_warn("%s\n", "Malformed JSON: Missing closing quote for value");
        ret = ERR;
        goto done;
      }
    } else {
      value_start = start;
      while (*start && *start != ',' && !isspace((unsigned char)*start)) {
        start;
      }
    }
    char* value = value_start;
    if (*start) {
      *start = '\0';  // Null-terminate the value
      start++;
    }

    // Trim and store the key-value pair
    key   = trim_whitespace(key);
    value = trim_whitespace(value);
    ht_insert(pairs, s_copy(key), s_copy(value));

    // Skip trailing commas and whitespace
    while (*start && isspace((unsigned char)*start)) {
      start++;
    }
    if (*start == ',') {
      start++;
    }
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
