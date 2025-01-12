#include "api/commands.h"

#include <pthread.h>
#include <string.h>
#include <unistd.h>

#include "globals.h"
#include "job.h"
#include "panic.h"
#include "proginfo.h"
#include "util.h"

typedef struct {
  const char* command;
  void (*handler)(int);
} command_handler_entry;

static void write_jobs_command(int client_fd);
static void write_crontabs_command(int client_fd);
static void write_program_info(int client_fd);

static command_handler_entry command_handler_map_index[] = {
  {.command = "IPC_LIST_JOBS",     .handler = write_jobs_command    },
  {.command = "IPC_LIST_CRONTABS", .handler = write_crontabs_command},
  {.command = "IPC_SHOW_INFO",     .handler = write_program_info    }
};

static pthread_once_t command_handlers_map_init_once = PTHREAD_ONCE_INIT;
static hash_table*    command_handlers_map;

static void
write_jobs_command (int client_fd) {
  write(client_fd, "[", 2);
  unsigned int len = array_size(job_queue);
  foreach (job_queue, i) {
    job_t* job = (job_t*)array_get_or_panic(job_queue, i);
    if (job->type != CRON) {
      continue;
    }

    char buf[128];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", localtime(&job->next_run));

    char* s = s_fmt(
      "{\"id\":\"%s\",\"cmd\":\"%s\",\"mailto\":\"%s\",\"state\":\"%s\","
      "\"next\":\"%s\"}%s",
      job->ident,
      job->cmd,
      job->mailto,
      job_state_names[job->state],
      buf,
      i != len - 1 ? ",\n" : ""
    );

    write(client_fd, s, strlen(s));
    free(s);
  }
  write(client_fd, "]\n", 3);
}

static void
write_crontabs_command (int client_fd) {
  write(client_fd, "[", 2);

  unsigned int entries = db->count;
  HT_ITER_START(db)
  entries--;
  crontab_t*   ct  = entry->value;
  unsigned int len = array_size(ct->entries);
  foreach (ct->entries, i) {
    cron_entry* ce = array_get_or_panic(ct->entries, i);

    char buf[128];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", localtime(&ce->next));

    char* se = s_concat_arr(ct->envp, ", ");

    char* s  = s_fmt(
      "{\"id\":\"%d\",\"cmd\":\"%s\",\"schedule\":\"%s\",\"owner\":\"%s\","
       "\"envp\":\"%s\",\"next\":\"%s\"}%s",
      ce->id,
      ce->cmd,
      ce->schedule,
      ct->uname,
      se,
      buf,
      entries != 0 || i != len - 1 ? ",\n" : ""
    );

    write(client_fd, s, strlen(s));
    free(s);
  }
  HT_ITER_END

  write(client_fd, "]\n", 3);
}

static void
write_program_info (int client_fd) {
  char*  st     = to_time_str(proginfo.start);
  time_t now    = time(NULL);

  double diff   = difftime(now, proginfo.start);
  char*  uptime = pretty_print_seconds(diff);

  char* s       = s_fmt(
    "{\n\t\"pid\": \"%d\",\n\t\"started_at\": \"%s\",\n\t\"uptime\": \"%s\","
          "\n\t\"version\": \"%s\"\n}\n",
    proginfo.pid,
    st,
    uptime,
    proginfo.version
  );
  write(client_fd, s, strlen(s));

  free(s);
  free(uptime);
}

static void
command_handlers_map_init (void) {
  command_handlers_map = ht_init(11, NULL);

  for (unsigned int i = 0; i < sizeof(command_handler_map_index) / sizeof(command_handler_entry); i++) {
    command_handler_entry entry = command_handler_map_index[i];
    ht_insert(command_handlers_map, entry.command, (void*)entry.handler);
  }
}

hash_table*
get_command_handlers_map (void) {
  pthread_once(&command_handlers_map_init_once, command_handlers_map_init);

  return command_handlers_map;
}
