#ifndef CRONTAB_H
#define CRONTAB_H

#include <stdbool.h>
#include <time.h>

#include "libhash/libhash.h"
#include "libutil/libutil.h"

typedef enum {
  /**
   * No cadence i.e. parse the cron entry and figure out the expr.
   */
  CADENCE_NA,
  CADENCE_HOURLY,
  CADENCE_DAILY,
  CADENCE_WEEKLY,
  CADENCE_MONTHLY,
} cadence_t;

/**
 * Holds metadata and configuration for one of the crontab directories being
 * tracked.
 */
typedef struct {
  /**
   * Does this directory house system i.e. root crontabs?
   */
  bool        is_root;
  /**
   * Is this a virtual crontab dir e.g. cron.daily, or does it have actual crontabs?
   */
  bool        is_virtual;
  /**
   * A cadence for virtual, recurring cron jobs. These are directories such as cron.daily,
   * which don't actually have any crontabs therein. Instead, they have a number of scripts
   * which should be executed at the cadence specified by the directory's name. We call these
   * virtual crontabs because we create a "virtual" crontab and fill out the details at runtime.
   *
   * If this is CADENCE_NA, the directory has actual crontabs and will be processed normally.
   */
  cadence_t   cadence;
  /**
   * The absolute path of the directory.
   */
  char       *path;
  /**
   * Optional regex to match against when scanning the directory for crontabs.
   */
  const char *regex;
} dir_config;

/**
 * Represents a single crontab (file). We assume (and we enforce this in the
 * crontab program) each file will be named after its user, except for root/sys
 * crontabs.
 */
typedef struct {
  /**
   * The last time this crontab file was modified.
   */
  time_t      mtime;
  /**
   * The owning user's username (and name of the crontab file).
   */
  char       *uname;
  /**
   * An array of this crontab's entries.
   * i.e. List<cron_entry*>
   */
  array_t    *entries;
  /**
   * A mapping of variables (key/value pairs) set in the crontab.
   * i.e. HashTable<char*, char*>
   */
  hash_table *vars;
  /**
   * A char* array of vars, concatenated such that each string is represented as
   * "key=value" literals.
   */
  char      **envp;
} crontab_t;

/**
 * Parses a file into new crontab.
 *
 * @param crontab_fd The file descriptor for the crontab file.
 * @param is_root A flag indicating whether this is a system (root-owned)
 * crontab.
 * @param curr_time The current time.
 * @param mtime The last modified time of the file represented by `crontab_fd`.
 * @param uname The username (and filename).
 * @return crontab_t*
 */
crontab_t *new_crontab(int crontab_fd, bool is_root, time_t curr_time, time_t mtime, char *uname);

/**
 * Deallocates a crontab.
 */
void free_crontab(crontab_t *ct);

/**
 * Scans all crontabs in the given directory and updates them if needed.
 *
 * @param old_db A pointer to the current database.
 * @param new_db A pointer to the new database; all updates will be placed here.
 * @param dir_conf The dir config for the directory to be scanned.
 * @param curr The current time.
 */
void scan_crontabs(hash_table *old_db, hash_table *new_db, dir_config *dir_conf, time_t curr);

/**
 * Same as scan_crontabs except this looks for executable files in lieu of actual crontabs. Then, for each executable,
 * it creates a "virtual" crontab (internal data structure) to represent the executable. Thus, the virtual crontab
 * is a singleton / has a single entry, always; the mtime comparison occurs on the script itself.
 *
 * This allows us to support canonical system cron directories such as cron.daily.
 *
 * @param old_db
 * @param new_db
 * @param dir_conf
 * @param curr
 * @param cadence The cadence of the virtual cron entry, since there is no actual cron expr.
 */
void scan_virtual_crontabs(hash_table *old_db, hash_table *new_db, dir_config *dir_conf, time_t curr, cadence_t cadence);

/**
 * Updates the crontab database by scanning all files that have been modified.
 *
 * @param db A pointer to the crontab database.
 * @param curr The current time.
 * @param dir_conf A variadic list of directories to be scanned.
 * @param ...
 */
hash_table *update_db(hash_table *db, time_t curr, dir_config *dir_conf, ...);

#endif /* CRONTAB_H */
