#ifndef CONFIG_H
#define CONFIG_H

#ifndef SYS_CRONTABS_DIR
#  define SYS_CRONTABS_DIR "/etc/cron.d"
#endif

#ifndef CRONTABS_DIR
#  define CRONTABS_DIR "/var/spool/cron/crontabs"
#endif

#ifndef DEFAULT_PATH
#  define DEFAULT_PATH "/usr/bin:/bin:/usr/sbin:/sbin"
#endif

#ifndef MAILCMD_PATH
#  define MAILCMD_PATH "/usr/bin/mail"
#endif

// TODO: Allow override
#define TIMESTAMP_FMT "%Y-%m-%d %H:%M:%S"

#endif /* CONFIG_H */
