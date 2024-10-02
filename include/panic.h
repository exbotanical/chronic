#ifndef PANIC_H
#define PANIC_H

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdnoreturn.h>

static noreturn void
__panic (const char* fmt, ...) {
  va_list va;

  va_start(va, fmt);

  vfprintf(stderr, fmt, va);

  va_end(va);

  exit(EXIT_FAILURE);
}

#define panic(fmt, ...) \
  __panic("[%s@L%d] " fmt, __func__, __LINE__, __VA_ARGS__)

// See: https://gcc.gnu.org/onlinedocs/gcc/Statement-Exprs.html
#define array_init_or_panic()                                              \
  ({                                                                       \
    array_t* __name = array_init();                                        \
    if (!__name) {                                                         \
      panic("%s\n", "array_init returned a null pointer - this is a bug"); \
    }                                                                      \
    __name;                                                                \
  })

#define array_push_or_panic(arr, el)                                  \
  if (!array_push(arr, el)) {                                         \
    panic("%s\n", "array_push failed - this is a bug, probably OOM"); \
  }

#define array_get_or_panic(arr, idx)                                    \
  ({                                                                    \
    void* el = array_get(arr, idx);                                     \
    if (!el) {                                                          \
      panic("%s\n", "array_get returned null pointer - this is a bug"); \
    }                                                                   \
    el;                                                                 \
  })

#define ht_init_or_panic(base_capacity, free_value)                     \
  ({                                                                    \
    hash_table* ht = ht_init(base_capacity, free_value);                \
    if (!ht) {                                                          \
      panic("%s\n", "ht_init returned a null pointer - this is a bug"); \
    }                                                                   \
    ht;                                                                 \
  })

#define ht_get_or_panic(key, value)                                    \
  ({                                                                   \
    void* v = ht_get(key, value);                                      \
    if (!v) {                                                          \
      panic("%s\n", "ht_get returned a null pointer - this is a bug"); \
    }                                                                  \
    v;                                                                 \
  })

#define s_copy_or_panic(cp) \
  ({                        \
    char* __n = s_copy(cp); \
    if (!__n) {             \
      panic("%s\n", "n");   \
    }                       \
    __n;                    \
  })

#endif /* PANIC_H */
