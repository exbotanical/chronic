#include "defs.h"

char *str_join(array_t *sarr, const char *delim) {
  buffer_t *buf = buffer_init(NULL);

  unsigned int sz = array_size(sarr);

  for (unsigned int i = 0; i < sz; i++) {
    const char *s = array_get(sarr, i);
    buffer_append(buf, s);

    if (i != sz - 1) {
      buffer_append(buf, delim);
    }
  }

  return buffer_state(buf);
}
