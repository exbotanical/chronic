#ifndef XMALLOC_H
#define XMALLOC_H

#include <stddef.h>
#include <stdlib.h>

#include "utils/xpanic.h"

/**
 * xmalloc is a malloc wrapper that exits the program if out of memory
 */
static inline void*
xmalloc (size_t sz) {
  void* ptr;
  if ((ptr = malloc(sz)) == NULL) {
    xpanic("xmalloc failed to allocate memory");
  }

  return ptr;
}

#endif /* XMALLOC_H */
