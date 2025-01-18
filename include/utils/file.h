#ifndef FILE_UTILS_H
#define FILE_UTILS_H

#include "libutil/libutil.h"

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
