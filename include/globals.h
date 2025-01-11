#ifndef GLOBALS_H
#define GLOBALS_H

#include "cli.h"
#include "libhash/libhash.h"
#include "libutil/libutil.h"
#include "proginfo.h"
#include "user.h"

extern cli_opts   opts;
extern user_t     usr;
extern proginfo_t proginfo;

extern hash_table *db;
extern array_t    *job_queue;
extern array_t    *mail_queue;

#endif /* GLOBALS_H */
