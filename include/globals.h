#ifndef GLOBALS_H
#define GLOBALS_H

#include "cli.h"
#include "libhash/libhash.h"
#include "libutil/libutil.h"
#include "proginfo.h"
#include "user.h"

/**
 * Command-line interface configuration options.
 */
extern cli_opts opts;

/**
 * Info about the user that executed this program.
 */
extern user_t usr;

/**
 * Info about the program itself.
 */
extern proginfo_t proginfo;

/**
 * Stores all valid crontabs.
 * i.e. HashTable<char*, crontab_t*> where char* is the file name.
 */
extern hash_table *db;

/**
 * Queue of jobs that need to run.
 * i.e. List<job_t*>
 */
extern array_t *job_queue;

/**
 * Queue of jobs that have been completed and need to be reported.
 * i.e. List<job_t*>
 */
extern array_t *mail_queue;

/**
 * Stores stringified job state enum names for convenience.
 */
extern const char *job_state_names[];

#endif /* GLOBALS_H */
