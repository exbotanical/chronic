#ifndef PARSER_H
#define PARSER_H

#include <stdbool.h>

#include "cronentry.h"
#include "libhash/libhash.h"
#include "util.h"

typedef enum { ENV_VAR_ADDED, SKIP_LINE, ENTRY, DONE } ParseLineResult;

void   strip_comment(char *str);
char  *until_nth_of_char(const char *str, char c, int n);
bool   is_comment_line(const char *str);
bool   should_parse_line(const char *line);
void   extract_vars(const char *s, hash_table *ht);
Retval parse_cmd(char *line, CronEntry *entry, int count);

Retval          parse_entry(CronEntry *entry, char *line);
ParseLineResult parse_line(char *ptr, int max_entries, hash_table *ht);

#endif /* PARSER_H */
