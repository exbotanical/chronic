#ifndef REGEXPR_H
#define REGEXPR_H

#include <stdbool.h>

#include "libhash/libhash.h"

bool match_variable(char *line, hash_table *vars);

#endif /* REGEXPR_H */
