#include "crontab.h"

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <pwd.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "constants.h"
#include "cronentry.h"
#include "log.h"
#include "opt-constants.h"
#include "panic.h"
#include "parser.h"
#include "util.h"

// Some systems won't have this option and we'll just have to live without it.
#ifndef O_NOFOLLOW
#  define O_NOFOLLOW 0
#endif

#define MAXENTRIES 256
#define RW_BUFFER  1024

/**
 * Returns true if the file represented by the stat entity is owned by the
 * user (represented by the pw entity).
 *
 * @param statbuf The stat struct for the file we're evaluating.
 * @param pw The user's passwd entry.
 * @return true The file IS owned by the given user.
 * @return false The file IS NOT owned by the given user.
 */
static bool
is_file_owner (struct stat* statbuf, struct passwd* pw) {
  return statbuf->st_uid == pw->pw_uid;
}

/**
 * Modifies and finalizes the given crontab's environment data by adding any
 * missing env vars. For example, we check for the user's home directory, shell,
 * etc and set those.
 *
 * @param ct The crontab whose environment we're finalizing.
 */
static void
complete_env (crontab_t* ct) {
  hash_table* vars = ct->vars;

// We don't want to have to create users for unit tests.
#ifndef UNIT_TEST
  struct passwd* pw = getpwnam(ct->uname);
  if (pw) {
    if (!ht_search(vars, HOMEDIR_ENVVAR)) {
      // TODO: update ht_insert API to return bool
      ht_insert(vars, HOMEDIR_ENVVAR, s_copy_or_panic(pw->pw_dir));
    }

    if (!ht_search(vars, SHELL_ENVVAR)) {
      ht_insert(vars, SHELL_ENVVAR, s_copy_or_panic(pw->pw_shell));
    }

    if (!ht_search(vars, UNAME_ENVVAR)) {
      ht_insert(vars, UNAME_ENVVAR, s_copy_or_panic(pw->pw_name));
    }

    if (!ht_search(vars, PATH_ENVVAR)) {
      ht_insert(vars, PATH_ENVVAR, DEFAULT_PATH);
    }
  } else {
    printlogf(
      "[ERROR] Something went really awry and uname=%s wasn't found in the "
      "system user db\n",
      ct->uname
    );
  }
#endif
  // Fill out the envp using the vars map. We're going to need this later and
  // forevermore, so we might as well get it out of the way upfront.
  if (vars->count > 0) {
    ct->envp         = xmalloc(sizeof(char*) * vars->count + 1);

    unsigned int idx = 0;
    for (unsigned int i = 0; i < (unsigned int)vars->capacity; i++) {
      ht_entry* r = vars->entries[i];
      if (!r) {
        continue;
      }

      ct->envp[idx] = s_fmt("%s=%s", r->key, r->value);
      idx++;
    }

    // `execle` requires it to be NULL-terminated
    ct->envp[idx] = NULL;
  }
}

int
get_crontab_fd_if_valid (char* fpath, char* uname, time_t last_mtime, struct stat* statbuf) {
  int            not_ok     = OK - 1;
  struct passwd* pw         = NULL;
  int            crontab_fd = not_ok;

#ifndef UNIT_TEST
  // No user found in passwd
  if (!s_equals(uname, ROOT_UNAME) && (pw = getpwnam(uname)) == NULL) {
    printlogf("user %s not found\n", uname);
    goto dont_process;
  }
#endif

  // Open in readonly, non-blocking mode. Don't follow symlinks.
  // Not following symlinks is largely for security
  if ((crontab_fd = open(fpath, O_RDONLY | O_NONBLOCK | O_NOFOLLOW, 0)) < OK) {
    printlogf("cannot read %s (reason=%s)\n", fpath, strerror(errno));
    goto dont_process;
  }

  if (fstat(crontab_fd, statbuf) < OK) {
    printlogf("fstat failed for fd %d (file %s)\n", crontab_fd, fpath);
    goto dont_process;
  }

  if (!S_ISREG(statbuf->st_mode)) {
    printlogf("file %s is not a regular file");
    goto dont_process;
  }

#ifndef UNIT_TEST
  int file_perms = statbuf->st_mode & ALL_PERMS;
  if (file_perms != OWNER_RW_PERMS) {
    printlogf(
      "file %s has invalid permissions (should be r/w by owner only but got "
      "%o)\n",
      fpath,
      file_perms
    );
    goto dont_process;
  }

  // Verify that the user the file is named after actually owns said file,
  // unless we're root.
  if (!usr.root && !is_file_owner(statbuf, pw)) {
    printlogf("file %s not owned by specified user %s (it's owned by %s)\n", fpath, uname, pw->pw_name);
    goto dont_process;
  }

  // This time we're checking that the user who executed this program matches
  // the crontab owner, unless we're root;.
  if (!usr.root && pw->pw_uid != usr.uid) {
    printlogf("current user %s must own this crontab (%s) or be root to schedule it\n ", uname, fpath);
    goto dont_process;
  }
#endif

  // Ensure there aren't any hard links
  if (statbuf->st_nlink != 1) {
    printlogf("file %s has hard symlinks (%d)\n", fpath, statbuf->st_nlink);
    goto dont_process;
  }

  if (last_mtime > -1) {
    // It shouldn't ever be greater than but whatever
    if (last_mtime >= statbuf->st_mtime) {
      printlogf(
        "file has not changed (last mtime: %ld, curr mtime: %ld)\n",
        fpath,
        last_mtime,
        statbuf->st_mtime
      );
      goto dont_process;
    }
  }

  return crontab_fd;

dont_process:
  return not_ok;
}

