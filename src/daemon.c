#include "defs.h"

bool daemonize() {
  pid_t pid;
  if ((pid = fork()) < 0) {
    perror("fork");

    return false;
  } else if (pid > 0) {
    _exit(0);
  }

  close(STDIN_FILENO);
  close(STDOUT_FILENO);

  int i;
  if ((i = open("/dev/null", O_RDWR)) < 0) {
    perror("open /dev/null");
    return false;
  }

  dup2(i, 0);
  dup2(i, 1);

  // become the session leader of a new process group
  // so we can control all processes using the gid
  if (setsid() < 0) {
    perror("setsid");
    return false;
  }

  // Detach from terminal and become daemon
  int fd;
  if ((fd = open("/dev/tty", O_RDWR)) >= 0) {
    ioctl(fd, TIOCNOTTY, 0);
    close(fd);
  }

  return true;
}

void reap(Job *job) {
  write_to_log("hello from the reaper...\n");

  int status;
  int r = waitpid(job->pid, &status, WNOHANG);

  // -1 == error; 0 == still running; pid == dead
  if (r < 0 || r == job->pid) {
    if (r > 0 && WIFEXITED(status))
      status = WEXITSTATUS(status);
    else
      status = 1;

    job->ret = status;
    job->status = EXITED;
  }
}
