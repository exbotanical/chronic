#ifndef REGEXPR_H
#define REGEXPR_H

#include <stdbool.h>

#include "libhash/libhash.h"

bool match_variable(char *line, hash_table *vars);
bool match_string(const char *string, const char *pattern);

#endif /* REGEXPR_H */
