#include "defs.h"

bool daemonize() {
  // Fork a new process
  pid_t pid;
  if ((pid = fork()) < 0) {
    perror("fork");

    return EXIT_FAILURE;
  } else if (pid > 0) {
    // Exit the parent process without performing atexit tasks
    _exit(0);
  }

  // Close std file descriptors to isolate the daemon process
  close(STDIN_FILENO);
  close(STDOUT_FILENO);

  // ...and redirect them to /dev/null to prevent side-effects of unintended
  // writes to them (in case of accidental opens)
  int i;
  if ((i = open("/dev/null", O_RDWR)) < 0) {
    perror("open /dev/null");
    return EXIT_FAILURE;
  }
  dup2(i, 0);
  dup2(i, 1);

  // Reset perms so we can set them explicitly
  umask(0);
  // Reset to root dir to prevent a dangling reference to the parent's cwd
  if (chdir("/") != 0) {
    perror("chdir(\"/\")");
    return EXIT_FAILURE;
  };

  // Become the session leader of a new process group
  // so we can control all processes using the gid
  if (setsid() < 0) {
    perror("setsid");
    return EXIT_FAILURE;
  }

  // Detach from terminal
  // TODO: use double-fork instead? (orphaned process cannot acquire
  // controlling terminal)
  int fd;
  if ((fd = open("/dev/tty", O_RDWR)) >= 0) {
    ioctl(fd, TIOCNOTTY, 0);
    close(fd);
  }

  return EXIT_SUCCESS;
}

void reap(Job *job) {
  // write_to_log("hello from the reaper...\n");

  // int status;
  // int r = waitpid(job->pid, &status, WNOHANG);

  // // -1 == error; 0 == still running; pid == dead
  // if (r < 0 || r == job->pid) {
  //   if (r > 0 && WIFEXITED(status))
  //     status = WEXITSTATUS(status);
  //   else
  //     status = 1;

  //   job->ret = status;
  //   job->status = EXITED;
  // }
}
