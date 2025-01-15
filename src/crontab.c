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

#include "config.h"
#include "cronentry.h"
#include "globals.h"
#include "logger.h"
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
      ht_insert(vars, HOMEDIR_ENVVAR, s_copy_or_panic(pw->pw_dir));
    }

    if (!ht_search(vars, SHELL_ENVVAR)) {
      ht_insert(vars, SHELL_ENVVAR, s_copy_or_panic(pw->pw_shell));
    }

    if (!ht_search(vars, UNAME_ENVVAR)) {
      ht_insert(vars, UNAME_ENVVAR, s_copy_or_panic(pw->pw_name));
    }

    if (!ht_search(vars, PATH_ENVVAR)) {
      ht_insert(vars, PATH_ENVVAR, s_copy_or_panic(DEFAULT_PATH));
    }
  } else {
    log_warn(
      "Something went really awry and uname=%s wasn't found in the "
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
    HT_ITER_START(vars)
    ct->envp[idx] = s_fmt("%s=%s", entry->key, entry->value);
    idx++;
    HT_ITER_END

    // `execle` requires it to be NULL-terminated
    ct->envp[idx] = NULL;
  }
}

int
get_crontab_fd_if_valid (char* fpath, char* uname, time_t last_mtime, struct stat* statbuf, bool virtual) {
  int not_ok     = OK - 1;
  int crontab_fd = not_ok;

#ifndef UNIT_TEST
  struct passwd* pw = NULL;
  // No user found in passwd
  if (!s_equals(uname, ROOT_UNAME) && (pw = getpwnam(uname)) == NULL) {
    log_warn("user %s not found\n", uname);
    goto dont_process;
  }
#endif

  // Open in readonly, non-blocking mode. Don't follow symlinks.
  // Not following symlinks is largely for security
  if ((crontab_fd = open(fpath, O_RDONLY | O_NONBLOCK | O_NOFOLLOW, 0)) < OK) {
    log_warn("cannot read %s (reason=%s)\n", fpath, strerror(errno));
    goto dont_process;
  }

  if (fstat(crontab_fd, statbuf) < OK) {
    log_warn("fstat failed for fd %d (file %s)\n", crontab_fd, fpath);
    goto dont_process;
  }

  if (!S_ISREG(statbuf->st_mode)) {
    log_warn("file %s is not a regular file", fpath);
    goto dont_process;
  }

#ifndef UNIT_TEST
  if (virtual) {
    // If it's virtual, that means each file is actually an executable. We need at least r/x for the owner.
    if ((statbuf->st_mode & OWNER_RX_PERMS) == 0) {
      log_warn(
        "file %s has invalid permissions (should be r/x by owner only but got "
        "%o)\n",
        fpath,
        statbuf->st_mode & ALL_PERMS
      );
      goto dont_process;
    }
    if (access(fpath, X_OK) != 0) {
      log_warn("file %s is not executable but was expected to be\n", fpath);
      goto dont_process;
    }
  } else {
    // It's a regular crontab...
    if ((statbuf->st_mode & OWNER_RW_PERMS) == 0) {
      log_warn(
        "file %s has invalid permissions (should be r/w by owner only but got "
        "%o)\n",
        fpath,
        statbuf->st_mode & ALL_PERMS
      );
      goto dont_process;
    }
  }

  // Verify that the user the file is named after actually owns said file,
  // unless we're root.
  if (!usr.root && !is_file_owner(statbuf, pw)) {
    log_warn("file %s not owned by specified user %s (it's owned by %s)\n", fpath, uname, pw->pw_name);
    goto dont_process;
  }

  // This time we're checking that the user who executed this program matches
  // the crontab owner, unless we're root;.
  if (!usr.root && pw->pw_uid != usr.uid) {
    log_warn("current user %s must own this crontab (%s) or be root to schedule it\n ", uname, fpath);
    goto dont_process;
  }
#endif

  // Ensure there aren't any hard links
  if (statbuf->st_nlink != 1) {
    log_warn("file %s has hard symlinks (%d)\n", fpath, statbuf->st_nlink);
    goto dont_process;
  }

  if (last_mtime > -1) {
    // It shouldn't ever be greater than but whatever
    if (last_mtime >= statbuf->st_mtime) {
      log_debug("file has not changed (mtime: %ld)\n", fpath, last_mtime);
      goto dont_process;
    }
  }

  return crontab_fd;

dont_process:
  return not_ok;
}

crontab_t*
new_virtual_crontab (time_t curr_time, time_t mtime, char* uname, char* fpath, cadence_t cadence) {
  crontab_t* ct     = xmalloc(sizeof(crontab_t));
  ct->mtime         = mtime;
  ct->uname         = uname;
  ct->envp          = NULL;
  ct->entries       = array_init_or_panic();
  ct->vars          = ht_init_or_panic(0, free);

  cron_entry* entry = new_cron_entry(fpath, curr_time, ct, cadence);
  if (!entry) {
    log_warn(
      "Failed to parse what was thought to be a cron entry: %s (user "
      "%s)\n",
      fpath,
      uname
    );

    return NULL;
  }

  array_push_or_panic(ct->entries, entry);
  complete_env(ct);

  return ct;
}

