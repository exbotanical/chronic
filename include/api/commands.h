#ifndef COMMANDS_H
#define COMMANDS_H

#include "libhash/libhash.h"
#include "libutil/libutil.h"

/**
 * Retrieves the command handlers map singleton.
 * Initialized lazily.
 *
 * @return hash_table* i.e. HashTable<char*, void (*handler)(int)>
 */
hash_table* get_command_handlers_map(void);

void write_jobs_info(buffer_t* buf);
void write_crontabs_info(buffer_t* buf);
void write_program_info(buffer_t* buf);

#endif /* COMMANDS_H */
