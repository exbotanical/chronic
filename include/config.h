#ifndef CONFIG_H
#define CONFIG_H

/* Crontab directory path for system daemons and applications */
#ifndef SYS_APP_CRONTABS_DIR
#  define SYS_APP_CRONTABS_DIR "/etc/crontab"
#endif

/* Crontab directory path for general system-wide jobs */
#ifndef SYS_CRONTABS_DIR
#  define SYS_CRONTABS_DIR "/etc/cron.d"
#endif

/* Crontab directory path for general hourly system-wide jobs */
#ifndef SYS_HOURLY_CRONTABS_DIR
#  define SYS_HOURLY_CRONTABS_DIR "/etc/cron.hourly"
#endif

/* Crontab directory path for general daily system-wide jobs */
#ifndef SYS_DAILY_CRONTABS_DIR
#  define SYS_DAILY_CRONTABS_DIR "/etc/cron.daily"
#endif

/* Crontab directory path for general weekly system-wide jobs */
#ifndef SYS_WEEKLY_CRONTABS_DIR
#  define SYS_WEEKLY_CRONTABS_DIR "/etc/cron.weekly"
#endif

/* Crontab directory path for general monthly system-wide jobs */
#ifndef SYS_MONTHLY_CRONTABS_DIR
#  define SYS_MONTHLY_CRONTABS_DIR "/etc/cron.monthly"
#endif

/* Crontab directory path for per-user jobs */
#ifndef CRONTABS_DIR
#  define CRONTABS_DIR "/var/spool/cron/crontabs"
#endif

/* Default PATH used in job execution context */
#ifndef DEFAULT_PATH
#  define DEFAULT_PATH "/usr/bin:/bin:/usr/sbin:/sbin"
#endif

/* Default mail program executable path */
#ifndef MAILCMD_PATH
#  define MAILCMD_PATH "/usr/bin/mail"
#endif

#endif /* CONFIG_H */
