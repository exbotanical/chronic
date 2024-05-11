#include "job.h"

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

#include "constants.h"
#include "cronentry.h"
#include "libutil/libutil.h"
#include "log.h"
#include "opt-constants.h"
#include "util.h"

// Mutex + cond var for the reaper daemon thread.
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t  cond  = PTHREAD_COND_INITIALIZER;

/**
 * Creates a new job of type CRON.
 *
 * @param entry The entry which this job will represent.
 * @return Job*
 *
 * TODO: pass entry by value? Otherwise if entry is freed before we call
 * fork, we're fucked.
 */
static Job*
new_cronjob (CronEntry* entry) {
  Job* job     = xmalloc(sizeof(Job));
  job->ret     = -1;
  job->pid     = -1;
  job->state   = PENDING;
  job->cmd     = s_copy(entry->cmd);  // TODO: free
  job->ident   = create_uuid();
  job->type    = CRON;

  ht_record* r = ht_search(entry->parent->vars, MAILTO_ENVVAR);
  job->mailto  = s_copy(r ? r->value : entry->parent->uname);

  return job;
}

/**
 * Creates a new job of type MAIL.
 *
 * @param og_job The original job about which this MAIL job is reporting.
 * @return Job*
 */
static Job*
new_mailjob (Job* og_job) {
  Job* job    = xmalloc(sizeof(Job));
  job->ret    = -1;
  job->pid    = -1;
  job->state  = PENDING;
  job->ident  = create_uuid();
  job->type   = MAIL;
  job->mailto = og_job->mailto;

  char mail_cmd[MED_BUFFER];
  sprintf(
    mail_cmd,
    MAILCMD_FMT,
    MAILCMD_PATH,
    DAEMON_IDENT,
    hostname,
    og_job->cmd,
    og_job->mailto
  );

  job->cmd = s_copy(mail_cmd);  // TODO: free

  return job;
}

/**
 * Checks for a return status on a RUNNING job's process.
 *
 * @param pid The process id to check.
 * @param status An int pointer into which the job's return status will be
 * stored, if applicable.
 * @return true when the job has finished and the status pointer has been set.
 * @return false when the job has not finished yet. The status pointer was
 * unused (so don't use it!).
 */
static bool
check_job (pid_t pid, int* status) {
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
/**
 * Creates and runs a MAIL job to report the given EXITED job `exited_job`.
 *
 * @param exited_job The EXITED job to report in the MAIL job.
 */
static void
run_mailjob (Job* exited_job) {
  Job* job = new_mailjob(exited_job);
  array_push(mail_queue, job);

  printlogf("[job %s] going to run mail cmd: %s\n", job->ident, job->cmd);

  if ((job->pid = fork()) == 0) {
    setsid();
    dup2(STDERR_FILENO, STDOUT_FILENO);

    FILE* mail_pipe = popen(job->cmd, "w");

    if (mail_pipe == NULL) {
      perror("popen");
      exit(EXIT_FAILURE);
    }

    fprintf(
      mail_pipe,
      fmt_str(
        "This is the body of the email (num %d).\n",
        ++temporary_mail_count
      )
    );

    if (pclose(mail_pipe) == -1) {
      perror("pclose");
      exit(EXIT_FAILURE);
    }

    exit(0);
  }
  job->state = RUNNING;
}

/**
 * Execute a cronjob for the given entry.
 * @param entry
 */
static void
run_cronjob (CronEntry* entry) {
  Job* job = new_cronjob(entry);

  pthread_mutex_lock(&mutex);
  array_push(job_queue, job);
  pthread_mutex_unlock(&mutex);

  char* home  = ht_get(entry->parent->vars, HOMEDIR_ENVVAR);
  char* shell = ht_get(entry->parent->vars, SHELL_ENVVAR);

  if ((job->pid = fork()) == 0) {
    // Detach from the crond and become session leader
    setsid();

    dup2(STDERR_FILENO, STDOUT_FILENO);

    printlogf(
      "[job %s] Writing log from child process pid=%d homedir=%s shell=%s "
      "cmd=%s\n",
      job->ident,
      getpid(),
      home,
      shell,
      job->cmd
    );

    chdir(home);

    int r = execle(shell, shell, "-c", job->cmd, NULL, entry->parent->envp);

    printlogf("execle failed with %d\n", r);
    perror("execle");
    _exit(EXIT_FAILURE);
  }

  printlogf("[job %s] New running job with pid %d\n", job->ident, job->pid);

  job->state = RUNNING;
}

static void
reap_job (Job* job) {
  switch (job->state) {
    case PENDING:
    case EXITED:
    default: break;
    case RUNNING: {
      int status;
      if (check_job(job->pid, &status)) {
        printlogf(
          "[job %s] transition RUNNING->EXITED (pid=%d, status=%d\n",
          job->ident,
          job->pid,
          status
        );

        job->ret   = status;
        job->state = EXITED;
        job->pid   = -1;
      }
    }
  }
}

/**
 * Reaps RUNNING jobs and keeps the process space clean.
 *
 * @param _arg Mandatory (but thus far unused) void pointer thread argument
 * @return void* Ditto ^
 */
static void*
reap_routine (void* _arg) {
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
        if (job->type == CRON) {
          run_mailjob(job);
        }
      }
    }

    foreach (mail_queue, i) {
      Job* job = array_get(mail_queue, i);
      printlogf("\nCHECKING MAIL JOB\n\n");
      reap_job(job);

      if (job->state == EXITED) {
        printlogf("[mail %s] finally exited\n", job->ident);
        array_remove(mail_queue, i);
      }
    }

    pthread_mutex_unlock(&mutex);
  }

  return NULL;
}

void
init_reap_routine (void) {
  pthread_t      reaper_thread_id;
  pthread_attr_t attr;
  int            rc = pthread_attr_init(&attr);
  rc = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

  pthread_create(&reaper_thread_id, &attr, &reap_routine, NULL);
}

void
signal_reap_routine (void) {
  pthread_mutex_lock(&mutex);
  pthread_cond_signal(&cond);
  pthread_mutex_unlock(&mutex);
}

void
run_jobs (hash_table* db, time_t ts) {
  // We must iterate the capacity here because hash table records are not
  // stored contiguously
  if (db->count > 0) {
    for (unsigned int i = 0; i < (unsigned int)db->capacity; i++) {
      ht_record* r = db->records[i];

      // If there's no record in this slot, continue
      if (!r) {
        continue;
      }

      Crontab* ct = r->value;
      foreach (ct->entries, i) {
        CronEntry* entry = array_get(ct->entries, i);
        if (entry->next == ts) {
          run_cronjob(entry);
        }
      }
    }
  }
}
