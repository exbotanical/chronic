#include <stdlib.h>
// #include <uuid/uuid.h>

#include "libutil/libutil.h"
#include "log.h"

void* xmalloc(size_t sz) {
  void* ptr;
  if ((ptr = malloc(sz)) == NULL) {
    write_to_log("[xmalloc::%s] failed to allocate memory\n", __func__);
    exit(1);
  }

  return ptr;
}

// need to free
// char* create_uuid(void) {
//   char uuid[UUID_STR_LEN];

//   uuid_t bin_uuid;
//   uuid_generate_random(bin_uuid);
//   uuid_unparse(bin_uuid, uuid);

//   return s_copy(uuid);
// }
