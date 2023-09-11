#ifndef PARSER_H
#define PARSER_H

#include <time.h>

#include "defs.h"

time_t parse(time_t curr, Job* job, char* line);

array_t* process_dir(char* dpath);

#endif /* PARSER_H */
