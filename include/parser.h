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
void            strip_comment(char *str);
bool            is_comment_line(const char *str);
bool            should_parse_line(const char *line);
retval_t        parse_schedule(const char *s, char *dest);
retval_t        parse_cmd(char *s, char *dest);
retval_t        parse_expr(cron_entry *entry);
retval_t        parse_entry(cron_entry *entry, const char *line);
parse_ln_result parse_line(char *ptr, int max_entries, hash_table *ht);

#endif /* PARSER_H */
