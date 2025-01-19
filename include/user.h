#ifndef USER_H
#define USER_H

#include <pwd.h>
#include <stdbool.h>

#include "constants.h"
#include "libutil/libutil.h"

/**
 * Stores information about the user that executed this program.
 */
typedef struct user {
  /* User id */
  unsigned int uid;
  /* Username or root if root (shocker) */
  char*        uname;
  /* Is this root? */
  bool         root;
} user_t;

static bool
is_valid_user (struct passwd* pw, const char* uname) {
  return pw != NULL || s_equals(uname, ROOT_UNAME);
}

void user_init(void);

#endif /* USER_H */
