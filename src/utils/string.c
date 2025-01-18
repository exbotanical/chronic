#include "utils/string.h"

#include <ctype.h>
#include <string.h>
#include <uuid/uuid.h>

#include "utils/xpanic.h"

#ifndef UUID_STR_LEN
#  define UUID_STR_LEN 37
#endif

char*
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

char*
create_uuid (void) {
  char uuid[UUID_STR_LEN];

  uuid_t bin_uuid;
  uuid_generate_random(bin_uuid);
  uuid_unparse(bin_uuid, uuid);

  return s_copy_or_panic(uuid);
}
