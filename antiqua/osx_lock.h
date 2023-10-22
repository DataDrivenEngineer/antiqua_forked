#ifndef _OSX_LOCK_H_
#define _OSX_LOCK_H_

#include "types.h"
#include <pthread.h>

extern pthread_mutex_t mutex;

MONExternC void lock(void);
MONExternC void unlock(void);

#endif
