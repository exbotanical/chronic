#include <uuid/uuid.h>

#include "defs.h"

// need to free
char *create_uuid(void) {
  char uuid[UUID_STR_LEN];

  uuid_t bin_uuid;
  uuid_generate_random(bin_uuid);
  uuid_unparse(bin_uuid, uuid);

  return s_copy(uuid);
}
