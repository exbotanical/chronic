#define _GNU_SOURCE        // For REG_RIP
#define _XOPEN_SOURCE   1  // For sigaction
#define _DEFAULT_SOURCE 1  // For SA_RESTART

#include "sig.h"

#include <errno.h>
#include <signal.h>
#include <string.h>
#include <ucontext.h>

#include "daemon.h"
#include "globals.h"
#include "logger.h"
#include "panic.h"

static void
handle_exit (int sig) {
  log_info("received a kill signal (%d); shutting down...\n", sig);
  daemon_shutdown();
}

static void
handle_sigsegv (int sig, siginfo_t* info, void* context) {
  log_error("Caught SIGSEGV (signal %d)\n", sig);
  log_error("Fault address: %p\n", info->si_addr);

  switch (info->si_code) {
    case SEGV_MAPERR:
      log_error("Reason: Address not mapped to object (SEGV_MAPERR)\n");
      break;
    case SEGV_ACCERR:
      log_error("Reason: Invalid permissions for mapped object (SEGV_ACCERR)\n");
      break;
    default: log_error("Reason: Unknown (code %d)\n", info->si_code); break;
  }

  ucontext_t* uc = (ucontext_t*)context;
#ifdef __x86_64__
  log_debug(
    "Instruction Pointer (%%rip): 0x%llx\n",
    (unsigned long long)uc->uc_mcontext.gregs[REG_RIP]
  );
  log_debug(
    "Stack Pointer (%%rsp): 0x%llx\n",
    (unsigned long long)uc->uc_mcontext.gregs[REG_RSP]
  );
  log_debug(
    "Faulting Address Register (%%cr2): 0x%llx\n",
    (unsigned long long)info->si_addr
  );
#elif __i386__
  log_debug(
    "Instruction Pointer (%%eip): 0x%lx\n",
    (unsigned long)uc->uc_mcontext.gregs[REG_EIP]
  );
  log_debug("Stack Pointer (%%esp): 0x%lx\n", (unsigned long)uc->uc_mcontext.gregs[REG_ESP]);
#else
  log_debug("CPU context not supported on this architecture.\n");
#endif

  daemon_shutdown();
}

static void
handle_sighup (int sig) {
  // TODO: inotify
  logger_reinit();
  log_info("logfile was closed (SIGHUP); re-opened\n");
}

static void
sigaction_init (struct sigaction* sa) {
  sigemptyset(&sa->sa_mask);

  sa->sa_flags =
    // restart any system calls that were interrupted by signal
    SA_RESTART
    // Extended signal info
    | SA_SIGINFO;
}

// TODO: reap on sigchld
void
sig_handlers_init (void) {
  struct sigaction sa_exit;
  sigaction_init(&sa_exit);
  sa_exit.sa_handler = handle_exit;

  struct sigaction sa_segfault;
  sigaction_init(&sa_segfault);
  sa_segfault.sa_sigaction  = handle_sigsegv;
  sa_segfault.sa_flags     |= SA_SIGINFO;

  struct sigaction sa_hangup;
  sigaction_init(&sa_hangup);
  sa_hangup.sa_handler = handle_sighup;

  // clang-format off
  if (sigaction(SIGINT, &sa_exit, NULL) < 0
    || sigaction(SIGTERM, &sa_exit, NULL) < 0
    || sigaction(SIGQUIT, &sa_exit, NULL) < 0
    || sigaction(SIGSEGV, &sa_segfault, NULL) < 0
    || sigaction(SIGHUP, &sa_hangup, NULL) < 0) {
    panic("Failed to setup signal handlers: %s", strerror(errno));
  }
  // clang-format on
}
