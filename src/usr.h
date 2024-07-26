#ifndef USR_H
#define USR_H

typedef struct user {
  unsigned int uid;
  char*        uname;
  bool         root;
} user_t;

#endif /* USR_H */
