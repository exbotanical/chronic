#include "daemon.h"

#include <fcntl.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <unistd.h>

#define DEV_NULL_DEVICE "/dev/null"
#define TERMINAL_DEVICE "/dev/tty"

RETVAL daemonize() {
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
  if ((i = open(DEV_NULL_DEVICE, O_RDWR)) < 0) {
    perror(
        "open"
        " " DEV_NULL_DEVICE);
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
  if ((fd = open(TERMINAL_DEVICE, O_RDWR)) >= 0) {
    ioctl(fd, TIOCNOTTY, 0);
    close(fd);
  }

  return OK;
}
