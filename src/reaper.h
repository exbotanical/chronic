#ifndef REAPER_H
#define REAPER_H

#include <pthread.h>

#include "libutil/libutil.h"

void signal_reap_routine(void);
void* reap_routine(void* _arg);

#endif /* REAPER_H */
