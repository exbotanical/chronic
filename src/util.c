#include <stdlib.h>
#include <uuid/uuid.h>

#include "libutil/libutil.h"
#include "log.h"
#include "opt-constants.h"

void*
xmalloc (size_t sz)
{
  void* ptr;
  if ((ptr = malloc(sz)) == NULL) {
    printlogf("[xmalloc::%s] failed to allocate memory\n", __func__);
    exit(1);
  }

  return ptr;
}

char*
to_time_str (time_t t)
{
  char       msg[512];
  struct tm* time_info = localtime(&t);
  strftime(msg, sizeof(msg), TIMESTAMP_FMT, time_info);

  return fmt_str("%s", msg);
}

char*
create_uuid (void)
{
  char uuid[UUID_STR_LEN];

  uuid_t bin_uuid;
  uuid_generate_random(bin_uuid);
  uuid_unparse(bin_uuid, uuid);

  return s_copy(uuid);
}
