#ifndef COMMANDS_H
#define COMMANDS_H

#include "libhash/libhash.h"

/**
 * Retrieves the command handlers map singleton.
 * Initialized lazily.
 *
 * @return hash_table* i.e. HashTable<char*, void (*handler)(int)>
 */
hash_table* get_command_handlers_map(void);

#endif /* COMMANDS_H */
