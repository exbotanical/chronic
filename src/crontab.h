#ifndef FS_H
#define FS_H

#include <stdbool.h>
#include <time.h>

#include "libhash/libhash.h"
#include "libutil/libutil.h"

// TODO: check dir mtime as well?
typedef struct {
  char *name;
  bool is_root;
} DirConfig;

typedef struct {
  array_t *jobs;
  time_t mtime;
} Crontab;

array_t *get_filenames(char *dpath);
Crontab *new_crontab(int crontab_fd, bool is_root, time_t curr_time,
                     time_t mtime, char *uname);
void scan_crontabs(hash_table *db, DirConfig dir_conf, time_t curr);

#endif /* FS_H */