crontab_t*
new_crontab (int crontab_fd, bool is_root, time_t curr_time, time_t mtime, char* uname) {
  FILE* fd;
  if (!(fd = fdopen(crontab_fd, "r"))) {
    printlogf("fdopen on crontab_fd %d failed\n", crontab_fd);
    perror("fdopen");
    return NULL;
  }

  unsigned int max_entries;
  unsigned int max_lines;

  if (is_root) {
    max_entries = 65535;
  } else {
    max_entries = MAXENTRIES;
  }

  max_lines = max_entries * 10;

  char buf[RW_BUFFER];

  crontab_t* ct = xmalloc(sizeof(crontab_t));
  ct->mtime     = mtime;
  ct->uname     = uname;
  ct->envp      = NULL;
  ct->entries   = array_init_or_panic();
  ct->vars      = ht_init_or_panic(0, free);

  while (fgets(buf, sizeof(buf), fd) != NULL && --max_lines) {
    char* ptr = buf;

    switch (parse_line(ptr, max_entries, ct->vars)) {
      case ENV_VAR_ADDED:
      case SKIP_LINE: continue;
      case DONE: break;
      case ENTRY: {
        cron_entry* entry = new_cron_entry(ptr, curr_time, ct);
        if (!entry) {
          printlogf(
            "Failed to parse what was thought to be a cron entry: %s (user "
            "%s)\n",
            ptr,
            uname
          );
          continue;
        }

        printlogf("New entry (%d) for crontab %s\n", entry->id, uname);
        array_push_or_panic(ct->entries, entry);
        max_entries--;
      }
      default: break;
    }
  }

  fclose(fd);
  complete_env(ct);

  return ct;
}

void
free_crontab (crontab_t* ct) {
  free(ct->uname);
  array_free(ct->entries, (free_fn*)free_cron_entry);
  ht_delete_table(ct->vars);
  if (ct->envp) {
    free(ct->envp);
  }
  free(ct);
}

void
scan_crontabs (hash_table* old_db, hash_table* new_db, dir_config dir_conf, time_t curr) {
  array_t* fnames = get_filenames(dir_conf.path);
  // If no files, fall through to db replacement
  // This will handle removal of any files that were deleted during runtime
  if (has_elements(fnames)) {
    foreach (fnames, i) {
      char* fname = array_get_or_panic(fnames, i);

      char* fpath;
      if (!(fpath = s_fmt("%s/%s", dir_conf.path, fname))) {
        printlogf("failed to concatenate as %s/%s\n", dir_conf.path, fname);
        continue;
      }
      crontab_t*  ct = ht_get(old_db, fpath);
      int         crontab_fd;
      struct stat statbuf;

      printlogf("scanning file %s...\n", fpath);

      char* uname = dir_conf.is_root ? ROOT_UNAME : fname;
      // File hasn't been processed before
      if (!ct) {
        if ((crontab_fd = get_crontab_fd_if_valid(fpath, uname, 0, &statbuf)) < OK) {
          printlogf("file %s not valid; continuing...\n", fpath);
          continue;
        }

        printlogf("creating new crontab from file %s...\n", fpath);

        ct = new_crontab(crontab_fd, dir_conf.is_root, curr, statbuf.st_mtime, s_copy_or_panic(fname));
        ht_insert(new_db, fpath, ct);
      }
      // File exists in db
      else {
        printlogf("crontab for file %s exists...\n", fpath);

        // Renew the fd and statbuf
        if ((crontab_fd = get_crontab_fd_if_valid(fpath, uname, -1, &statbuf)) < OK) {
          printlogf(
            "existing crontab file %s not valid; it won't be carried over. "
            "continuing...\n",
            fpath
          );
          continue;
        }

        // The crontab was not modified, just renew the entries so we know when
        // to run them next
        if (ct->mtime >= statbuf.st_mtime) {
          printlogf("existing file %s not modified, renewing entries if any\n", fpath);
          foreach (ct->entries, i) {
            cron_entry* entry = array_get_or_panic(ct->entries, i);
            renew_cron_entry(entry, curr);
          }
        } else {
          printlogf("existing file %s was modified, recreating crontab\n", fpath);
          // modified, re-process
          ct = new_crontab(crontab_fd, dir_conf.is_root, curr, statbuf.st_mtime, s_copy_or_panic(fname));
        }

        // set the og entry to NULL
        old_db->free_value = NULL;
        ht_insert(old_db, fpath, NULL);
        // TODO: ??? - also ^this^ causes dangling refs
        old_db->free_value = (free_fn*)free_crontab;

        ht_insert(new_db, fpath, ct);
      }
    }

    array_free(fnames, free);
  }
}

hash_table*
update_db (hash_table* db, time_t curr, dir_config* dir_conf, ...) {
  // We HAVE to make a brand new db each time, else we will not be able to
  // tell if a file was deleted
  hash_table* new_db = ht_init_or_panic(0, (free_fn*)free_crontab);

  va_list args;
  va_start(args, dir_conf);

  while (dir_conf != NULL) {
    scan_crontabs(db, new_db, *dir_conf, curr);

    dir_conf = va_arg(args, dir_config*);
  }

  va_end(args);
  ht_delete_table(db);

  return new_db;
}
