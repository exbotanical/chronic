#include "job.h"

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include <uuid/uuid.h>

#include "constants.h"
#include "cronentry.h"
#include "libutil/libutil.h"
#include "log.h"
#include "opt-constants.h"
#include "util.h"

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

// need to free
static char* create_uuid(void) {
  char uuid[UUID_STR_LEN];

  uuid_t bin_uuid;
  uuid_generate_random(bin_uuid);
  uuid_unparse(bin_uuid, uuid);

  return s_copy(uuid);
}

// TODO: pass entry by value? Otherwise if entry is freed before we call fork,
// we're fucked
static Job* new_cronjob(CronEntry* entry) {
  Job* job = xmalloc(sizeof(Job));
  job->ret = -1;
  job->pid = -1;
  job->state = PENDING;
  job->cmd = s_copy(entry->cmd);  // TODO: free
  job->ident = create_uuid();
  job->type = CMD;

  ht_record* r = ht_search(entry->parent->vars, MAILTO_ENVVAR);
  job->mailto = s_copy(r ? r->value : entry->parent->uname);

  return job;
}

static Job* new_mailjob(Job* og_job) {
  Job* job = xmalloc(sizeof(Job));
  job->ret = -1;
  job->pid = -1;
  job->state = PENDING;
  job->ident = create_uuid();
  job->type = MAIL;
  job->mailto = og_job->mailto;

  char mail_cmd[MED_BUFFER];
  sprintf(mail_cmd, MAILCMD_FMT, MAILCMD_PATH, DAEMON_IDENT, hostname,
          og_job->cmd, og_job->mailto);

  job->cmd = s_copy(mail_cmd);  // TODO: free

  return job;
}

static bool await_job(pid_t pid, int* status) {
  int r = waitpid(pid, status, WNOHANG);

  printlogf("[pid=%d] waitpid result is %d\n", pid, r);

  // -1 == error; 0 == still running; pid == dead
  if (r < 0 || r == pid) {
    if (r > 0 && WIFEXITED(*status)) {
      *status = WEXITSTATUS(*status);
    } else {
      *status = 1;
    }
    return true;
  }

  return false;
}

static unsigned int temporary_mail_count = 0;

// TODO: [bash] <defunct>
// ps aux | grep './chronic' | grep -v 'grep' | awk '{print $2}' | wc -l
// ps aux | grep './chronic' | grep -v 'grep' | awk '{print $2}' | xargs kill
static void run_mailjob(Job* og_job) {
  Job* job = new_mailjob(og_job);
  array_push(job_queue, job);

  printlogf("[job %s] going to run mail cmd: %s\n", job->ident, job->cmd);

  if ((job->pid = fork()) == 0) {
    setsid();
    dup2(STDERR_FILENO, STDOUT_FILENO);

    FILE* mail_pipe = popen(job->cmd, "w");

    if (mail_pipe == NULL) {
      perror("popen");
      exit(EXIT_FAILURE);
    }

    fprintf(mail_pipe, fmt_str("This is the body of the email (num %d).\n",
                               ++temporary_mail_count));

    if (pclose(mail_pipe) == -1) {
      perror("pclose");
      exit(EXIT_FAILURE);
    }
  }
}

// void enqueue_job(CronEntry* entry) {
//   Job* job = new_cronjob(entry);
//   array_push(job_queue, job);
// }

void run_cronjob(CronEntry* entry) {
  Job* job = new_cronjob(entry);
  array_push(job_queue, job);

  char* home = ht_get(entry->parent->vars, HOMEDIR_ENVVAR);
  char* shell = ht_get(entry->parent->vars, SHELL_ENVVAR);

  if ((job->pid = fork()) == 0) {
    // Detach from the crond and become session leader
    setsid();

    dup2(STDERR_FILENO, STDOUT_FILENO);

    printlogf(
        "[job %s] Writing log from child process pid=%d homedir=%s shell=%s "
        "cmd=%s\n",
        job->ident, getpid(), home, shell, job->cmd);

    chdir(home);

    int r = execle(shell, shell, "-c", job->cmd, NULL, entry->parent->envp);

    printlogf("execle failed with %d\n", r);
    perror("execle");
    _exit(EXIT_FAILURE);
  }

  printlogf("[job %s] New running job with pid %d\n", job->ident, job->pid);

  job->state = RUNNING;
}

// ps aux | grep './chronic' | awk '{print $2}' | xargs kill
static void reap_job(Job* job) {
  switch (job->state) {
    case PENDING:
    case EXITED:
    default:
      break;
    case RUNNING: {
      int status;
      if (await_job(job->pid, &status)) {
        printlogf("[job %s] transition RUNNING->EXITED (pid=%d, status=%d\n",
                  job->ident, job->pid, status);

        job->ret = status;
        job->state = EXITED;
        job->pid = -1;

        if (job->type == CMD) {
          // run_mailjob(job);
        }
      }
    }
  }
}

static void* reap_routine(void* _arg) {
  while (true) {
    pthread_mutex_lock(&mutex);
    pthread_cond_wait(&cond, &mutex);

    printlogf("in reaper thread\n");
    foreach (job_queue, i) {
      Job* job = array_get(job_queue, i);
      // TODO: null checks everywhere ^
      reap_job(job);

      // We need to be careful to only remove EXITED jobs, else
      // we'll end up with zombley processes
      if (job->state == EXITED) {
        array_remove(job_queue, i);
      }
    }

    pthread_mutex_unlock(&mutex);
  }

  return NULL;
}

void init_reap_routine(void) {
  pthread_t reaper_thread_id;
  pthread_attr_t attr;
  int rc = pthread_attr_init(&attr);
  rc = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

  pthread_create(&reaper_thread_id, &attr, &reap_routine, NULL);
}

void signal_reap_routine(void) {
  pthread_mutex_lock(&mutex);
  pthread_cond_signal(&cond);
  pthread_mutex_unlock(&mutex);
}
