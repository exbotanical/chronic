#define _XOPEN_SOURCE   1  // For sigaction
#define _DEFAULT_SOURCE 1  // For SA_RESTART
#include "daemon.h"

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <unistd.h>

#include "cli.h"
#include "log.h"

#define DEV_NULL           "/dev/null"
#define DEV_TTY            "/dev/tty"
#define LOCKFILE_BUFFER_SZ 512
#define LOCKFILE_PATH      "/var/run/crond.pid"

Retval
daemonize (void) {
  // Fork a new process
  pid_t pid;
  if ((pid = fork()) < 0) {
    perror("fork");

    return ERR;
  } else if (pid > 0) {
    // Exit the parent process without performing atexit tasks
    _exit(0);
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

  // Initially, we set the perms to be r/w by owner only to prevent race conds.
  if ((fd = open(LOCKFILE_PATH, O_RDWR | O_CREAT, 0600)) == -1) {
    printlogf("failed to open/create %s: %s", LOCKFILE_PATH, strerror(errno));
    exit(EXIT_FAILURE);
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
      long  pid = strtol(buf, endptr, 10);
      if (errno != 0 || (endptr && endptr != '\n')) {
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

    exit(EXIT_FAILURE);
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
  unlink(LOCKFILE_PATH);
  exit(EXIT_SUCCESS);
}

void
setup_sig_handlers (void) {
  struct sigaction sa;
  int              n;

  sigemptyset(&sa);
  // restart any system calls that were interrupted by signal
  sa.sa_flags   = SA_RESTART;
  sa.sa_handler = daemon_quit;
  if (sigaction(SIGINT, &sa, NULL) < OK || sigaction(SIGTERM, &sa, NULL)) {
    printlogf("failed to setup signal handler: %s\n", strerror(errno));
    exit(EXIT_FAILURE);
  }
  // TODO: reap on sigchld
}
