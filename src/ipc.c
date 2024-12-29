// TODO: Wire all this up - file is currently all rough draft stuff.
#include "ipc.h"

#include <pthread.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include "log.h"
#include "panic.h"

typedef enum {
  IPC_LIST_JOBS,
  IPC_PAUSE_JOB,
  IPC_DELETE_JOB,
  IPC_RESUME_JOB,
  IPC_JOB_INFO,
} ipc_command;

typedef enum {
  JOB_RUN_STATE_ACTIVE,
  JOB_RUN_STATE_PAUSED,
} job_run_state;

typedef struct {
  char*         id;
  char*         body;
  char*         schedule;
  unsigned int  created_at;
  unsigned int  last_run;
  unsigned int  runs;
  job_run_state run_state;
} job_info_t;

#define SOCKET_PATH          "/tmp/daemon.sock"
#define MAX_CONCURRENT_CONNS 5
#define RECV_BUFFER_SIZE     1024

static int server_fd = -1;

static void
ipc_routine (void* _arg) {
  int  client_fd;
  char buffer[RECV_BUFFER_SIZE];
  printlogf("in ipc routine\n");

  while (true) {
    if ((client_fd = accept(server_fd, NULL, NULL)) == -1) {
      perror("accept");
      continue;
    }
    printlogf(">>> %s\n", "ccccc");

    read(client_fd, buffer, sizeof(buffer));
    printlogf(">>> %s\n", buffer);
    close(client_fd);
  }
}

static void
init_ipc_routine (void) {
  pthread_t      reaper_thread_id;
  pthread_attr_t attr;
  int            rc = pthread_attr_init(&attr);
  if (rc != 0) {
    panic("pthread_attr_init failed with rc %d\n", rc);
  }
  if ((rc = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED)) != 0) {
    panic("pthread_attr_setdetachstate failed with rc %d\n", rc);
  }
  pthread_create(&reaper_thread_id, &attr, &ipc_routine, NULL);
}

void
init_ipc_server (void) {
  struct sockaddr_un addr;

  if ((server_fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
    perror("socket");
    panic("failed to create socket");
  }

  memset(&addr, 0, sizeof(addr));
  addr.sun_family = AF_UNIX;
  strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);
  unlink(SOCKET_PATH);

  if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
    perror("bind");
    panic("failed to bind socket");
  }

  if (listen(server_fd, MAX_CONCURRENT_CONNS) == -1) {
    perror("listen");
    panic("failed to listen on socket");
  }

  init_ipc_routine();
  printlogf("listening on domain socket");
}
