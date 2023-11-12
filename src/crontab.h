#ifndef FS_H
#define FS_H

#include <stdbool.h>
#include <time.h>

#include "libhash/libhash.h"
#include "libutil/libutil.h"

/**
 * Holds metadata and configuration for one of the crontab directories being
 * tracked.
 *
 * TODO: check dir mtime as well?
 */
typedef struct {
  /**
   * Does this directory house system i.e. root crontabs?
   */
  bool is_root;
  /**
   * The absolute path of the directory.
   */
  char *path;
} DirConfig;

/**
 * Represents a single crontab (file). We assume (and we enforce this in the
 * crontab program) each file will be named after its user, except for root/sys
 * crontabs.
 */
typedef struct {
  /**
   * The last time this crontab file was modified.
   */
  time_t mtime;
  /**
   * The owning user's username (and name of the crontab file).
   */
  char *uname;
  /**
   * An array of this crontab's entries.
   */
  array_t *entries;
  /**
   * A mapping of variables (key/value pairs) set in the crontab.
   */
  hash_table *vars;
  /**
   * A char* array of vars, concatenated such that each string is represented as
   * "key=value" literals.
   */
  char **envp;
} Crontab;

/**
 * Given a directory path, returns an array of all of the valid filenames
 * therein.
 *
 * @param dirpath
 */
array_t *get_filenames(char *dirpath);

/**
 * Parses a file into new crontab.
 *
 * @param crontab_fd The file descriptor for the crontab file.
 * @param is_root A flag indicating whether this is a system (root-owned)
 * crontab.
 * @param curr_time The current time.
 * @param mtime The last modified time of the file represented by `crontab_fd`.
 * @param uname The username (and filename).
 * @return Crontab*
 */
Crontab *new_crontab(int crontab_fd, bool is_root, time_t curr_time,
                     time_t mtime, char *uname);

/**
 * Scans all crontabs in the given directory and updates them if needed.
 *
 * @param old_db A pointer to the current database.
 * @param new_db A pointer to the new database; all updates will be placed here.
 * @param dir_conf The dir config for the directory to be scanned.
 * @param curr The current time.
 *
 * TODO: static
 */
void scan_crontabs(hash_table *old_db, hash_table *new_db, DirConfig dir_conf,
                   time_t curr);

/**
 * Updates the crontab database by scanning all files that have been modified.
 *
 * @param db A pointer to the crontab database.
 * @param curr The current time.
 * @param dir_conf A variadic list of directories to be scanned.
 * @param ...
 */
void update_db(hash_table *db, time_t curr, DirConfig *dir_conf, ...);

#endif /* FS_H */
