#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <fcntl.h>

#define ROOT_UNAME           "root"
#define ROOT_UID             0

#define ALL_PERMS            07777
#define OWNER_RW_PERMS       0600

#define HOMEDIR_ENVVAR       "HOME"
#define SHELL_ENVVAR         "SHELL"
#define PATH_ENVVAR          "PATH"
#define UNAME_ENVVAR         "USER"
#define MAILTO_ENVVAR        "MAILTO"

#define TINY_BUFFER          32
#define SMALL_BUFFER         TINY_BUFFER * 8
#define MED_BUFFER           SMALL_BUFFER * 4
#define LARGE_BUFFER         MED_BUFFER * 2

/* Mail command path | sender | hostname | subject |  recipient */
#define MAILCMD_FMT          "%s -r%s@%s -s '%s' %s"
#define MAIL_SUBJECT         "cron job completed"

#define CHRONIC_VERSION      "0.0.1"
#define DAEMON_IDENT         "crond"

#define DEV_NULL             "/dev/null"
#define DEV_TTY              "/dev/tty"

/* Generic timestamp format */
#define TIMESTAMP_FMT        "%Y-%m-%d %H:%M:%S"

#define DEFAULT_LOGFILE_PATH "/var/log/" DAEMON_IDENT ".log"
#endif /* CONSTANTS_H */
