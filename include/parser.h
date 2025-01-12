#ifndef PARSER_H
#define PARSER_H

#include <stdbool.h>

#include "cronentry.h"
#include "libhash/libhash.h"
#include "util.h"

typedef enum { ENV_VAR_ADDED, SKIP_LINE, ENTRY, DONE } parse_ln_result;

/**
 * Strips the preceding '#'s out of a string and return it.
 * This is a dumb function and it doesn't handle any edge cases, so use it judiciously.
 * @param str
 */
void strip_comment(char *str);

char           *until_nth_of_char(const char *str, char c, int n);
bool            is_comment_line(const char *str);
bool            should_parse_line(const char *line);
void            extract_vars(const char *s, hash_table *ht);
retval_t        parse_cmd(char *line, cron_entry *entry, int count);
retval_t        parse_entry(cron_entry *entry, char *line);
parse_ln_result parse_line(char *ptr, int max_entries, hash_table *ht);

#endif /* PARSER_H */
