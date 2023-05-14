#include "defs.h"

void* xmalloc(size_t sz) {
  void* ptr;
  if ((ptr = malloc(sz)) == NULL) {
    write_to_log("[xmalloc::%s] failed to allocate memory\n", __func__);
    exit(1);
  }

  return ptr;
}
