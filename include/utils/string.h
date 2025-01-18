#ifndef STRING_UTILS_H
#define STRING_UTILS_H

#define X(e) [e] = #e

/**
 * Trims all whitespace surrounding a given string.
 *
 * @param str
 * @return char*
 */
char* trim_whitespace(char* str);

/**
 * Replaces all tabs in the given string with spaces.
 *
 * @param str
 */
void replace_tabs_with_spaces(char* str);

/**
 * Generates a v4 UUID. Must be freed by caller.
 */
char* create_uuid(void);

#endif /* STRING_UTILS_H */
