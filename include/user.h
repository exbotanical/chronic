#ifndef USER_H
#define USER_H

#include <stdbool.h>

/**
 * Stores information about the user that executed this program.
 */
typedef struct user {
  unsigned int uid;
  char*        uname;
  bool         root;
} user_t;

void user_init(void);

#endif /* USER_H */
