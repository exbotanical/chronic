#ifndef FILE_UTILS_H
#define FILE_UTILS_H

#include <fcntl.h>
#include <pwd.h>
#include <sys/stat.h>
#include <unistd.h>

#include "constants.h"
#include "libutil/libutil.h"
#include "utils/xpanic.h"

// Some systems won't have this option and we'll just have to live without it.
#ifndef O_NOFOLLOW
#  define O_NOFOLLOW 00400000
#endif

/**
 * Returns true if the file represented by the stat entity is owned by the
 * user (represented by the pw entity).
 *
 * @param statbuf The stat struct for the file we're evaluating.
 * @param pw The user's passwd entry.
 * @return true The file IS owned by the given user.
 * @return false The file IS NOT owned by the given user.
 */
static inline bool
is_file_owner (struct stat *statbuf, struct passwd *pw) {
  if (!statbuf || !pw) {
    xpanic("statbuf and/or pw was unexpectedly NULL\n");
  }
  return statbuf->st_uid == pw->pw_uid;
}

static inline bool
has_hard_links (struct stat *statbuf) {
  return statbuf->st_nlink != 1;
}

static inline bool
is_read_exec_by_owner (struct stat *statbuf) {
  return (statbuf->st_mode & OWNER_RX_PERMS) == OWNER_RX_PERMS;
}

static inline bool
is_read_write_by_owner (struct stat *statbuf) {
  return (statbuf->st_mode & OWNER_RW_PERMS) == OWNER_RW_PERMS;
}

static inline bool
is_executable (const char *fpath) {
  return access(fpath, X_OK) == 0;
}

static inline int
safe_open (const char *fpath) {
  return open(fpath, O_RDONLY | O_NONBLOCK | O_NOFOLLOW, 0);
}

/**
 * Given a directory path, returns an array of all of the valid filenames
 * therein.
 *
 * @param dirpath
 * @param regex optional - if passed, only matches will be returned
 */
array_t *get_filenames(char *dirpath, const char *regex);

/**
 * Returns a bool indicating whether a file at the given path exists.
 *
 * @param path
 * @return true The file exists
 * @return false The file does not exist
 */
bool file_exists(const char *path);

#endif /* FILE_UTILS_H */
