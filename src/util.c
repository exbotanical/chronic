#include <stdlib.h>

#include "libutil/libutil.h"
#include "log.h"

void* xmalloc(size_t sz) {
  void* ptr;
  if ((ptr = malloc(sz)) == NULL) {
    printlogf("[xmalloc::%s] failed to allocate memory\n", __func__);
    exit(1);
  }

  return ptr;
}
