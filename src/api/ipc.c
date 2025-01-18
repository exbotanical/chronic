#include "api/ipc.h"

#include <errno.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdbool.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include "api/commands.h"
#include "constants.h"
#include "job.h"
#include "logger.h"
#include "utils/file.h"
#include "utils/json.h"
#include "utils/xpanic.h"

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

#define SOCKET_PATH          "/tmp/chronic.sock"
#define MAX_CONCURRENT_CONNS 5
#define RECV_BUFFER_SIZE     1024
#define ERROR_MESSAGE_FMT    "{\"error\":\"%s\"}"

static int server_fd = -1;

static void
ipc_write_err (int client_fd, const char* msg, ...) {
  char out_a[128];
  char out_b[256];

  va_list va;
  va_start(va, msg);
  vsnprintf(out_a, sizeof(out_a), msg, va);
  va_end(va);

  snprintf(out_b, sizeof(out_b), ERROR_MESSAGE_FMT, out_a);

  write(client_fd, out_b, strlen(out_b));
}

static void*
ipc_routine (void* arg __attribute__((unused))) {
  int  client_fd;
  char buffer[RECV_BUFFER_SIZE];
  log_debug("%s\n", "in ipc routine");

  while (true) {
    if ((client_fd = accept(server_fd, NULL, NULL)) == -1) {
      log_warn("failed to accept client conn (reason: %s)\n", strerror(errno));
      continue;
    }

    read(client_fd, buffer, sizeof(buffer));
    log_debug("API req: '%s'\n", buffer);

    hash_table* pairs = ht_init(11, free);
    if (parse_json(buffer, pairs) != OK) {
      ipc_write_err(client_fd, "invalid format");
      goto client_done;
    }

    char* command = ht_get(pairs, "command");
    if (!command) {
      ipc_write_err(client_fd, "missing command");
      goto client_done;
    }

    log_debug("received command '%s'\n", command);
    void (*handler)(int) = ht_get(get_command_handlers_map(), command);

    if (!handler) {
      ipc_write_err(client_fd, "unknown command '%s'", command);
      goto client_done;
    }

    handler(client_fd);

  client_done:
    memset(buffer, 0, RECV_BUFFER_SIZE);
    ht_delete_table(pairs);
    close(client_fd);
  }

  return NULL;
}

static void
ipc_routine_init (void) {
  pthread_t      reaper_thread_id;
  pthread_attr_t attr;
  int            rc = pthread_attr_init(&attr);
  if (rc != 0) {
    xpanic("pthread_attr_init failed with rc %d\n", rc);
  }
  if ((rc = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED)) != 0) {
    xpanic("pthread_attr_setdetachstate failed with rc %d\n", rc);
  }
  pthread_create(&reaper_thread_id, &attr, &ipc_routine, NULL);
}

void
ipc_init (void) {
  struct sockaddr_un addr;

  if ((server_fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
    xpanic("failed to create socket (reason: %s)", strerror(errno));
  }

  memset(&addr, 0, sizeof(addr));
  addr.sun_family = AF_UNIX;
  strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);
  if (file_exists(SOCKET_PATH)) {
    log_warn("socket path %s already exists. removing...\n", SOCKET_PATH);
    unlink(SOCKET_PATH);
  }

  if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
    log_error("failed to bind IPC server sock (reason: %s)\n", strerror(errno));
    xpanic("failed to bind socket");
  }

  if (listen(server_fd, MAX_CONCURRENT_CONNS) == -1) {
    log_error("failed to listen on IPC server sock (reason: %s)\n", strerror(errno));
    xpanic("failed to listen on socket");
  }

  ipc_routine_init();
  log_debug("%s\n", "listening on domain socket...");
}

void
ipc_shutdown (void) {
  log_debug("%s\n", "shutting down socket server...");
  close(server_fd);
  unlink(SOCKET_PATH);
}
