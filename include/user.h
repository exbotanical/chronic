#ifndef USER_H
#define USER_H

#include <stdbool.h>

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

void user_init(void);

#endif /* USER_H */
