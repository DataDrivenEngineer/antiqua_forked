#ifndef _OSX_LOCK_H_
#define _OSX_LOCK_H_

#include <pthread.h>

#if !defined(__cplusplus)
#define MONExternC extern
#else
#define MONExternC extern "C"
#endif

extern pthread_mutex_t mutex;

MONExternC void lock(void);
MONExternC void unlock(void);

#endif
