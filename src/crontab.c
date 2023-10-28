#include "crontab.h"

#include <dirent.h>
#include <fcntl.h>
#include <pwd.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include "cronentry.h"
#include "defs.h"
#include "log.h"
#include "parser.h"
#include "util.h"

/*
 * Because crontab/at files may be owned by their respective users we
 * take extreme care in opening them.  If the OS lacks the O_NOFOLLOW
 * we will just have to live without it.  In order for this to be an
 * issue an attacker would have to subvert group CRON_GROUP.
 * TODO: comment
 */
#ifndef O_NOFOLLOW
#define O_NOFOLLOW 0
#endif

#define ALL_PERMS 07777
#define OWNER_RW_PERMS 0600

#define MAXENTRIES 256
#define RW_BUFFER 1024

array_t* get_filenames(char* dpath) {
  DIR* dir;

  if ((dir = opendir(dpath)) != NULL) {
    write_to_log("scanning dir %s\n", dpath);
    struct dirent* den;

    array_t* file_names = array_init();

    while ((den = readdir(dir)) != NULL) {
      // Skip pointer files
      if (s_equals(den->d_name, ".") || s_equals(den->d_name, "..")) {
        continue;
      }

      write_to_log("found file %s/%s\n", dpath, den->d_name);
      array_push(file_names, s_copy(den->d_name));
    }

    closedir(dir);
    return file_names;
  } else {
    perror("opendir");
    write_to_log("unable to scan dir %s\n", dpath);
  }

  return NULL;
}

bool file_not_owned_by(struct stat* statbuf, struct passwd* pw, char* uname) {
  return statbuf->st_uid != ROOT_UID &&
         (pw == NULL || statbuf->st_uid != pw->pw_uid ||
          strcmp(uname, pw->pw_name) != 0);
}

int get_crontab_fd_if_valid(char* fpath, char* uname, time_t last_mtime,
                            struct stat* statbuf) {
  int not_ok = OK - 1;
  struct passwd* pw = NULL;
  int crontab_fd = not_ok;

#ifndef UNIT_TEST
  // No user found in passwd
  if (!s_equals(uname, ROOT_UNAME) && (pw = getpwnam(uname)) == NULL) {
    write_to_log("user %s not found\n", uname);
    goto dont_process;
  }
#endif

  // Open in readonly, non-blocking mode. Don't follow symlinks.
  // Not following symlinks is largely for security
  if ((crontab_fd = open(fpath, O_RDONLY | O_NONBLOCK | O_NOFOLLOW, 0)) < OK) {
    write_to_log("cannot read %s\n", fpath);
    goto dont_process;
  }

  if (fstat(crontab_fd, statbuf) < OK) {
    write_to_log("fstat failed for fd %d (file %s)\n", crontab_fd, fpath);
    goto dont_process;
  }

  if (!S_ISREG(statbuf->st_mode)) {
    write_to_log("file %s is not a regular file");
    goto dont_process;
  }

#ifndef UNIT_TEST
  int file_perms = statbuf->st_mode & ALL_PERMS;

  if (file_perms != OWNER_RW_PERMS) {
    write_to_log(
        "file %s has invalid permissions (should be r/w by owner only but got "
        "%o)\n",
        fpath, file_perms);
    goto dont_process;
  }

  // For non-root, check that the given user matches the file owner
  if (file_not_owned_by(statbuf, pw, uname)) {
    write_to_log("file %s not owned by specified user %s (it's owned by %s)\n",
                 fpath, uname, pw->pw_name);
    goto dont_process;
  }
#endif

  // Ensure there aren't any hard links
  if (statbuf->st_nlink != 1) {
    write_to_log("file %s has hard symlinks (%d)\n", fpath, statbuf->st_nlink);
    goto dont_process;
  }

  if (last_mtime > -1) {
    // It shouldn't ever be greater than but whatever
    if (last_mtime >= statbuf->st_mtime) {
      write_to_log("file has not changed (last mtime: %ld, curr mtime: %ld)\n",
                   fpath, last_mtime, statbuf->st_mtime);
      goto dont_process;
    }
  }

  return crontab_fd;

dont_process:
  return not_ok;
}

