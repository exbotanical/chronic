#ifndef FS_H
#define FS_H

#include <stdbool.h>
#include <time.h>

#include "libhash/libhash.h"
#include "libutil/libutil.h"

// TODO: check dir mtime as well?
typedef struct {
  bool is_root;
  char *name;
} DirConfig;

typedef struct {
  time_t mtime;
  char *uname;
  array_t *entries;
  hash_table *vars;
  char **envp;
} Crontab;

array_t *get_filenames(char *dpath);

Crontab *new_crontab(int crontab_fd, bool is_root, time_t curr_time,
                     time_t mtime, char *uname);

void scan_crontabs(hash_table *old_db, hash_table *new_db, DirConfig dir_conf,
                   time_t curr);

void update_db(hash_table *db, time_t curr, DirConfig *dir_conf, ...);

#endif /* FS_H */
