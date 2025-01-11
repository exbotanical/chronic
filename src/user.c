#include "user.h"

#include <pwd.h>
#include <unistd.h>

#include "globals.h"

void
user_init (void) {
  usr.uid   = getuid();
  usr.root  = usr.uid == 0;
  usr.uname = getpwuid(usr.uid)->pw_name;
}
