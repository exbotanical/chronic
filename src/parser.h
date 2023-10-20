#ifndef PARSER_H
#define PARSER_H

#include "defs.h"

void strip_comment(char *str);
char *until_nth_of_char(const char *str, char c, int n);
RETVAL parse_cmd(char *line, Job *job, int count);
RETVAL parse(Job *job, char *line);
bool is_comment_line(const char *str);
bool should_parse_line(const char *line);

#endif /* PARSER_H */