crontab_t*
new_crontab (int crontab_fd, bool is_root, time_t curr_time, time_t mtime, char* uname) {
  FILE* fd;
  if (!(fd = fdopen(crontab_fd, "r"))) {
    log_warn("fdopen on crontab_fd %d failed (reason: %s)\n", crontab_fd, strerror(errno));
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
        cron_entry* entry = new_cron_entry(ptr, curr_time, ct, CADENCE_NA);
        if (!entry) {
          log_warn(
            "Failed to parse what was thought to be a cron entry: %s (user "
            "%s)\n",
            ptr,
            uname
          );
          continue;
        }

        log_debug("New entry (%d) for crontab %s\n", entry->id, uname);
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

static inline void
renew_crontab_entries (crontab_t* ct, time_t curr) {
  foreach (ct->entries, i) {
    cron_entry* entry = array_get_or_panic(ct->entries, i);
    renew_cron_entry(entry, curr);
  }
}

void
scan_crontabs (hash_table* old_db, hash_table* new_db, dir_config* dir_conf, time_t curr) {
  array_t* fnames = get_filenames(dir_conf->path, dir_conf->regex);

  // If no files are found in the directory, fall through to the database
  // replacement. This will handle removal of any files that were deleted
  // during runtime. If there are files, we check each.
  if (!fnames) {
    return;
  }

  foreach (fnames, i) {
    char* fname = array_get_or_panic(fnames, i);

    char* fpath;
    if (!(fpath = s_fmt("%s/%s", dir_conf->path, fname))) {
      log_warn("failed to concatenate as %s/%s\n", dir_conf->path, fname);
      continue;
    }

    crontab_t*  ct = ht_get(old_db, fpath);
    int         crontab_fd;
    struct stat statbuf;
    char*       uname = dir_conf->is_root ? ROOT_UNAME : fname;

    log_debug("scanning file %s...\n", fpath);

    // The file hasn't been processed before. Create the new crontab.
    if (!ct) {
      if ((crontab_fd = get_crontab_fd_if_valid(fpath, uname, 0, &statbuf, false)) < OK) {
        log_warn("file %s not valid; continuing...\n", fpath);
        continue;
      }

      log_debug("creating new crontab from file %s...\n", fpath);

      ct = new_crontab(crontab_fd, dir_conf->is_root, curr, statbuf.st_mtime, s_copy_or_panic(uname));
    } else {
      // The file has been processed before. If the file has been modified, we need to re-process it.
      // Otherwise, renewing the next run time is sufficient.
      log_debug("crontab for file %s exists...\n", fpath);

      // Renew the fd and statbuf
      if ((crontab_fd = get_crontab_fd_if_valid(fpath, uname, -1, &statbuf, false)) < OK) {
        log_warn(
          "existing crontab file %s not valid; it won't be carried over. "
          "continuing...\n",
          fpath
        );
        continue;
      }

      if (ct->mtime >= statbuf.st_mtime) {
        // The crontab was not modified, just renew the entries so we know when to run them next.
        log_debug("existing file %s not modified, renewing entries if any\n", fpath);
        renew_crontab_entries(ct, curr);

        // Make sure we don't accidentally free the old entries since they have been copied over.
        ht_insert(old_db, fpath, NULL);
      } else {
        // The crontab was modified, re-process.
        log_debug("existing file %s was modified, recreating crontab\n", fpath);
        ct = new_crontab(crontab_fd, dir_conf->is_root, curr, statbuf.st_mtime, s_copy_or_panic(fname));
      }
    }

    ht_insert(new_db, fpath, ct);
    close(crontab_fd);
  }

  array_free(fnames, free);
}

void
scan_virtual_crontabs (hash_table* old_db, hash_table* new_db, dir_config* dir_conf, time_t curr, cadence_t cadence) {
  array_t* fnames = get_filenames(dir_conf->path, NULL);
  if (!fnames) {
    return;
  }

  foreach (fnames, i) {
    char* fname = array_get_or_panic(fnames, i);

    char* fpath;
    if (!(fpath = s_fmt("%s/%s", dir_conf->path, fname))) {
      log_warn("failed to concatenate as %s/%s\n", dir_conf->path, fname);
      continue;
    }

    log_debug("scanning cadence file %s...\n", fpath);

    int         crontab_fd;
    struct stat statbuf;
    if ((crontab_fd = get_crontab_fd_if_valid(fpath, ROOT_UNAME, 0, &statbuf, true)) < OK) {
      log_warn("cadence file %s not valid; continuing...\n", fpath);
      continue;
    }
    close(crontab_fd);

    crontab_t* ct    = ht_get(old_db, fpath);
    char*      uname = s_copy(ROOT_UNAME);

    if (!ct) {
      log_debug("creating new virtual crontab from cadence file %s...\n", fpath);
      ct = new_virtual_crontab(curr, statbuf.st_mtime, uname, fpath, cadence);

    } else {
      if (ct->mtime >= statbuf.st_mtime) {
        log_debug("existing cadence file %s not modified, renewing the entry\n", fpath);

        renew_crontab_entries(ct, curr);
        ht_insert(old_db, fpath, NULL);
      } else {
        log_debug("existing cadence file %s was modified, recreating virtual crontab\n", fpath);

        ct = new_virtual_crontab(curr, statbuf.st_mtime, uname, s_copy_or_panic(fname), cadence);
        // Dont set null in else path because we recreate the crontab and want the old one to be freed
      }
    }

    ht_insert(new_db, fpath, ct);
  }

  array_free(fnames, free);
}

hash_table*
update_db (hash_table* db, time_t curr, dir_config* dir_conf, ...) {
  // We HAVE to make a brand new db each time, else we will not be able to
  // tell if a file was deleted
  hash_table* new_db = ht_init_or_panic(0, (free_fn*)free_crontab);

  va_list args;
  va_start(args, dir_conf);

  while (dir_conf != NULL) {
    if (dir_conf->cadence != CADENCE_NA) {
      scan_virtual_crontabs(db, new_db, dir_conf, curr, dir_conf->cadence);
    } else {
      scan_crontabs(db, new_db, dir_conf, curr);
    }
    dir_conf = va_arg(args, dir_config*);
  }

  va_end(args);

  ht_delete_table(db);

  return new_db;
}
