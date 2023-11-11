#include "job.h"

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
static Job* new_job(CronEntry* entry) {
  Job* job = xmalloc(sizeof(Job));
  job->ret = -1;
  job->pid = -1;
  job->mailer_pid = -1;
  job->status = PENDING;
  job->cmd = s_copy(entry->cmd);  // TODO: free
  job->ident = create_uuid();

  ht_record* r = ht_search(entry->parent->vars, MAILTO_ENVVAR);
  job->mailto = s_copy(r ? r->value : entry->parent->uname);

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

void enqueue_job(CronEntry* entry) {
  Job* job = new_job(entry);

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

  job->status = RUNNING;
  array_push(job_queue, job);
}

unsigned int temporary_mail_count = 0;
//  TODO: [bash] <defunct>
// execl("/bin/sh", "/bin/sh", "-c", job->cmd, NULL);
// ps aux | grep './chronic' | grep -v 'grep' | awk '{print $2}' | wc -l
// ps aux | grep './chronic' | grep -v 'grep' | awk '{print $2}' | xargs kill
void run_mailjob(Job* mailjob) {
  mailjob->status = MAIL_RUNNING;
  printlogf("[job %s] transition EXITED->MAIL_RUNNING\n", mailjob->ident);

  char mail_cmd[MED_BUFFER];

  sprintf(mail_cmd, MAILCMD_FMT, MAILCMD_PATH, DAEMON_IDENT, hostname,
          mailjob->cmd, mailjob->mailto);

  printlogf("going to run mailcmd: %s\n", mail_cmd);

  if ((mailjob->mailer_pid = fork()) == 0) {
    setsid();
    dup2(STDERR_FILENO, STDOUT_FILENO);

    FILE* mail_pipe = popen(mail_cmd, "w");

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
// ps aux | grep './chronic' | awk '{print $2}' | xargs kill
void reap_job(Job* job) {
  // This shouldn't happen, but just in case...
  if (job->status == RESOLVED) {
    printlogf(
        "[job %s] Tried to reap job with status=%s. This is a "
        "bug.\n",
        job->ident, job_status_names[job->status]);
    return;
  }

  int status;

  if (job->status == MAIL_RUNNING) {
    printlogf("[job %s] await (pid=%d, status=MAIL_RUNNING)\n", job->ident,
              job->mailer_pid);

    // Main job is done, await the mail job
    if (await_job(job->mailer_pid, &status)) {
      printlogf(
          "[job %s] transition MAIL_RUNNING->RESOLVED (pid=%d, status=%d)\n",
          job->ident, job->mailer_pid, status);

      job->status = RESOLVED;
      job->mailer_pid = -1;
      // TODO: cleanup fns
    }
  } else {
    if (await_job(job->pid, &status)) {
      printlogf("[job %s] transition RUNNING->EXITED (pid=%d, status=%d)\n",
                job->ident, job->pid, status);

      job->ret = status;
      job->status = EXITED;
      job->pid = -1;
    }
  }
}
