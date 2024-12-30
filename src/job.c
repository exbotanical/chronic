#include "job.h"

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

#include "config.h"
#include "constants.h"
#include "cronentry.h"
#include "libutil/libutil.h"
#include "log.h"
#include "panic.h"
#include "util.h"

// Mutex + cond var for the reaper daemon thread.
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t  cond  = PTHREAD_COND_INITIALIZER;

/**
 * Creates a new job of type CRON.
 *
 * @param entry The entry which this job will represent.
 * @return job_t*
 */
static job_t*
new_cronjob (cron_entry* entry) {
  job_t* job  = xmalloc(sizeof(job_t));
  job->ident  = create_uuid();
  job->type   = CRON;
  job->state  = PENDING;
  job->cmd    = s_copy_or_panic(entry->cmd);
  job->ret    = -1;
  job->pid    = -1;

  ht_entry* r = ht_search(entry->parent->vars, MAILTO_ENVVAR);
  job->mailto = s_copy_or_panic(r ? r->value : entry->parent->uname);

  return job;
}

void
free_cronjob (job_t* job) {
  free(job->cmd);
  free(job->ident);
  free(job->mailto);
  free(job);
}

/**
 * Creates a new job of type MAIL.
 *
 * @param og_job The original job about which this MAIL job is reporting.
 * @return job_t*
 */
static job_t*
new_mailjob (job_t* og_job) {
  job_t* job  = xmalloc(sizeof(job_t));
  job->ident  = create_uuid();
  job->type   = MAIL;
  job->state  = PENDING;
  job->mailto = s_copy_or_panic(og_job->mailto);
  job->ret    = -1;
  job->pid    = -1;

  char mail_cmd[MED_BUFFER];
  sprintf(
    mail_cmd,
    MAILCMD_FMT,
    MAILCMD_PATH,
    DAEMON_IDENT,
    hostname,
    MAIL_SUBJECT,
    og_job->mailto
  );

  job->cmd = s_copy_or_panic(mail_cmd);

  return job;
}

void
free_mailjob (job_t* job) {
  free(job->ident);
  free(job->cmd);
  free(job->mailto);
  free(job);
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

/**
 * Creates and runs a MAIL job to report the given EXITED job `exited_job`.
 *
 * @param exited_job The EXITED job to report in the MAIL job.
 */
static void
run_mailjob (job_t* exited_job) {
  job_t* job = new_mailjob(exited_job);
  array_push_or_panic(mail_queue, job);

  printlogf("[job %s] going to run mail cmd: %s\n", job->ident, job->cmd);

  if ((job->pid = fork()) == 0) {
    setsid();
    dup2(STDERR_FILENO, STDOUT_FILENO);

    FILE* mail_pipe = popen(job->cmd, "w");

    if (mail_pipe == NULL) {
      perror("popen");
      exit(EXIT_FAILURE);
    }

    char* email_body = s_fmt("command: %s", exited_job->cmd);
    fprintf(mail_pipe, email_body);
    free(email_body);

    if (pclose(mail_pipe) == -1) {
      perror("pclose");
      exit(EXIT_FAILURE);
    }

    exit(EXIT_SUCCESS);
  }

  job->state = RUNNING;
}

/**
 * Execute a cronjob for the given entry.
 * @param entry
 */
static void
run_cronjob (cron_entry* entry) {
  job_t* job = new_cronjob(entry);

  pthread_mutex_lock(&mutex);
  array_push_or_panic(job_queue, job);
  pthread_mutex_unlock(&mutex);

  char* home  = ht_get_or_panic(entry->parent->vars, HOMEDIR_ENVVAR);
  char* shell = ht_get_or_panic(entry->parent->vars, SHELL_ENVVAR);

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
    int rc = execle(shell, shell, "-c", job->cmd, NULL, entry->parent->envp);

    printlogf("execle failed with %d\n", rc);
    perror("execle");
    _exit(EXIT_FAILURE);
  }

  printlogf("[job %s] New running job with pid %d\n", job->ident, job->pid);
  job->state = RUNNING;
}

static void
reap_job (job_t* job) {
  switch (job->state) {
    case PENDING: break;
    case EXITED: break;
    case RUNNING: {
      int status;
      if (check_job(job->pid, &status)) {
        printlogf(
          "[job %s] transition RUNNING->EXITED (pid=%d, status=%d)\n",
          job->ident,
          job->pid,
          status
        );

        job->ret   = status;
        job->state = EXITED;
        job->pid   = -1;
      }
    }
    default: break;
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
      job_t* job = array_get_or_panic(job_queue, i);
      reap_job(job);

      // We need to be careful to only remove EXITED jobs, else
      // we'll end up with zombley processes
      if (job->state == EXITED) {
        array_remove(job_queue, i);
        if (job->type == CRON) {
          run_mailjob(job);
          free_cronjob(job);
        }
      }
    }

    foreach (mail_queue, i) {
      job_t* job = array_get_or_panic(mail_queue, i);
      reap_job(job);

      if (job->state == EXITED) {
        printlogf("[mail %s] finally exited\n", job->ident);
        array_remove(mail_queue, i);
        free_mailjob(job);
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
  if (rc != 0) {
    panic("pthread_attr_init failed with rc %d\n", rc);
  }
  if ((rc = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED)) != 0) {
    panic("pthread_attr_setdetachstate failed with rc %d\n", rc);
  }
  pthread_create(&reaper_thread_id, &attr, &reap_routine, NULL);
}

void
signal_reap_routine (void) {
  pthread_mutex_lock(&mutex);
  pthread_cond_signal(&cond);
  pthread_mutex_unlock(&mutex);
}

void
try_run_jobs (hash_table* db, time_t ts) {
  if (db->count > 0) {
    HT_ITER_START(db)
    crontab_t* ct = entry->value;
    foreach (ct->entries, i) {
      cron_entry* ce = array_get_or_panic(ct->entries, i);
      if (ce->next == ts) {
        run_cronjob(ce);
      }
    }
    HT_ITER_END
  }
}
