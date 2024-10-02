#define _XOPEN_SOURCE   1  // For sigaction
#define _DEFAULT_SOURCE 1  // For SA_RESTART
#include "daemon.h"

#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>  // For flock
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <unistd.h>

#include "cli.h"
#include "constants.h"
#include "job.h"
#include "libutil/libutil.h"
#include "log.h"
#include "panic.h"

#define LOCKFILE_BUFFER_SZ    512
#define SYS_LOCKFILE_PATH     "/var/run/crond.pid"
#define USR_LOCKFILE_PATH_FMT "/tmp/%s.crond.pid"

static pthread_once_t init_lockfile_path_once = PTHREAD_ONCE_INIT;
static char*          lockfile_path;

static void
init_lockfile_path (void) {
  lockfile_path
    = usr.root ? SYS_LOCKFILE_PATH : s_fmt(USR_LOCKFILE_PATH_FMT, usr.uname);
}

static char*
get_lockfile_path (void) {
  pthread_once(&init_lockfile_path_once, init_lockfile_path);

  return lockfile_path;
}

retval_t
daemonize (void) {
  // Fork a new process
  pid_t pid;
  if ((pid = fork()) < 0) {
    perror("fork");

    return ERR;
  } else if (pid > 0) {
    // Exit the parent process without performing atexit tasks
    _exit(EXIT_SUCCESS);
  }

  // Close std file descriptors to isolate the daemon process
  fclose(stdin);
  fclose(stdout);

  // ...and redirect them to /dev/null to prevent side-effects of unintended
  // writes to them (in case of accidental opens)
  int i;
  if ((i = open(DEV_NULL, O_RDWR)) < 0) {
    perror("open"
           " " DEV_NULL);
    return ERR;
  }
  dup2(i, STDIN_FILENO);
  dup2(i, STDOUT_FILENO);

  // Reset perms so we can set them explicitly
  umask(0);
  // Reset to root dir to prevent a dangling reference to the parent's cwd
  // if (chdir("/") != 0) {
  //   perror("chdir(\"/\")");
  //   return ERR;
  // };

  // Become the session leader of a new process group
  // so we can control all processes using the gid
  if (setsid() < 0) {
    perror("setsid");
    return ERR;
  }

  // Detach from terminal
  // TODO: use double-fork instead? (orphaned process cannot acquire
  // controlling terminal)
  int fd;
  if ((fd = open(DEV_TTY, O_RDWR)) >= 0) {
    ioctl(fd, TIOCNOTTY, 0);
    close(fd);
  }

  return OK;
}

void
daemon_lock (void) {
  int  fd;
  char buf[LOCKFILE_BUFFER_SZ];

  char* lp = get_lockfile_path();
  printlogf("lockfile path: %s\n", lp);
  // Initially, we set the perms to be r/w by owner only to prevent race
  // conds.
  if ((fd = open(lp, O_RDWR | O_CREAT, 0600)) == -1) {
    panic("failed to open/create %s", lp, strerror(errno));
  }

  if (flock(fd, LOCK_EX /* exclusive */ | LOCK_NB /* non-blocking */) < OK) {
    printlogf("flock error %s\n", strerror(errno));

    int read_bytes = -1;
    if ((read_bytes = read(fd, buf, sizeof(buf) - 1) < OK)) {
      printlogf("read error %s\n", strerror(errno));
    }
    close(fd);

    if (read_bytes > 0) {
      errno = 0;
      char* endptr;
      long  pid = strtol(buf, &endptr, 10);
      if (errno != 0 || (endptr && *endptr != '\n')) {
        printlogf("cannot acquire dedupe lock; unable to determine if that's "
                  "because there's already a running instance\n");
      } else {
        printlogf(
          "cannot acquire dedupe lock; there's already a running instance "
          "with pid %ld\n",
          pid
        );
      }
    }

    panic("%s\n", "failed to acquire dedupe lock");
  }

  fchmod(fd, 0644);

  lseek(fd, (off_t)0, SEEK_SET);
  sprintf(buf, "%d\n", getpid());
  int bytes_writ = write(fd, buf, strlen(buf));
  ftruncate(fd, bytes_writ);
  // Leave the file open and locked
}

void
daemon_quit () {
  printlogf("received a kill signal; shutting down...\n");
  logger_close();
  unlink(get_lockfile_path());
  free(opts.log_file);
  array_free(job_queue, (free_fn*)free_cronjob);
  array_free(mail_queue, (free_fn*)free_mailjob);

  exit(EXIT_SUCCESS);
}

void
setup_sig_handlers (void) {
  struct sigaction sa;

  sigemptyset(&sa.sa_mask);
  // restart any system calls that were interrupted by signal
  sa.sa_flags   = SA_RESTART;
  sa.sa_handler = daemon_quit;
  if (sigaction(SIGINT, &sa, NULL) < OK || sigaction(SIGTERM, &sa, NULL)) {
    panic(
      "[%s@L%d] failed to setup signal handler: %s\n",
      __func__,
      __LINE__,
      strerror(errno)
    );
  }
  // TODO: reap on sigchld
}