Crontab* new_crontab(int crontab_fd, bool is_root, time_t curr_time,
                     time_t mtime, char* uname) {
  FILE* fd;

  if (!(fd = fdopen(crontab_fd, "r"))) {
    write_to_log("fdopen on crontab_fd %d failed\n", crontab_fd);
    // TODO: perrors
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

  array_t* entries = array_init();

  while (fgets(buf, sizeof(buf), fd) != NULL && --max_lines) {
    char* ptr = buf;
    int len;

    // Skip whitespace and newlines
    while (*ptr == ' ' || *ptr == '\t' || *ptr == '\n') ++ptr;

    // Remove trailing newline
    len = strlen(ptr);
    if (len && ptr[len - 1] == '\n') ptr[--len] = 0;

    // If that's it or the entire line is a comment, skip
    if (!should_parse_line(ptr)) continue;

    if (--max_entries == 0) break;

    // TODO: handle
    CronEntry* entry = new_cron_entry(ptr, curr_time, uname);
    write_to_log("New entry (%d) for crontab %s\n", entry->id, uname);
    array_push(entries, entry);
  }

  fclose(fd);

  Crontab* ct = xmalloc(sizeof(Crontab));
  ct->entries = entries;
  ct->mtime = mtime;

  return ct;
}

void scan_crontabs(hash_table* old_db, hash_table* new_db, DirConfig dir_conf,
                   time_t curr) {
  array_t* fnames = get_filenames(dir_conf.name);
  // If no files, fall through to db replacement
  // This will handle removal of any files that were deleted during runtime
  if (has_elements(fnames)) {
    foreach (fnames, i) {
      char* fname = array_get(fnames, i);
      char* fpath;
      if (!(fpath = fmt_str("%s/%s", dir_conf.name, fname))) {
        write_to_log("failed to concatenate as %s/%s\n", dir_conf.name, fname);
        continue;
      }
      Crontab* ct = (Crontab*)ht_get(old_db, fpath);

      int crontab_fd;
      struct stat statbuf;

      write_to_log("scanning file %s...\n", fpath);

      char* uname = dir_conf.is_root ? ROOT_UNAME : fname;
      // File hasn't been processed before
      if (!ct) {
        if ((crontab_fd = get_crontab_fd_if_valid(fpath, uname, 0, &statbuf)) <
            OK) {
          write_to_log("file %s not valid; continuing...\n", fpath);
          continue;
        }

        write_to_log("creating new crontab from file %s...\n", fpath);

        ct = new_crontab(crontab_fd, dir_conf.is_root, curr, statbuf.st_mtime,
                         fname);
        // TODO: free existing values
        ht_insert_ptr(new_db, fpath, ct);
      }
      // File exists in db
      else {
        write_to_log("crontab for file %s exists...\n", fpath);

        // Skips mtime check
        if ((crontab_fd = get_crontab_fd_if_valid(fpath, uname, -1, &statbuf)) <
            OK) {
          write_to_log(
              "existing crontab file %s not valid; it won't be carried over. "
              "continuing...\n",
              fpath);
          continue;
        }

        // not modified, just renew the entries
        // TODO: research whether we should adjust the precision of our mtime
        if (ct->mtime >= statbuf.st_mtime) {
          write_to_log(
              "existing file %s not modified, renewing entries if any\n",
              fpath);
          foreach (ct->entries, i) {
            CronEntry* entry = array_get(ct->entries, i);
            renew_cron_entry(entry, curr);
          }
        } else {
          write_to_log("existing file %s was modified, recreating crontab\n",
                       fpath);

          // modified, re-process
          ct = new_crontab(crontab_fd, dir_conf.is_root, curr, statbuf.st_mtime,
                           fname);
        }

        ht_insert_ptr(new_db, fpath, ct);
      }
    }

    array_free(fnames);
  }
}

void update_db(hash_table* db, time_t curr, DirConfig* dir_conf, ...) {
  // We HAVE to make a brand new db each time, else we will not be able to
  // tell if a file was deleted
  hash_table* new_db = ht_init(0);

  va_list args;
  va_start(args, dir_conf);

  while (dir_conf != NULL) {
    scan_crontabs(db, new_db, *dir_conf, curr);

    dir_conf = va_arg(args, DirConfig*);
  }

  va_end(args);

  // TODO: we have to remove records that we copied over to the new_db first
  // ht_delete_table_ptr(db);

  *db = *new_db;
}
