#ifndef USR_H
#define USR_H

/**
 * Stores information about the user that executed this program.
 */
typedef struct user {
  unsigned int uid;
  char*        uname;
  bool         root;
} user_t;

#endif /* USR_H */
