#include "defs.h"

array_t* get_filenames(char* dpath) {
  DIR* dir;

  if ((dir = opendir(dpath)) != NULL) {
    struct dirent* den;

    array_t* file_names = array_init();

    while ((den = readdir(dir)) != NULL) {
      // Skip pointer files
      if (s_equals(den->d_name, ".") || s_equals(den->d_name, "..")) {
        continue;
      }

      array_push(file_names, s_copy(den->d_name));
    }

    closedir(dir);
    return file_names;
  } else {
    perror("opendir");
    write_to_log("unable to scan directory %s\n", dpath);
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
  if ((pw = getpwnam(uname)) == NULL) {
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

  struct stat statbuf2;
  fstat(crontab_fd, &statbuf2);

  if (!S_ISREG(statbuf->st_mode)) {
    write_to_log("file %s not regular");
    goto dont_process;
  }

#ifndef UNIT_TEST
  if ((statbuf->st_mode & ALL_PERMS) != OWNER_RW_PERMS) {
    write_to_log("file %s has invalid permissions (r/w by owner only)\n");
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

  array_t* jobs = array_init();

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
    Job* job = new_job(ptr, curr_time, uname);
    array_push(jobs, job);
  }

  fclose(fd);

  Crontab* ct = xmalloc(sizeof(Crontab));
  ct->jobs = jobs;
  ct->mtime = mtime;

  return ct;
}

// We HAVE to make a brand new db each time, else we will not be able to tell if
// a file was deleted
void scan_crontabs(hash_table* db, DirConfig dir_conf, time_t curr) {
  // TODO: handle
  array_t* fnames = get_filenames(dir_conf.name);

  hash_table* new_db = ht_init(0);

  foreach (fnames, i) {
    char* fname = array_get(fnames, i);
    Crontab* ct = (Crontab*)ht_get(db, fname);

    char* fpath;
    if (!(fpath = fmt_str("%s/%s", dir_conf.name, fname))) {
      write_to_log("failed to concatenate as %s/%s\n", dir_conf.name, fname);
      continue;
    }

    int crontab_fd;
    struct stat statbuf;
    // File hasn't been processed before
    if (!ct) {
      if ((crontab_fd = get_crontab_fd_if_valid(fpath, fname, 0, &statbuf)) <
          OK) {
        continue;
      }

      ct = new_crontab(crontab_fd, dir_conf.is_root, curr, statbuf.st_mtime,
                       fname);

      // TODO: free existing values
      ht_insert(new_db, fname, ct);
    }
    // File exists in db
    else {
      // Skips mtime check
      if ((crontab_fd = get_crontab_fd_if_valid(fpath, fname, -1, &statbuf)) <
          OK) {
        continue;
      }

      // not modified, just renew the jobs
      // TODO: research whether we should adjust the precision of our mtime
      if (ct->mtime >= statbuf.st_mtime) {
        foreach (ct->jobs, i) {
          Job* job = array_get(ct->jobs, i);
          renew_job(job, curr);
        }
      } else {
        // modified, re-process
        // TODO: free
        ct = new_crontab(crontab_fd, dir_conf.is_root, curr, statbuf.st_mtime,
                         fname);
      }

      // TODO: remove from old db
      ht_insert(new_db, fname, ct);
    }
  }

  array_free(fnames);
  // TODO: free
  *db = *new_db;
}
